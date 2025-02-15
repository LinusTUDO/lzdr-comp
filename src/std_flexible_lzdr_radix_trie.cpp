#include "std_flexible_lzdr_radix_trie.h"
#include "lzdr_linear_time.h"
#include "compressor.h"
#include "slice.h"
#include "radix_trie.h"

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

namespace std_flexible_lzdr_radix_trie_internal {
    // Returns true if a new factor node got created or a splitting node got turned into a factor node, otherwise false.
    // In other words: true if did not already exist in radix trie, otherwise false.
    // If false, the count of the node will be increased by one.
    bool insert_into_radix_trie(CountedRadixTrie &trie, const Slice &insert) {
        // Current nodes
        CountedRadixTrieNode* current_node = &trie.root_node;
        CountedRadixTrieEdge* current_edge = nullptr;
        std::optional<std::list<uint8_t>::iterator> edge_rest_text_iter = std::nullopt;

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

                    CountedRadixTrieEdge& edge = it->second;
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
                            // Already is a factor node, increase extra count
                            current_node->extra_count += 1;
                            trie.num_factor_nodes += 1;
                            return false;
                        }
                    } else {
                        // Update edge
                        current_edge = &edge;

                        if (is_last_byte) {
                            // Since this is the last matching byte, the rest text needs to be split off
                            // and a new factor node needs to be inserted before it
                            std::list<uint8_t> old_edge_rest_text;

                            // Move the complete edge rest text into a temporary list
                            old_edge_rest_text.splice(old_edge_rest_text.begin(), current_edge->rest_text);

                            // Get first byte as old rest edge byte and remove from front
                            // This will always work since we know the rest text is not empty
                            uint8_t old_rest_edge_byte = old_edge_rest_text.front();
                            old_edge_rest_text.pop_front();

                            // Get edges from edge end node
                            std::unordered_map<uint8_t, CountedRadixTrieEdge> temp_edges = std::move(current_edge->end_node.edges);

                            // Remove edges from end node (reset edges after move)
                            current_edge->end_node.edges.clear();

                            // Add new edge with rest of current edge text
                            CountedRadixTrieNode copied_factor_node = current_edge->end_node.copy_without_edges();
                            copied_factor_node.edges = std::move(temp_edges); // reassign edges
                            CountedRadixTrieEdge old_rest_edge = {std::move(copied_factor_node), std::move(old_edge_rest_text)};
                            current_edge->end_node.edges.emplace(old_rest_edge_byte, std::move(old_rest_edge));

                            // Finally turn the edge end node into a new factor node
                            current_edge->end_node.index = trie.num_factor_nodes;
                            current_edge->end_node.next_factor_node_index = current_edge->end_node.index;
                            current_edge->end_node.extra_count = 0;
                            trie.num_factor_nodes += 1;
                            return true;
                        }

                        edge_rest_text_iter = std::make_optional(edge.rest_text.begin());
                    }
                } else {
                    // Edge does not exist, create new edge with current_byte
                    std::list<uint8_t> rest_text;
                    for (size_t j = input_i + 1; j < insert.size(); ++j) {
                        rest_text.push_back(insert[j]);
                    }
                    CountedRadixTrieNode new_node = CountedRadixTrieNode::create_factor_node(trie.num_factor_nodes);
                    CountedRadixTrieEdge new_edge = {std::move(new_node), std::move(rest_text)};
                    current_node->edges.emplace(current_byte, std::move(new_edge));
                    trie.num_factor_nodes += 1;
                    return true;
                }
            } else {
                // We have to iterate over edge text
                if (**edge_rest_text_iter == current_byte) {
                    // The current byte got successfully read, therefore increase input_i
                    ++input_i;

                    // Move to next element in iterator
                    ++(*edge_rest_text_iter);
                    if (*edge_rest_text_iter == current_edge->rest_text.end()) {
                        // Reached end of iterator, go to next node
                        current_node = &current_edge->end_node;
                        current_edge = nullptr;
                        edge_rest_text_iter = std::nullopt;

                        if (is_last_byte) {
                            // Convert splitting node to factor node if needed
                            if (current_node->index == 0) {
                                current_node->index = trie.num_factor_nodes;
                                current_node->next_factor_node_index = current_node->index;
                                trie.num_factor_nodes += 1;
                                return true;
                            }
                            // Already is a factor node, increase extra count
                            current_node->extra_count += 1;
                            trie.num_factor_nodes += 1;
                            return false;
                        }
                    } else {
                        if (is_last_byte) {
                            // We read the last matching byte on an edge rest text.
                            //
                            // Use splice to move everything after the last matching byte
                            // until the end of the edge text into old_edge_rest_text.
                            //
                            // Note, that we have already moved to the next element beforehand,
                            // so the iterator points to one byte after the last matching byte right now.
                            std::list<uint8_t> old_edge_rest_text;
                            old_edge_rest_text.splice(old_edge_rest_text.begin(), current_edge->rest_text, *edge_rest_text_iter, current_edge->rest_text.end());

                            // Get first byte after last matching byte as old rest edge byte and remove from front
                            // This will always work since we know that we had not reached the end of the iterator
                            uint8_t old_rest_edge_byte = old_edge_rest_text.front();
                            old_edge_rest_text.pop_front();

                            // Get edges from edge end node
                            std::unordered_map<uint8_t, CountedRadixTrieEdge> temp_edges = std::move(current_edge->end_node.edges);

                            // Remove edges from end node (reset edges after move)
                            current_edge->end_node.edges.clear();

                            // Add new edge with rest of current edge text
                            CountedRadixTrieNode copied_factor_node = current_edge->end_node.copy_without_edges();
                            copied_factor_node.edges = std::move(temp_edges); // reassign edges
                            CountedRadixTrieEdge old_rest_edge = {std::move(copied_factor_node), std::move(old_edge_rest_text)};
                            current_edge->end_node.edges.emplace(old_rest_edge_byte, std::move(old_rest_edge));

                            // Finally turn the edge end node into a new factor node
                            current_edge->end_node.index = trie.num_factor_nodes;
                            current_edge->end_node.next_factor_node_index = current_edge->end_node.index;
                            current_edge->end_node.extra_count = 0;
                            trie.num_factor_nodes += 1;
                            return true;
                        }
                    }
                } else {
                    // We were unable to read the byte, therefore do not increase input_i
                    // Use splice to move everything from the first mismatching byte until the end of the edge text into old_edge_rest_text
                    std::list<uint8_t> old_edge_rest_text;
                    old_edge_rest_text.splice(old_edge_rest_text.begin(), current_edge->rest_text, *edge_rest_text_iter, current_edge->rest_text.end());

                    // Get first mismatch byte as old rest edge byte and remove from front
                    uint8_t old_rest_edge_byte = old_edge_rest_text.front();
                    old_edge_rest_text.pop_front();

                    // Get edges from edge end node
                    std::unordered_map<uint8_t, CountedRadixTrieEdge> temp_edges = std::move(current_edge->end_node.edges);

                    // Remove edges from end node (reset edges after move)
                    current_edge->end_node.edges.clear();

                    // Add new edge with rest of current edge text
                    CountedRadixTrieNode copied_factor_node = current_edge->end_node.copy_without_edges();
                    copied_factor_node.edges = std::move(temp_edges); // reassign edges
                    CountedRadixTrieEdge old_rest_edge = {std::move(copied_factor_node), std::move(old_edge_rest_text)};
                    current_edge->end_node.edges.emplace(old_rest_edge_byte, std::move(old_rest_edge));

                    // Convert end node to splitting node after creating the copied_factor_node.
                    // This will always be a splitting node, since we always have to add the edge with the
                    // mismatched byte to it.
                    current_edge->end_node.index = 0;
                    current_edge->end_node.extra_count = 0;

                    // Add edge with rest of input
                    std::list<uint8_t> rest_text;
                    for (size_t j = input_i + 1; j < insert.size(); ++j) {
                        rest_text.push_back(insert[j]);
                    }
                    CountedRadixTrieNode new_node = CountedRadixTrieNode::create_factor_node(trie.num_factor_nodes);
                    CountedRadixTrieEdge new_edge = {std::move(new_node), std::move(rest_text)};
                    current_edge->end_node.edges.emplace(current_byte, std::move(new_edge));
                    trie.num_factor_nodes += 1;
                    return true;
                }
            }
        }

        // We do not handle empty insertions.
        throw std::runtime_error("Empty insertion unsupported");
    }

    // Returns true if a factor node got deleted, converted to a splitting node,
    // or the count of the factor node has been decreased by one, otherwise false.
    bool remove_from_radix_trie(CountedRadixTrie &trie, const Slice &remove) {
        // Current nodes
        CountedRadixTrieNode* current_node = &trie.root_node;
        CountedRadixTrieEdge* current_edge = nullptr;
        std::optional<std::list<uint8_t>::iterator> edge_rest_text_iter = std::nullopt;
        CountedRadixTrieNode* previous_node = nullptr;
        std::optional<uint8_t> previous_edge_byte = std::nullopt;

        // Insert
        size_t input_i = 0;
        while (input_i < remove.size()) {
            uint8_t current_byte = remove[input_i];
            const bool is_last_byte = input_i == remove.size() - 1;

            if (current_edge == nullptr) {
                // Currently, we are exactly at a node
                if (auto it = current_node->edges.find(current_byte); it != current_node->edges.end()) {
                    // Edge exists
                    // The current byte got successfully read, therefore increase input_i
                    ++input_i;

                    CountedRadixTrieEdge& edge = it->second;
                    if (edge.rest_text.empty()) {
                        // Go directly to next node
                        previous_node = current_node;
                        previous_edge_byte = std::make_optional(current_byte);
                        current_node = &edge.end_node;

                        if (is_last_byte) {
                            if (current_node->index == 0) {
                                // Splitting node is not a factor node
                                return false;
                            }

                            if (current_node->extra_count == 0) {
                                // Time to remove factor node
                                if (current_node->edges.size() > 1) {
                                    // Convert factor node to splitting node
                                    current_node->index = 0;
                                    current_node->next_factor_node_index = current_node->edges.begin()->second.end_node.next_factor_node_index;
                                    trie.num_factor_nodes -= 1;
                                } else if (current_node->edges.size() == 1) {
                                    auto next_edge = *current_node->edges.begin();
                                    edge.rest_text.push_back(next_edge.first);
                                    edge.rest_text.splice(edge.rest_text.end(), next_edge.second.rest_text);
                                    edge.end_node = next_edge.second.end_node;
                                    trie.num_factor_nodes -= 1;
                                } else {
                                    // Delete factor node
                                    const bool deleted = previous_node->edges.erase(*previous_edge_byte);
                                    assert(deleted);
                                    trie.num_factor_nodes -= 1;
                                }
                            } else {
                                current_node->extra_count -= 1;
                                trie.num_factor_nodes -= 1;
                            }
                            return true;
                        }
                    } else {
                        if (is_last_byte) {
                            // We end exactly on the first byte of the edge, so not a factor node
                            return false;
                        }

                        // Update edge
                        current_edge = &edge;
                        edge_rest_text_iter = std::make_optional(edge.rest_text.begin());
                        previous_edge_byte = std::make_optional(current_byte);
                    }
                } else {
                    // Edge does not exist
                    // There was no factor node that matched the whole text
                    return false;
                }
            } else {
                // We have to iterate over edge text
                if (**edge_rest_text_iter == current_byte) {
                    // The current byte got successfully read, therefore increase input_i
                    ++input_i;

                    // Move to next element in iterator
                    ++(*edge_rest_text_iter);
                    if (*edge_rest_text_iter == current_edge->rest_text.end()) {
                        // Reached end of iterator, go to next node
                        previous_node = current_node;
                        current_node = &current_edge->end_node;
                        CountedRadixTrieEdge* previous_edge = current_edge;
                        current_edge = nullptr;
                        edge_rest_text_iter = std::nullopt;

                        if (is_last_byte) {
                            if (current_node->index == 0) {
                                // Splitting node is not a factor node
                                return false;
                            }

                            if (current_node->extra_count == 0) {
                                // Time to remove factor node
                                if (current_node->edges.size() > 1) {
                                    // Convert factor node to splitting node
                                    current_node->index = 0;
                                    current_node->next_factor_node_index = current_node->edges.begin()->second.end_node.next_factor_node_index;
                                    trie.num_factor_nodes -= 1;
                                } else if (current_node->edges.size() == 1) {
                                    auto next_edge = *current_node->edges.begin();
                                    previous_edge->rest_text.push_back(next_edge.first);
                                    previous_edge->rest_text.splice(previous_edge->rest_text.end(), next_edge.second.rest_text);
                                    previous_edge->end_node = next_edge.second.end_node;
                                    trie.num_factor_nodes -= 1;
                                } else {
                                    // Delete factor node
                                    const bool deleted = previous_node->edges.erase(*previous_edge_byte);
                                    assert(deleted);
                                    trie.num_factor_nodes -= 1;
                                }
                            } else {
                                current_node->extra_count -= 1;
                                trie.num_factor_nodes -= 1;
                            }
                            return true;
                        }
                    } else {
                        if (is_last_byte) {
                            // There was no factor node that matched the whole text
                            return false;
                        }
                    }
                } else {
                    // There was no factor node that matched the whole text
                    return false;
                }
            }
        }

        // We do not handle empty removals.
        throw std::runtime_error("Empty removal unsupported");
    }
}

// Returns the number of factors
size_t std_flexible_lzdr_radix_trie(const Slice input, const bool check_decompressed_equals_input) {
    // LZDR factors: pairs of factor end position and factor slice
    // The end position is inclusive, that means, if input[i] is the last character of the factor,
    // then `i` is the end position.
    std::vector<std::pair<size_t, Slice> > lzdr_factors_with_end_position;
    std::vector<uint8_t> compressed_data;
    std::vector<uint8_t> compressed_best_factor;
    size_t i = 0;
    size_t factor_count = 0;

    // Make sure RadixTrie previous_factors gets deallocated at end of block
    {
        RadixTrie previous_factors;
        while (i < input.size()) {
            Slice rest_input = input.slice(i);
            NextFactorResult2 longest_factor = lzdr_linear_time_internal::next_longest_factor(
                input, i, rest_input, previous_factors);

            ++factor_count;

#ifndef NDEBUG
            std::cout << "Factor " << factor_count << ": " << longest_factor.factor_slice << std::endl;
#endif

            // We add this to the vector before increasing `i`
            lzdr_factors_with_end_position.emplace_back(
                i + longest_factor.factor_slice.size() - 1, longest_factor.factor_slice);

            i += longest_factor.factor_slice.size();
            lzdr_linear_time_internal::insert_into_radix_trie(previous_factors, longest_factor.insertion_node, longest_factor.insertion_slice);

            if (check_decompressed_equals_input) {
                compressed_data.insert(compressed_data.end(),
                                       std::make_move_iterator(longest_factor.compressed_data.begin()),
                                       std::make_move_iterator(longest_factor.compressed_data.end()));
            }
        }
    }

    // Reset
    i = 0;
    factor_count = 0;

    // Now the actual standard flexible LZDR
    CountedRadixTrie previous_factors;
    std::vector<Slice> temp_added_factors;
    size_t temp_factor_count = 0;
    size_t num_extra_truncations_combinations = 0;
    size_t num_extra_truncations_repetitions = 0;
    while (i < input.size()) {
        Slice rest_input = input.slice(i);
#ifndef NDEBUG
        std::cout << "Rest input: " << rest_input << std::endl;
#endif

        // Add all factors that end before i, and do this before calculating `normal_longest_factor`
        // Minus 1, because we do not want to count the root node.
        while (temp_factor_count < lzdr_factors_with_end_position.size()) {
            std::pair<size_t, Slice> next_previous_factor = lzdr_factors_with_end_position[temp_factor_count];
            if (next_previous_factor.first < i) {
#ifndef NDEBUG
                std::cout << "New available factor: " << next_previous_factor.second << std::endl;
#endif
                std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(previous_factors, next_previous_factor.second);
                ++temp_factor_count;
            } else {
                break;
            }
        }

        // Go through all possible factors between length 1 and |normal_longest_factor|
        Slice normal_longest_factor = lzdr_linear_time_internal::next_longest_factor_counted_trie(
                input, i, rest_input, rest_input.size(), previous_factors).factor_slice;
        size_t best_factor_length = 0;
        size_t best_total_length = 0;
        bool used_extra_truncation = false;
        for (size_t l = normal_longest_factor.size(); l >= 1; --l) {
            Slice truncated_rest_input = rest_input.slice(0, l);
#ifndef NDEBUG
            std::cout << "  Testing factor: " << truncated_rest_input << std::endl;
#endif
            NextFactorResult next_factor_result = lzdr_linear_time_internal::next_longest_factor_counted_trie(
                input, i, rest_input, truncated_rest_input.size(), previous_factors);
            Slice next_factor = next_factor_result.factor_slice;

            // If this length cannot be represented by a factor, throw error
            if (truncated_rest_input.size() != next_factor.size()) {
                throw std::runtime_error("Cannot be represented by a factor");
            }

            // Add all factors that end before the next factor starts at `i + next_factor.size()`
            // Minus 1, because we do not want to count the root node.
            temp_added_factors.clear();
            while (temp_factor_count < lzdr_factors_with_end_position.size()) {
                std::pair<size_t, Slice> next_previous_factor = lzdr_factors_with_end_position[temp_factor_count];
                if (next_previous_factor.first < i + next_factor.size()) {
                    std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(previous_factors, next_previous_factor.second);
                    temp_added_factors.push_back(next_previous_factor.second);
                    ++temp_factor_count;
#ifndef NDEBUG
                    std::cout << "    -> Temporarily available factor: " << next_previous_factor.second << std::endl;
#endif
                } else {
                    break;
                }
            }

            Slice next_rest_input = rest_input.slice(next_factor.size());
            size_t current_total_length = next_factor.size();
            if (next_rest_input.empty()) {
#ifndef NDEBUG
                std::cout << "    -> Next rest: " << std::endl;
                std::cout << "    -> No next factor" <<
                        " (total length: " << current_total_length << ")" << std::endl;
#endif
            } else {
                Slice next_next_factor = lzdr_linear_time_internal::next_longest_factor_counted_trie(
                    input, i + next_factor.size(), next_rest_input, next_rest_input.size(), previous_factors).factor_slice;
                current_total_length += next_next_factor.size();
#ifndef NDEBUG
                std::cout << "    -> Next rest: " << next_rest_input << std::endl;
                std::cout << "    -> Next factor: " << next_next_factor <<
                        " (total length: " << current_total_length << ")" << std::endl;
#endif
            }

            if (best_factor_length == 0 || current_total_length > best_total_length) {
                best_factor_length = next_factor.size();
                best_total_length = current_total_length;
                compressed_best_factor = next_factor_result.compressed_data;
                used_extra_truncation = next_factor_result.used_extra_truncation;
            }

            // Remove factors that were temporarily added in reverse order
            for (auto it = temp_added_factors.rbegin(); it != temp_added_factors.rend(); ++it) {
                const Slice& slice = *it;
                std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(previous_factors, slice);
                --temp_factor_count;
            }
        }

        assert(best_factor_length > 0 && "Factor length must be greater than 0");

        Slice longest_factor = rest_input.slice(0, best_factor_length);
        ++factor_count;
#ifndef NDEBUG
        std::cout << "Factor " << factor_count << ": " << longest_factor << std::endl;
#endif

        if (used_extra_truncation) {
            if (const uint8_t factor_type = compressed_best_factor[0]; 0 <= factor_type && factor_type <= 3) {
                num_extra_truncations_combinations += 1;
            } else if (factor_type == 5 || factor_type == 6) {
                num_extra_truncations_repetitions += 1;
            } else {
                throw std::out_of_range("Extra truncation could not be associated with factor type");
            }
        }

        i += longest_factor.size();

        if (check_decompressed_equals_input) {
            compressed_data.insert(compressed_data.end(),
                                   std::make_move_iterator(compressed_best_factor.begin()),
                                   std::make_move_iterator(compressed_best_factor.end()));
        }
    }

    if (check_decompressed_equals_input) {
        if (const std::vector<uint8_t> decompressed_data = lzdr_decompress(compressed_data);
            decompressed_data.size() % 2 != 0 || !(input == Slice(decompressed_data).slice(decompressed_data.size() / 2))) {
            throw std::out_of_range("Decompressed not equal to input");
        }
    }

    std::cout << "Num extra truncations (combination): " << num_extra_truncations_combinations << std::endl;
    std::cout << "Num extra truncations (repetition): " << num_extra_truncations_repetitions << std::endl;

    return factor_count;
}
