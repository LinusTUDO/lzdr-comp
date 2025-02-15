#include "flexible_lzdr_max_radix_trie.h"
#include "lzdr_linear_time.h"
#include "std_flexible_lzdr_radix_trie.h"
#include "compressor.h"
#include "slice.h"
#include "radix_trie.h"

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

// Returns the number of factors
size_t flexible_lzdr_max_radix_trie(const Slice input) {
    // LZDR factors: pairs of factor end position and factor slice
    // The end position is inclusive, that means, if input[i] is the last character of the factor,
    // then `i` is the end position.
    std::vector<std::pair<size_t, Slice> > lzdr_factors_with_end_position;
    CountedRadixTrie previous_factors;
    std::vector<Slice> temp_added_factors;
    std::vector<uint8_t> compressed_best_factor;

    size_t factor_count = 0;
    size_t num_extra_truncations_combinations = 0;
    size_t num_extra_truncations_repetitions = 0;
    size_t i = 0;
    while (i < input.size()) {
        Slice rest_input = input.slice(i);
#ifndef NDEBUG
        std::cout << "Rest input: " << rest_input << std::endl;
#endif

        // Add all new factors that end before i, and do this before calculating `normal_longest_factor`
        for (auto it = lzdr_factors_with_end_position.begin(); it != lzdr_factors_with_end_position.end(); ) {
            std::pair<size_t, Slice> next_previous_factor = *it;
            if (next_previous_factor.first < i) {
#ifndef NDEBUG
                std::cout << "New available factor: " << next_previous_factor.second << std::endl;
#endif
                std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(previous_factors, next_previous_factor.second);
                it = lzdr_factors_with_end_position.erase(it); // erase returns the iterator to the next element
            } else {
                ++it; // Only increment if no element is removed
            }
        }

        Slice normal_longest_factor = lzdr_linear_time_internal::next_longest_factor_counted_trie(
            input, i, rest_input, rest_input.size(), previous_factors).factor_slice;

        // We add the maximum factor to the temporary vector before increasing `i`
        // and before checking the flexible factors
        lzdr_factors_with_end_position.emplace_back(i + normal_longest_factor.size() - 1, normal_longest_factor);

        // Go through all possible factors between length 1 and |normal_longest_factor|
        size_t best_factor_length = 0;
        size_t best_total_length = 0;
        bool used_extra_truncation = false;
        for (size_t l = normal_longest_factor.size(); l >= 1; --l) {
            Slice truncated_rest_input = rest_input.slice(0, l);
#ifndef NDEBUG
            std::cout << "  Testing factor: " << truncated_rest_input << std::endl;
#endif
            NextFactorResult next_factor = lzdr_linear_time_internal::next_longest_factor_counted_trie(
                input, i, rest_input, truncated_rest_input.size(), previous_factors);

            // If this length cannot be represented by a factor, throw error
            if (truncated_rest_input.size() != next_factor.factor_slice.size()) {
                throw std::runtime_error("Cannot be represented by a factor");
            }

            // Add all factors that end before the next factor starts at `i + next_factor.size()`
            temp_added_factors.clear();
            for (std::pair next_previous_factor : lzdr_factors_with_end_position) {
                if (next_previous_factor.first < i + next_factor.factor_slice.size()) {
                    std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(previous_factors, next_previous_factor.second);
                    temp_added_factors.push_back(next_previous_factor.second);
#ifndef NDEBUG
                    std::cout << "    -> Temporarily available factor: " << next_previous_factor.second << std::endl;
#endif
                }
            }

            Slice next_rest_input = rest_input.slice(next_factor.factor_slice.size());
            size_t current_total_length = next_factor.factor_slice.size();
            if (next_rest_input.empty()) {
#ifndef NDEBUG
                std::cout << "    -> Next rest: " << std::endl;
                std::cout << "    -> No next factor" <<
                        " (total length: " << current_total_length << ")" << std::endl;
#endif
            } else {
                Slice next_next_factor = lzdr_linear_time_internal::next_longest_factor_counted_trie(
                    input, i + next_factor.factor_slice.size(), next_rest_input, next_rest_input.size(), previous_factors).factor_slice;
                current_total_length += next_next_factor.size();
#ifndef NDEBUG
                std::cout << "    -> Next rest: " << next_rest_input << std::endl;
                std::cout << "    -> Next factor: " << next_next_factor <<
                        " (total length: " << current_total_length << ")" << std::endl;
#endif
            }

            if (best_factor_length == 0 || current_total_length > best_total_length) {
                best_factor_length = next_factor.factor_slice.size();
                best_total_length = current_total_length;
                compressed_best_factor = next_factor.compressed_data;
                used_extra_truncation = next_factor.used_extra_truncation;
            }

            // Remove factors that were temporarily added in reverse order
            for (auto it = temp_added_factors.rbegin(); it != temp_added_factors.rend(); ++it) {
                const Slice& slice = *it;
                std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(previous_factors, slice);
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
    }

    std::cout << "Num extra truncations (combination): " << num_extra_truncations_combinations << std::endl;
    std::cout << "Num extra truncations (repetition): " << num_extra_truncations_repetitions << std::endl;

    return factor_count;
}
