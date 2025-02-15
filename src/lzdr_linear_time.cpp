#include "lzdr_linear_time.h"
#include "slice.h"
#include "radix_trie.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

// LZDR compression format:
// First byte:
//   0: Combination: byte x byte x length
//   1: Combination: byte x factor x length
//   2: Combination: factor x byte x length
//   3: Combination: factor x factor x length
//   4: Truncation: factor x length
//   5: Repetition: factor x total length
//   6: Repetition: byte x total length
//   7-255: undefined
// byte: uint8_t
// factor, length, repetition: uint32_t
namespace lzdr_compressor {
    Compressor create_compressor_for_combination(const bool first_is_byte, const bool second_is_byte) {
        size_t count = 1 + 4;
        if (first_is_byte) {
            count += 1;
        } else {
            count += 4;
        }
        if (second_is_byte) {
            count += 1;
        } else {
            count += 4;
        }
        Compressor compressor(count);
        if (first_is_byte) {
            if (second_is_byte) {
                compressor.write_byte(0);
            } else {
                compressor.write_byte(1);
            }
        } else {
            if (second_is_byte) {
                compressor.write_byte(2);
            } else {
                compressor.write_byte(3);
            }
        }
        return compressor;
    }

    Compressor create_compressor_for_truncation() {
        constexpr size_t count = 1 + 4 + 4;
        Compressor compressor(count);
        compressor.write_byte(4);
        return compressor;
    }

    Compressor create_compressor_for_repetition(const bool is_byte) {
        size_t count = 1 + 4;
        if (is_byte) {
            count += 1;
        } else {
            count += 4;
        }
        Compressor compressor(count);
        if (is_byte) {
            compressor.write_byte(6);
        } else {
            compressor.write_byte(5);
        }
        return compressor;
    }
}

namespace {
    struct CombinationFactor {
        size_t first_factor;
        size_t second_factor;
        bool first_is_byte;
        bool second_is_byte;
        size_t length;
        bool used_extra_truncation;
        RadixTrieNode* insertion_node;
        size_t first_node_length;
    };
    struct TruncationFactor {
        size_t factor_index;
        size_t length;
        RadixTrieNode* insertion_node;
        size_t last_node_length;
    };
    struct RepetitionFactor {
        size_t factor;
        bool factor_is_byte;
        size_t factor_length;
        size_t total_length;
        RadixTrieNode* insertion_node;
    };

    NextFactorResult2 combination_factor_to_result(const Slice &rest_input, const CombinationFactor &factor) {
        Compressor compressor = lzdr_compressor::create_compressor_for_combination(factor.first_is_byte, factor.second_is_byte);
        if (factor.first_is_byte) {
            compressor.write_byte(static_cast<uint8_t>(factor.first_factor));
        } else {
            compressor.write_u32(static_cast<uint32_t>(factor.first_factor));
        }
        if (factor.second_is_byte) {
            compressor.write_byte(static_cast<uint8_t>(factor.second_factor));
        } else {
            compressor.write_u32(static_cast<uint32_t>(factor.second_factor));
        }
        compressor.write_u32(static_cast<uint32_t>(factor.length));
        const Slice insertion_slice = rest_input.slice(factor.first_node_length, factor.length - factor.first_node_length);
        return NextFactorResult2{rest_input.slice(0, factor.length), std::move(compressor.data()), factor.used_extra_truncation, factor.insertion_node, insertion_slice};
    }

    NextFactorResult2 truncation_factor_to_result(const Slice &rest_input, const TruncationFactor &factor) {
        Compressor compressor = lzdr_compressor::create_compressor_for_truncation();
        compressor.write_u32(static_cast<uint32_t>(factor.factor_index));
        compressor.write_u32(static_cast<uint32_t>(factor.length));
        const Slice insertion_slice = rest_input.slice(factor.last_node_length, factor.length - factor.last_node_length);
        return NextFactorResult2{rest_input.slice(0, factor.length), std::move(compressor.data()), false, factor.insertion_node, insertion_slice};
    }

    NextFactorResult2 repetition_factor_to_result(const Slice &rest_input, const RepetitionFactor &factor) {
        Compressor compressor = lzdr_compressor::create_compressor_for_repetition(factor.factor_is_byte);
        if (factor.factor_is_byte) {
            compressor.write_byte(static_cast<uint8_t>(factor.factor));
        } else {
            compressor.write_u32(static_cast<uint32_t>(factor.factor));
        }
        compressor.write_u32(static_cast<uint32_t>(factor.total_length));
        const bool used_extra_truncation = factor.total_length % factor.factor_length != 0;
        const Slice insertion_slice = factor.factor_is_byte ? rest_input.slice(0, factor.total_length) : rest_input.slice(factor.factor_length, factor.total_length - factor.factor_length);
        NextFactorResult2 result = {rest_input.slice(0, factor.total_length), std::move(compressor.data()), used_extra_truncation, factor.insertion_node, insertion_slice};
        return result;
    }

    NextFactorResult combination_factor_to_result_trunc(const Slice &rest_input, const CombinationFactor &factor, const size_t usable_len) {
        const size_t total_length = std::min(factor.length, usable_len);
        Compressor compressor = lzdr_compressor::create_compressor_for_combination(factor.first_is_byte, factor.second_is_byte);
        if (factor.first_is_byte) {
            compressor.write_byte(static_cast<uint8_t>(factor.first_factor));
        } else {
            compressor.write_u32(static_cast<uint32_t>(factor.first_factor));
        }
        if (factor.second_is_byte) {
            compressor.write_byte(static_cast<uint8_t>(factor.second_factor));
        } else {
            compressor.write_u32(static_cast<uint32_t>(factor.second_factor));
        }
        compressor.write_u32(static_cast<uint32_t>(total_length));
        const bool used_extra_truncation = factor.used_extra_truncation || total_length != factor.length;
        return NextFactorResult{rest_input.slice(0, total_length), std::move(compressor.data()), used_extra_truncation};
    }

    NextFactorResult truncation_factor_to_result_trunc(const Slice &rest_input, const TruncationFactor &factor, const size_t usable_len) {
        const size_t total_length = std::min(factor.length, usable_len);
        Compressor compressor = lzdr_compressor::create_compressor_for_truncation();
        compressor.write_u32(static_cast<uint32_t>(factor.factor_index));
        compressor.write_u32(static_cast<uint32_t>(total_length));
        return NextFactorResult{rest_input.slice(0, total_length), std::move(compressor.data()), false};
    }

    NextFactorResult repetition_factor_to_result_trunc(const Slice &rest_input, const RepetitionFactor &factor, const size_t usable_len) {
        const size_t total_length = std::min(factor.total_length, usable_len);
        Compressor compressor = lzdr_compressor::create_compressor_for_repetition(factor.factor_is_byte);
        if (factor.factor_is_byte) {
            compressor.write_byte(static_cast<uint8_t>(factor.factor));
        } else {
            compressor.write_u32(static_cast<uint32_t>(factor.factor));
        }
        compressor.write_u32(static_cast<uint32_t>(total_length));
        const bool used_extra_truncation = total_length % factor.factor_length != 0;
        NextFactorResult result = {rest_input.slice(0, total_length), std::move(compressor.data()), used_extra_truncation};
        return result;
    }

    size_t naive_lce(const Slice &s, size_t start1, size_t start2) {
        size_t total = 0;
        while (start1 < s.size() && start2 < s.size() && s[start1] == s[start2]) {
            total += 1;
            ++start1;
            ++start2;
        }
        return total;
    }
}

namespace lzdr_linear_time_internal {
    // This method requires rest_input to be not empty!
    NextFactorResult2 next_longest_factor(
        const Slice &entire_input, const size_t bytes_already_read,
        const Slice &rest_input, RadixTrie &previous_factors) {
        // Current nodes
        RadixTrieNode* current_node = &previous_factors.root_node;
        RadixTrieEdge* current_edge = nullptr;
        bool finished_first_factor_node = false;
        size_t edge_rest_text_index = 0;
        // For truncation: combination first node is not always same, as truncation allows splitting nodes
        size_t truncation_last_node_length = 0;

        // Factors
        // Note: The combination factor is initialized with second_factor=0 and second_is_byte=false
        //       so that the second factor is empty by default in case only one factor is needed for the input.
        //       The same is done for the first factor to check if it has been initialized,
        //       that fact will later be used in the while loop.
        CombinationFactor combination_factor = {0, 0, false, false, 0, false, &previous_factors.root_node, 0};
        std::optional<TruncationFactor> truncation_factor = std::nullopt;
        // Initialize repetition factor with maximized single character repetition via LCE
        RepetitionFactor repetition_factor = {rest_input[0], true, 1, 1 + naive_lce(entire_input, bytes_already_read, bytes_already_read + 1), &previous_factors.root_node};

        // Maximize factors
        size_t input_i = 0;
        while (input_i < rest_input.size()) {
            uint8_t current_byte = rest_input[input_i];
            bool is_last_byte = input_i == rest_input.size() - 1;

            // Update combination factor if the current factor is uninitialized,
            // as a combination factor allows one single byte.
            // (We do this eagerly instead of doing this in the case where the edge from the root node does not exist,
            // as it is possible that an edge from the root node exists, but a factor node is never reached)
            if (!finished_first_factor_node) {
                if (combination_factor.first_factor == 0 && !combination_factor.first_is_byte) {
                    combination_factor.first_factor = current_byte;
                    combination_factor.first_is_byte = true;
                    combination_factor.length = input_i + 1;
                }
            } else {
                if (combination_factor.second_factor == 0 && !combination_factor.second_is_byte) {
                    combination_factor.second_factor = current_byte;
                    combination_factor.second_is_byte = true;
                    combination_factor.length = input_i + 1;
                }
            }

            if (current_edge == nullptr) {
                // Currently, we are exactly at a node
                if (auto it = current_node->edges.find(current_byte); it != current_node->edges.end()) {
                    // Edge exists
                    // The current byte got successfully read, therefore increase input_i
                    ++input_i;

                    RadixTrieEdge& edge = it->second;

                    // Update truncation factor / second combination factor
                    // (We do this eagerly instead of waiting for the first mismatch,
                    // as there is no guarantee a mismatch will happen if the input ends early)
                    if (!finished_first_factor_node) {
                        if (edge.rest_text.empty()) {
                            truncation_factor = std::make_optional(TruncationFactor{edge.end_node.next_factor_node_index, input_i, &edge.end_node, input_i});
                        } else {
                            truncation_factor = std::make_optional(TruncationFactor{edge.end_node.next_factor_node_index, input_i, current_node, truncation_last_node_length});
                        }
                    } else {
                        // Make sure to not overwrite single byte second factor
                        // with truncated factor, when truncated length is the same
                        if (input_i > combination_factor.length) {
                            combination_factor.second_factor = edge.end_node.next_factor_node_index;
                            combination_factor.second_is_byte = false;
                            combination_factor.length = input_i;
                            combination_factor.used_extra_truncation = true;
                        }
                    }

                    if (edge.rest_text.empty()) {
                        // Go directly to next node
                        current_node = &edge.end_node;
                        truncation_last_node_length = input_i;

                        if (current_node->index != 0) {
                            // Update first combination factor
                            if (!finished_first_factor_node) {
                                combination_factor.first_factor = current_node->index;
                                combination_factor.first_is_byte = false;
                                combination_factor.length = input_i;
                                combination_factor.insertion_node = current_node;
                                combination_factor.first_node_length = input_i;
                            } else {
                                // This branch is only used to track extra truncations
                                combination_factor.used_extra_truncation = false;
                            }

                            // Update repetition factor
                            if (!finished_first_factor_node) {
                                if (const size_t repetition_len = input_i + naive_lce(entire_input, bytes_already_read, bytes_already_read + input_i); repetition_len > repetition_factor.total_length) {
                                    repetition_factor = RepetitionFactor{current_node->index, false, input_i, repetition_len, current_node};
                                }
                            }
                        }
                    } else {
                        // Update edge
                        current_edge = &edge;
                        edge_rest_text_index = 0;
                    }
                } else {
                    // Edge does not exist
                    // We were unable to read the byte, therefore do not increase input_i
                    if (!finished_first_factor_node) {
                        // Move on to second factor
                        finished_first_factor_node = true;
                        current_node = &previous_factors.root_node;
                        // Second factor begins after end of first factor
                        input_i = combination_factor.length;
                        continue;
                    } else {
                        // Second factor finished, break loop
                        break;
                    }
                }
            } else {
                // We have to iterate over edge text
                if (current_edge->rest_text[edge_rest_text_index] == current_byte) {
                    // The current byte got successfully read, therefore increase input_i
                    ++input_i;
                    // Move to next index of edge rest text
                    ++edge_rest_text_index;

                    // Update truncation factor / second combination factor
                    // (We do this eagerly instead of waiting for the first mismatch,
                    // as there is no guarantee a mismatch will happen if the input ends early)
                    if (!finished_first_factor_node) {
                        if (edge_rest_text_index == current_edge->rest_text.size()) {
                            truncation_factor = std::make_optional(TruncationFactor{current_edge->end_node.next_factor_node_index, input_i, &current_edge->end_node, input_i});
                        } else {
                            truncation_factor = std::make_optional(TruncationFactor{current_edge->end_node.next_factor_node_index, input_i, current_node, truncation_last_node_length});
                        }
                    } else {
                        combination_factor.second_factor = current_edge->end_node.next_factor_node_index;
                        combination_factor.second_is_byte = false;
                        combination_factor.length = input_i;
                        combination_factor.used_extra_truncation = true;
                    }

                    if (edge_rest_text_index == current_edge->rest_text.size()) {
                        // Reached end of edge rest text, go to next node
                        current_node = &current_edge->end_node;
                        current_edge = nullptr;
                        truncation_last_node_length = input_i;

                        if (current_node->index != 0) {
                            // Update first combination factor
                            if (!finished_first_factor_node) {
                                combination_factor.first_factor = current_node->index;
                                combination_factor.first_is_byte = false;
                                combination_factor.length = input_i;
                                combination_factor.insertion_node = current_node;
                                combination_factor.first_node_length = input_i;
                            } else {
                                // This branch is only used to track extra truncations
                                combination_factor.used_extra_truncation = false;
                            }

                            // Update repetition factor
                            if (!finished_first_factor_node) {
                                if (const size_t repetition_len = input_i + naive_lce(entire_input, bytes_already_read, bytes_already_read + input_i); repetition_len > repetition_factor.total_length) {
                                    repetition_factor = RepetitionFactor{current_node->index, false, input_i, repetition_len, current_node};
                                }
                            }
                        }
                    }
                } else {
                    // We were unable to read the byte, therefore do not increase input_i
                    if (!finished_first_factor_node) {
                        // Move on to second factor
                        finished_first_factor_node = true;
                        current_node = &previous_factors.root_node;
                        current_edge = nullptr;
                        // Second factor begins after end of first factor
                        input_i = combination_factor.length;
                        continue;
                    } else {
                        // Second factor finished, break loop
                        break;
                    }
                }
            }

            // Account for the case that we reached the last byte while searching for the
            // first combination factor
            if (!finished_first_factor_node && is_last_byte) {
                // Move on to second factor
                finished_first_factor_node = true;
                current_node = &previous_factors.root_node;
                current_edge = nullptr;
                // Second factor begins after end of first factor
                input_i = combination_factor.length;
            }
        }

        // Return the one that yields the maximum length
        // (where ties are broken such that combination is the preferred method
        // and repetition is preferred least)
        NextFactorResult2 longest_factor = combination_factor_to_result(rest_input, combination_factor);
        if (truncation_factor) {
            if (NextFactorResult2 truncation_result = truncation_factor_to_result(rest_input, *truncation_factor);
                truncation_result.factor_slice.size() > longest_factor.factor_slice.size()) {
                longest_factor = truncation_result;
            }
        }
        if (NextFactorResult2 repetition_result = repetition_factor_to_result(rest_input, repetition_factor);
            repetition_result.factor_slice.size() > longest_factor.factor_slice.size()) {
            longest_factor = repetition_result;
        }
        return longest_factor;
    }

    // This method requires rest_input to be not empty!
    // Basically a copy of the above method, just with `Counted` added.
    // This method should be kept in sync with the normal method.
    NextFactorResult next_longest_factor_counted_trie(
        const Slice &entire_input, const size_t bytes_already_read,
        const Slice &rest_input, const size_t usable_rest_input_len,
        CountedRadixTrie &previous_factors) {
        // Current nodes
        CountedRadixTrieNode* current_node = &previous_factors.root_node;
        CountedRadixTrieEdge* current_edge = nullptr;
        bool finished_first_factor_node = false;
        std::optional<std::list<uint8_t>::iterator> edge_rest_text_iter = std::nullopt;
        bool optimalRepetitionFactorFound = false;

        // Factors
        // Note: The combination factor is initialized with second_factor=0 and second_is_byte=false
        //       so that the second factor is empty by default in case only one factor is needed for the input.
        //       The same is done for the first factor to check if it has been initialized,
        //       that fact will later be used in the while loop.
        CombinationFactor combination_factor = {0, 0, false, false, 0, false};
        std::optional<TruncationFactor> truncation_factor = std::nullopt;
        // Initialize repetition factor with maximized single character repetition via LCE
        RepetitionFactor repetition_factor = {rest_input[0], true, 1, 1 + naive_lce(entire_input, bytes_already_read, bytes_already_read + 1)};

        // Maximize factors
        size_t input_i = 0;
        while (input_i < rest_input.size()) {
            uint8_t current_byte = rest_input[input_i];
            bool is_last_byte = input_i == rest_input.size() - 1;

            // Update combination factor if the current factor is uninitialized,
            // as a combination factor allows one single byte.
            // (We do this eagerly instead of doing this in the case where the edge from the root node does not exist,
            // as it is possible that an edge from the root node exists, but a factor node is never reached)
            if (!finished_first_factor_node) {
                if (combination_factor.first_factor == 0 && !combination_factor.first_is_byte) {
                    combination_factor.first_factor = current_byte;
                    combination_factor.first_is_byte = true;
                    combination_factor.length = input_i + 1;
                }
            } else {
                if (combination_factor.second_factor == 0 && !combination_factor.second_is_byte) {
                    combination_factor.second_factor = current_byte;
                    combination_factor.second_is_byte = true;
                    combination_factor.length = input_i + 1;
                }
            }

            if (current_edge == nullptr) {
                // Currently, we are exactly at a node
                if (auto it = current_node->edges.find(current_byte); it != current_node->edges.end()) {
                    // Edge exists
                    // The current byte got successfully read, therefore increase input_i
                    ++input_i;

                    CountedRadixTrieEdge& edge = it->second;

                    // Update truncation factor / second combination factor
                    // (We do this eagerly instead of waiting for the first mismatch,
                    // as there is no guarantee a mismatch will happen if the input ends early)
                    if (!finished_first_factor_node) {
                        truncation_factor = std::make_optional(TruncationFactor{edge.end_node.next_factor_node_index, input_i});
                    } else {
                        // Make sure to not overwrite single byte second factor
                        // with truncated factor, when truncated length is the same
                        if (input_i > combination_factor.length) {
                            combination_factor.second_factor = edge.end_node.next_factor_node_index;
                            combination_factor.second_is_byte = false;
                            combination_factor.length = input_i;
                            combination_factor.used_extra_truncation = true;
                        }
                    }

                    if (edge.rest_text.empty()) {
                        // Go directly to next node
                        current_node = &edge.end_node;

                        if (current_node->index != 0) {
                            // Update first combination factor
                            if (!finished_first_factor_node) {
                                combination_factor.first_factor = current_node->index;
                                combination_factor.first_is_byte = false;
                                combination_factor.length = input_i;
                            } else {
                                // This branch is only used to track extra truncations
                                combination_factor.used_extra_truncation = false;

                                // Avoid truncation with longer second combination factor and use this instead
                                // Useful for Flexible algorithms
                                if (input_i == usable_rest_input_len) {
                                    break;
                                }
                            }

                            // Update repetition factor
                            if (!finished_first_factor_node) {
                                if (!optimalRepetitionFactorFound) {
                                    if (const size_t repetition_len = input_i + naive_lce(entire_input, bytes_already_read, bytes_already_read + input_i);
                                        repetition_len > repetition_factor.total_length || ((repetition_len == repetition_factor.total_length || repetition_len >= usable_rest_input_len) && std::min(repetition_factor.total_length, usable_rest_input_len) % repetition_factor.factor_length != 0 && std::min(repetition_len, usable_rest_input_len) % input_i == 0)) {
                                        repetition_factor = RepetitionFactor{current_node->index, false, input_i, repetition_len};
                                        if (repetition_len >= usable_rest_input_len && usable_rest_input_len % input_i == 0) {
                                            optimalRepetitionFactorFound = true;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        // Update edge
                        current_edge = &edge;
                        edge_rest_text_iter = std::make_optional(edge.rest_text.begin());
                    }
                } else {
                    // Edge does not exist
                    // We were unable to read the byte, therefore do not increase input_i
                    if (!finished_first_factor_node) {
                        // Move on to second factor
                        finished_first_factor_node = true;
                        current_node = &previous_factors.root_node;
                        // Second factor begins after end of first factor
                        input_i = combination_factor.length;
                        continue;
                    } else {
                        // Second factor finished, break loop
                        break;
                    }
                }
            } else {
                // We have to iterate over edge text
                if (**edge_rest_text_iter == current_byte) {
                    // The current byte got successfully read, therefore increase input_i
                    ++input_i;

                    // Update truncation factor / second combination factor
                    // (We do this eagerly instead of waiting for the first mismatch,
                    // as there is no guarantee a mismatch will happen if the input ends early)
                    if (!finished_first_factor_node) {
                        truncation_factor = std::make_optional(TruncationFactor{current_edge->end_node.next_factor_node_index, input_i});
                    } else {
                        combination_factor.second_factor = current_edge->end_node.next_factor_node_index;
                        combination_factor.second_is_byte = false;
                        combination_factor.length = input_i;
                        combination_factor.used_extra_truncation = true;
                    }

                    // Move to next element in iterator
                    ++(*edge_rest_text_iter);
                    if (*edge_rest_text_iter == current_edge->rest_text.end()) {
                        // Reached end of iterator, go to next node
                        current_node = &current_edge->end_node;
                        current_edge = nullptr;
                        edge_rest_text_iter = std::nullopt;

                        if (current_node->index != 0) {
                            // Update first combination factor
                            if (!finished_first_factor_node) {
                                combination_factor.first_factor = current_node->index;
                                combination_factor.first_is_byte = false;
                                combination_factor.length = input_i;
                            } else {
                                // This branch is only used to track extra truncations
                                combination_factor.used_extra_truncation = false;

                                // Avoid truncation with longer second combination factor and use this instead
                                // Useful for Flexible algorithms
                                if (input_i == usable_rest_input_len) {
                                    break;
                                }
                            }

                            // Update repetition factor
                            if (!finished_first_factor_node) {
                                if (!optimalRepetitionFactorFound) {
                                    if (const size_t repetition_len = input_i + naive_lce(entire_input, bytes_already_read, bytes_already_read + input_i);
                                        repetition_len > repetition_factor.total_length || ((repetition_len == repetition_factor.total_length || repetition_len >= usable_rest_input_len) && std::min(repetition_factor.total_length, usable_rest_input_len) % repetition_factor.factor_length != 0 && std::min(repetition_len, usable_rest_input_len) % input_i == 0)) {
                                        repetition_factor = RepetitionFactor{current_node->index, false, input_i, repetition_len};
                                        if (repetition_len >= usable_rest_input_len && usable_rest_input_len % input_i == 0) {
                                            optimalRepetitionFactorFound = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    // We were unable to read the byte, therefore do not increase input_i
                    if (!finished_first_factor_node) {
                        // Move on to second factor
                        finished_first_factor_node = true;
                        current_node = &previous_factors.root_node;
                        current_edge = nullptr;
                        edge_rest_text_iter = std::nullopt;
                        // Second factor begins after end of first factor
                        input_i = combination_factor.length;
                        continue;
                    } else {
                        // Second factor finished, break loop
                        break;
                    }
                }
            }

            // Account for the case that we reached the last byte while searching for the
            // first combination factor
            if (!finished_first_factor_node && is_last_byte) {
                // Move on to second factor
                finished_first_factor_node = true;
                current_node = &previous_factors.root_node;
                current_edge = nullptr;
                edge_rest_text_iter = std::nullopt;
                // Second factor begins after end of first factor
                input_i = combination_factor.length;
            }
        }

        // Return the one that yields the maximum length
        // (where ties are broken such that combination is the preferred method
        // and repetition is preferred least)
        NextFactorResult longest_factor = combination_factor_to_result_trunc(rest_input, combination_factor, usable_rest_input_len);
        if (truncation_factor) {
            if (NextFactorResult truncation_result = truncation_factor_to_result_trunc(rest_input, *truncation_factor, usable_rest_input_len);
                truncation_result.factor_slice.size() > longest_factor.factor_slice.size()) {
                longest_factor = truncation_result;
            }
        }
        if (NextFactorResult repetition_result = repetition_factor_to_result_trunc(rest_input, repetition_factor, usable_rest_input_len);
            repetition_result.factor_slice.size() > longest_factor.factor_slice.size()) {
            longest_factor = repetition_result;
        }
        return longest_factor;
    }

    // Returns true if a new factor node got created or a splitting node got turned into a factor node, otherwise false.
    // In other words: true if did not already exist in radix trie, otherwise false.
    bool insert_into_radix_trie(RadixTrie &trie, RadixTrieNode *from_node, const Slice &insert) {
        // Current nodes
        RadixTrieNode* current_node = from_node;
        RadixTrieEdge* current_edge = nullptr;
        size_t edge_rest_text_index = 0;

        // Insert
        size_t input_i = 0;
        while (input_i < insert.size()) {
            uint8_t current_byte = insert[input_i];
            bool is_last_byte = input_i == insert.size() - 1;

            if (current_edge == nullptr) {
                // Currently, we are exactly at a node
                if (auto it = current_node->edges.find(current_byte); it != current_node->edges.end()) {
                    // Edge exists
                    // The current byte got successfully read, therefore increase input_i
                    ++input_i;

                    RadixTrieEdge& edge = it->second;
                    if (edge.rest_text.empty()) {
                        // Go directly to next node
                        current_node = &edge.end_node;

                        if (is_last_byte) {
                            // Convert splitting node to factor node if needed
                            if (current_node->index == 0) {
                                current_node->index = trie.num_factor_nodes;
                                current_node->next_factor_node_index = current_node->index;
                                trie.num_factor_nodes += 1;
                                return true;
                            }
                            // Already is a factor node, nothing to do
                            return false;
                        }
                    } else {
                        // Update edge
                        current_edge = &edge;

                        if (is_last_byte) {
                            // Since this is the last matching byte, the rest text needs to be split off
                            // and a new factor node needs to be inserted before it.
                            // Move the complete edge rest text into a temporary variable.
                            Slice old_edge_rest_text = current_edge->rest_text;
                            current_edge->rest_text = Slice::create_empty();

                            // Get first byte as old rest edge byte and remove from front
                            // This will always work since we know the rest text is not empty
                            uint8_t old_rest_edge_byte = old_edge_rest_text[0];
                            old_edge_rest_text = old_edge_rest_text.slice(1);

                            // Get edges from edge end node
                            std::unordered_map<uint8_t, RadixTrieEdge> temp_edges = std::move(current_edge->end_node.edges);

                            // Remove edges from end node (reset edges after move)
                            current_edge->end_node.edges.clear();

                            // Add new edge with rest of current edge text
                            RadixTrieNode copied_factor_node = current_edge->end_node.copy_without_edges();
                            copied_factor_node.edges = std::move(temp_edges); // reassign edges
                            RadixTrieEdge old_rest_edge = {std::move(copied_factor_node), old_edge_rest_text};
                            current_edge->end_node.edges.emplace(old_rest_edge_byte, std::move(old_rest_edge));

                            // Finally turn the edge end node into a new factor node
                            current_edge->end_node.index = trie.num_factor_nodes;
                            current_edge->end_node.next_factor_node_index = current_edge->end_node.index;
                            trie.num_factor_nodes += 1;
                            return true;
                        }

                        edge_rest_text_index = 0;
                    }
                } else {
                    // Edge does not exist, create new edge with current_byte
                    RadixTrieNode new_node = RadixTrieNode::create_factor_node(trie.num_factor_nodes);
                    RadixTrieEdge new_edge = {std::move(new_node), insert.slice(input_i + 1)};
                    current_node->edges.emplace(current_byte, std::move(new_edge));
                    trie.num_factor_nodes += 1;
                    return true;
                }
            } else {
                // We have to iterate over edge text
                if (current_edge->rest_text[edge_rest_text_index] == current_byte) {
                    // The current byte got successfully read, therefore increase input_i
                    ++input_i;

                    // Move to next index of edge rest text
                    ++edge_rest_text_index;
                    if (edge_rest_text_index == current_edge->rest_text.size()) {
                        // Reached end of edge rest text, go to next node
                        current_node = &current_edge->end_node;
                        current_edge = nullptr;

                        if (is_last_byte) {
                            // Convert splitting node to factor node if needed
                            if (current_node->index == 0) {
                                current_node->index = trie.num_factor_nodes;
                                current_node->next_factor_node_index = current_node->index;
                                trie.num_factor_nodes += 1;
                                return true;
                            }
                            // Already is a factor node, nothing to do
                            return false;
                        }
                    } else {
                        if (is_last_byte) {
                            // We read the last matching byte on an edge rest text.
                            //
                            // Use slice to move everything after the last matching byte
                            // until the end of the edge text into old_edge_rest_text.
                            //
                            // Note, that we have already moved to the next element beforehand,
                            // so the edge_rest_text_index points to one byte after the last matching byte right now.
                            Slice old_edge_rest_text = current_edge->rest_text.slice(edge_rest_text_index);
                            current_edge->rest_text = current_edge->rest_text.slice(0, edge_rest_text_index);

                            // Get first byte after last matching byte as old rest edge byte and remove from front
                            // This will always work since we know that we had not reached the end of the slice
                            uint8_t old_rest_edge_byte = old_edge_rest_text[0];
                            old_edge_rest_text = old_edge_rest_text.slice(1);

                            // Get edges from edge end node
                            std::unordered_map<uint8_t, RadixTrieEdge> temp_edges = std::move(current_edge->end_node.edges);

                            // Remove edges from end node (reset edges after move)
                            current_edge->end_node.edges.clear();

                            // Add new edge with rest of current edge text
                            RadixTrieNode copied_factor_node = current_edge->end_node.copy_without_edges();
                            copied_factor_node.edges = std::move(temp_edges); // reassign edges
                            RadixTrieEdge old_rest_edge = {std::move(copied_factor_node), old_edge_rest_text};
                            current_edge->end_node.edges.emplace(old_rest_edge_byte, std::move(old_rest_edge));

                            // Finally turn the edge end node into a new factor node
                            current_edge->end_node.index = trie.num_factor_nodes;
                            current_edge->end_node.next_factor_node_index = current_edge->end_node.index;
                            trie.num_factor_nodes += 1;
                            return true;
                        }
                    }
                } else {
                    // We were unable to read the byte, therefore do not increase input_i
                    // Use slice to move everything from the first mismatching byte until the end of the edge text into old_edge_rest_text
                    Slice old_edge_rest_text = current_edge->rest_text.slice(edge_rest_text_index);
                    current_edge->rest_text = current_edge->rest_text.slice(0, edge_rest_text_index);

                    // Get first mismatch byte as old rest edge byte and remove from front
                    uint8_t old_rest_edge_byte = old_edge_rest_text[0];
                    old_edge_rest_text = old_edge_rest_text.slice(1);

                    // Get edges from edge end node
                    std::unordered_map<uint8_t, RadixTrieEdge> temp_edges = std::move(current_edge->end_node.edges);

                    // Remove edges from end node (reset edges after move)
                    current_edge->end_node.edges.clear();

                    // Add new edge with rest of current edge text
                    RadixTrieNode copied_factor_node = current_edge->end_node.copy_without_edges();
                    copied_factor_node.edges = std::move(temp_edges); // reassign edges
                    RadixTrieEdge old_rest_edge = {std::move(copied_factor_node), old_edge_rest_text};
                    current_edge->end_node.edges.emplace(old_rest_edge_byte, std::move(old_rest_edge));

                    // Convert end node to splitting node after creating the copied_factor_node.
                    // This will always be a splitting node, since we always have to add the edge with the
                    // mismatched byte to it.
                    current_edge->end_node.index = 0;

                    // Add edge with rest of input
                    Slice rest_text = insert.slice(input_i + 1);
                    RadixTrieNode new_node = RadixTrieNode::create_factor_node(trie.num_factor_nodes);
                    RadixTrieEdge new_edge = {std::move(new_node), rest_text};
                    current_edge->end_node.edges.emplace(current_byte, std::move(new_edge));
                    trie.num_factor_nodes += 1;
                    return true;
                }
            }
        }

        if (std::addressof(trie.root_node) == current_node) {
            // We do not handle empty insertions on root node.
            throw std::runtime_error("Empty insertion unsupported");
        } else {
            // Convert splitting node to factor node if needed
            if (current_node->index == 0) {
                current_node->index = trie.num_factor_nodes;
                current_node->next_factor_node_index = current_node->index;
                trie.num_factor_nodes += 1;
                return true;
            }
            // Already is a factor node, nothing to do
            return false;
        }
    }
}

// Returns the number of factors
size_t lzdr_linear_time(const Slice input, const bool check_decompressed_equals_input) {
    RadixTrie previous_factors;
    std::vector<uint8_t> compressed_data;

    size_t num_factors = 0;
    size_t num_extra_truncations_combinations = 0;
    size_t num_extra_truncations_repetitions = 0;
    size_t i = 0;
    while (i < input.size()) {
        Slice rest_input = input.slice(i);
        NextFactorResult2 longest_factor = lzdr_linear_time_internal::next_longest_factor(
            input, i, rest_input, previous_factors);

        if (longest_factor.used_extra_truncation) {
            if (const uint8_t factor_type = longest_factor.compressed_data[0]; 0 <= factor_type && factor_type <= 3) {
                num_extra_truncations_combinations += 1;
            } else if (factor_type == 5 || factor_type == 6) {
                num_extra_truncations_repetitions += 1;
            } else {
                throw std::out_of_range("Extra truncation could not be associated with factor type");
            }
        }

        ++num_factors;

#ifndef NDEBUG
        std::cout << "Factor " << num_factors << ": " << longest_factor.factor_slice << std::endl;
#endif

        i += longest_factor.factor_slice.size();

        lzdr_linear_time_internal::insert_into_radix_trie(previous_factors, longest_factor.insertion_node, longest_factor.insertion_slice);

        if (check_decompressed_equals_input) {
            compressed_data.insert(compressed_data.end(),
                                   std::make_move_iterator(longest_factor.compressed_data.begin()),
                                   std::make_move_iterator(longest_factor.compressed_data.end()));
        }
    }

    if (check_decompressed_equals_input) {
        if (const std::vector<uint8_t> decompressed_data = lzdr_decompress(compressed_data);
            !(input == Slice(decompressed_data))) {
            throw std::out_of_range("Decompressed not equal to input");
        }
    }

    std::cout << "Num extra truncations (combination): " << num_extra_truncations_combinations << std::endl;
    std::cout << "Num extra truncations (repetition): " << num_extra_truncations_repetitions << std::endl;

    return num_factors;
}

std::vector<uint8_t> lzdr_decompress(const std::vector<uint8_t> &compressed) {
    std::vector<std::vector<uint8_t> > previous_factors;
    size_t i = 0;
    while (i < compressed.size()) {
        if (const uint8_t type = compressed[i]; type == 0) {
            if (i + 6 >= compressed.size()) {
                throw std::out_of_range("Index out of bounds");
            }
            std::vector factor = {compressed[i + 1], compressed[i + 2]};
            const uint32_t length = u32_from_bytes(compressed.data() + (i + 3));
            if (factor.size() > length) {
                while (factor.size() > length) {
                    factor.pop_back();
                }
            } else if (factor.size() < length) {
                size_t r = 0;
                const size_t mod_r = factor.size();
                while (factor.size() < length) {
                    factor.push_back(factor[r]);
                    r = (r + 1) % mod_r;
                }
            }
            assert(factor.size() == length);
            previous_factors.push_back(factor);
            i += 7;
        } else if (type == 1) {
            if (i + 9 >= compressed.size()) {
                throw std::out_of_range("Index out of bounds");
            }
            std::vector factor = {compressed[i + 1]};
            if (const uint32_t second_factor = u32_from_bytes(compressed.data() + (i + 2)); second_factor != 0) {
                for (const uint8_t byte: previous_factors[second_factor - 1]) {
                    factor.push_back(byte);
                }
            }
            const uint32_t length = u32_from_bytes(compressed.data() + (i + 6));
            if (factor.size() > length) {
                while (factor.size() > length) {
                    factor.pop_back();
                }
            } else if (factor.size() < length) {
                size_t r = 0;
                const size_t mod_r = factor.size();
                while (factor.size() < length) {
                    factor.push_back(factor[r]);
                    r = (r + 1) % mod_r;
                }
            }
            assert(factor.size() == length);
            previous_factors.push_back(factor);
            i += 10;
        } else if (type == 2) {
            if (i + 9 >= compressed.size()) {
                throw std::out_of_range("Index out of bounds");
            }
            std::vector<uint8_t> factor;
            const uint32_t first_factor = u32_from_bytes(compressed.data() + (i + 1));
            for (const uint8_t byte: previous_factors[first_factor - 1]) {
                factor.push_back(byte);
            }
            factor.push_back(compressed[i + 5]);
            const uint32_t length = u32_from_bytes(compressed.data() + (i + 6));
            if (factor.size() > length) {
                while (factor.size() > length) {
                    factor.pop_back();
                }
            } else if (factor.size() < length) {
                size_t r = 0;
                const size_t mod_r = factor.size();
                while (factor.size() < length) {
                    factor.push_back(factor[r]);
                    r = (r + 1) % mod_r;
                }
            }
            assert(factor.size() == length);
            previous_factors.push_back(factor);
            i += 10;
        } else if (type == 3) {
            if (i + 12 >= compressed.size()) {
                throw std::out_of_range("Index out of bounds");
            }
            std::vector<uint8_t> factor;
            const uint32_t first_factor = u32_from_bytes(compressed.data() + (i + 1));
            for (const uint8_t byte: previous_factors[first_factor - 1]) {
                factor.push_back(byte);
            }
            if (const uint32_t second_factor = u32_from_bytes(compressed.data() + (i + 5)); second_factor != 0) {
                for (const uint8_t byte: previous_factors[second_factor - 1]) {
                    factor.push_back(byte);
                }
            }
            const uint32_t length = u32_from_bytes(compressed.data() + (i + 9));
            if (factor.size() > length) {
                while (factor.size() > length) {
                    factor.pop_back();
                }
            } else if (factor.size() < length) {
                size_t r = 0;
                const size_t mod_r = factor.size();
                while (factor.size() < length) {
                    factor.push_back(factor[r]);
                    r = (r + 1) % mod_r;
                }
            }
            assert(factor.size() == length);
            previous_factors.push_back(factor);
            i += 13;
        } else if (type == 4) {
            if (i + 8 >= compressed.size()) {
                throw std::out_of_range("Index out of bounds");
            }
            std::vector<uint8_t> factor;
            const std::vector<uint8_t> &first_factor = previous_factors[
                u32_from_bytes(compressed.data() + (i + 1)) - 1];
            const uint32_t length = u32_from_bytes(compressed.data() + (i + 5));
            for (size_t j = 0; j < length; ++j) {
                factor.push_back(first_factor[j]);
            }
            previous_factors.push_back(factor);
            i += 9;
        } else if (type == 5) {
            if (i + 8 >= compressed.size()) {
                throw std::out_of_range("Index out of bounds");
            }
            std::vector<uint8_t> factor;
            const std::vector<uint8_t> &first_factor = previous_factors[
                u32_from_bytes(compressed.data() + (i + 1)) - 1];
            const uint32_t total_length = u32_from_bytes(compressed.data() + (i + 5));
            const size_t repetitions = total_length / first_factor.size();
            const size_t extra_chars = total_length % first_factor.size();
            for (size_t j = 0; j < repetitions; ++j) {
                for (const uint8_t byte: first_factor) {
                    factor.push_back(byte);
                }
            }
            assert(first_factor.size() >= extra_chars);
            for (size_t j = 0; j < extra_chars; ++j) {
                factor.push_back(first_factor[j]);
            }
            assert(factor.size() == total_length);
            previous_factors.push_back(factor);
            i += 9;
        } else if (type == 6) {
            if (i + 5 >= compressed.size()) {
                throw std::out_of_range("Index out of bounds");
            }
            std::vector<uint8_t> factor;
            const uint32_t total_length = u32_from_bytes(compressed.data() + (i + 2));
            for (size_t j = 0; j < total_length; ++j) {
                factor.push_back(compressed[i + 1]);
            }
            assert(factor.size() == total_length);
            previous_factors.push_back(factor);
            i += 6;
        } else {
            throw std::out_of_range("Unknown type");
        }
    }

    std::vector<uint8_t> decompressed;
    for (const std::vector<uint8_t> &factor: previous_factors) {
        decompressed.insert(decompressed.end(), factor.begin(), factor.end());
    }
    return decompressed;
}
