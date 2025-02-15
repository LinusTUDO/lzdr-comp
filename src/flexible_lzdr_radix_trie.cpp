#include "flexible_lzdr_radix_trie.h"
#include "lzdr_linear_time.h"
#include "std_flexible_lzdr_radix_trie.h"
#include "compressor.h"
#include "slice.h"
#include "radix_trie.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

// Returns the number of factors
size_t flexible_lzdr_radix_trie(const Slice input, const bool check_decompressed_equals_input) {
    CountedRadixTrie previous_factors;
    std::vector<uint8_t> compressed_data;
    std::vector<uint8_t> compressed_best_factor;

    size_t num_extra_truncations_combinations = 0;
    size_t num_extra_truncations_repetitions = 0;
    size_t i = 0;
    size_t factor_count = 0;
    while (i < input.size()) {
        Slice rest_input = input.slice(i);
        Slice normal_longest_factor = lzdr_linear_time_internal::next_longest_factor_counted_trie(
                input, i, rest_input, rest_input.size(), previous_factors).factor_slice;
#ifndef NDEBUG
        std::cout << "Rest input: " << rest_input << std::endl;
#endif

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

            std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(previous_factors, next_factor.factor_slice);

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

            std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(previous_factors, next_factor.factor_slice);
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
        std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(previous_factors, longest_factor);

        if (check_decompressed_equals_input) {
            compressed_data.insert(compressed_data.end(),
                                   std::make_move_iterator(compressed_best_factor.begin()),
                                   std::make_move_iterator(compressed_best_factor.end()));
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

    return factor_count;
}
