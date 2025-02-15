#include "lzd_radix_tree.h"
#include "lzdr_linear_time.h"
#include "slice.h"
#include "radix_trie.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace lzd_radix_tree_internal {
    struct CombinationFactor {
        size_t first_factor;
        size_t second_factor;
        bool first_is_byte;
        bool second_is_byte;
        size_t length;
    };

    NextFactorResult combination_factor_to_result(const Slice &rest_input, const CombinationFactor &factor) {
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
        return NextFactorResult{rest_input.slice(0, factor.length), std::move(compressor.data()), false};
    }

    // This method requires rest_input to be not empty!
    NextFactorResult next_longest_factor(const Slice &rest_input, RadixTrie &previous_factors) {
        // Current nodes
        RadixTrieNode* current_node = &previous_factors.root_node;
        RadixTrieEdge* current_edge = nullptr;
        bool finished_first_factor_node = false;
        size_t edge_rest_text_index = 0;

        // Factors
        // Note: The combination factor is initialized with second_factor=0 and second_is_byte=false
        //       so that the second factor is empty by default in case only one factor is needed for the input.
        //       The same is done for the first factor to check if it has been initialized,
        //       that fact will later be used in the while loop.
        CombinationFactor combination_factor = {0, 0, false, false, 0};

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

                    if (edge.rest_text.empty()) {
                        // Go directly to next node
                        current_node = &edge.end_node;

                        if (current_node->index != 0) {
                            // Update combination factor
                            if (!finished_first_factor_node) {
                                combination_factor.first_factor = current_node->index;
                                combination_factor.first_is_byte = false;
                            } else {
                                combination_factor.second_factor = current_node->index;
                                combination_factor.second_is_byte = false;
                            }
                            combination_factor.length = input_i;
                        }
                    } else {
                        // Update edge
                        current_edge = &edge;
                        edge_rest_text_index = 0;
                    }
                } else {
                    // Edge does not exist
                    if (current_node == std::addressof(previous_factors.root_node)) {
                        // Take into account that the combination factor allows for single byte not yet existing
                        // So we basically read the current byte, even if it does not exist
                        ++input_i;

                        // The combination factor has already been updated at the start of the loop
                        // for this case, so we do not need to update it again.

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
                    } else {
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

                        if (current_node->index != 0) {
                            // Update combination factor
                            if (!finished_first_factor_node) {
                                combination_factor.first_factor = current_node->index;
                                combination_factor.first_is_byte = false;
                            } else {
                                combination_factor.second_factor = current_node->index;
                                combination_factor.second_is_byte = false;
                            }
                            combination_factor.length = input_i;
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

        return combination_factor_to_result(rest_input, combination_factor);
    }
}

// Returns the number of factors
size_t lzd_radix_tree(const Slice input, const bool check_decompressed_equals_input) {
    RadixTrie previous_factors;
    std::vector<uint8_t> compressed_data;

    size_t num_factors = 0;
    size_t i = 0;
    while (i < input.size()) {
        Slice rest_input = input.slice(i);
        NextFactorResult longest_factor = lzd_radix_tree_internal::next_longest_factor(rest_input, previous_factors);

        ++num_factors;

#ifndef NDEBUG
        std::cout << "Factor " << num_factors << ": " << longest_factor.factor_slice << std::endl;
#endif

        i += longest_factor.factor_slice.size();

        lzdr_linear_time_internal::insert_into_radix_trie(previous_factors, &previous_factors.root_node, longest_factor.factor_slice);

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

    return num_factors;
}
