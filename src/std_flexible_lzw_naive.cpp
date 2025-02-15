#include "std_flexible_lzw_naive.h"
#include "slice.h"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lzw_naive_internal {
    void initialize_dictionary(std::unordered_set<Slice, SliceHash> &previous_factors) {
        for (uint8_t i = 0; ; ++i) {
            Slice single_char = {ALL_BYTES + i, 1};
            previous_factors.insert(single_char);
            if (i == 255) {
                break;
            }
        }
    }

    std::optional<Slice> next_longest_new_factor(const Slice &rest_input,
                                                 const std::unordered_set<Slice, SliceHash> &
                                                 previous_factors) {
        // Since all factors of length 1 are already in the dictionary,
        // we can start with length 2 to find a new factor.
        for (size_t i = 2; i <= rest_input.size(); ++i) {
            Slice new_rest_input = rest_input.slice(0, i);

            if (previous_factors.find(new_rest_input) != previous_factors.end()) {
                // Already in dictionary, skip
                continue;
            }

            return std::make_optional(new_rest_input);
        }

        return std::nullopt;
    }
}

// Returns the number of factors
size_t std_flexible_lzw_naive(const Slice input) {
    // LZW factors: pairs of position when factor gets available and factor slice
    // If the position is `i` and we are reading a character input[j] with j >= i,
    // the factor is available.
    std::vector<std::pair<size_t, Slice> > lzw_factors_with_availability_position;
    std::unordered_set<Slice, SliceHash> previous_factors;
    lzw_naive_internal::initialize_dictionary(previous_factors);

    size_t i = 0;
    while (i < input.size()) {
        Slice rest_input = input.slice(i);
        std::optional<Slice> new_longest_factor = lzw_naive_internal::next_longest_new_factor(
            rest_input, previous_factors);

        if (!new_longest_factor) {
            // Entire rest input has entry in dictionary,
            // so there will be no new factor to add to the dictionary anymore
            break;
        }

#ifndef NDEBUG
        std::cout << "Dictionary factor " << (previous_factors.size() + 1) << ": " << *new_longest_factor << std::endl;
#endif

        // We only advance by the length of the largest prefix found in the dictionary,
        // and that is the new factor size minus 1.
        i += new_longest_factor->size() - 1;

        // We add this to the vector after increasing `i` to the next position,
        // because that is where the factor will get available
        lzw_factors_with_availability_position.emplace_back(i, *new_longest_factor);

        previous_factors.insert(*new_longest_factor);
    }

    // Reset
    previous_factors.clear();
    lzw_naive_internal::initialize_dictionary(previous_factors);
    size_t dictionary_initialized_size = previous_factors.size();
    i = 0;

    // Now the actual standard flexible LZW
    std::vector<Slice> temporary_factors;
    size_t factor_count = 0;
    while (i < input.size()) {
        Slice rest_input = input.slice(i);
#ifndef NDEBUG
        std::cout << "Rest input: " << rest_input << std::endl;
#endif

        // Add all factors that become available with i, and do this before calculating `normal_longest_new_factor`
        assert(previous_factors.size() >= dictionary_initialized_size);
        while (previous_factors.size() - dictionary_initialized_size < lzw_factors_with_availability_position.size()) {
            std::pair<size_t, Slice> next_possible_factor = lzw_factors_with_availability_position[previous_factors.
                size() - dictionary_initialized_size];
            if (i >= next_possible_factor.first) {
#ifndef NDEBUG
                std::cout << "New available dictionary factor: " << next_possible_factor.second << std::endl;
#endif
                const bool has_been_inserted = previous_factors.insert(next_possible_factor.second).second;
                assert(has_been_inserted && "New dictionary factor should not exist in dictionary");
            } else {
                break;
            }
        }

        std::optional<Slice> normal_longest_new_factor = lzw_naive_internal::next_longest_new_factor(
            rest_input, previous_factors);
        if (!normal_longest_new_factor) {
            // Entire rest input has entry in dictionary,
            // so there will be no new factor to add to the dictionary anymore
#ifndef NDEBUG
            std::cout << "Output factor: " << rest_input << std::endl;
#endif
            ++factor_count;
            break;
        }

        // Go through all possible output factors
        const size_t longest_output_factor_length = normal_longest_new_factor->size() - 1;
        size_t best_output_factor_length = 0;
        size_t best_total_output_length = 0;
        for (size_t l = longest_output_factor_length; l >= 1; --l) {
#ifndef NDEBUG
            std::cout << "  Testing output factor: " << rest_input.slice(0, l) << std::endl;
#endif

            Slice next_rest_input = rest_input.slice(l);

            // Add all factors that become available at `i + l`
            assert(previous_factors.size() >= dictionary_initialized_size);
            while (previous_factors.size() - dictionary_initialized_size < lzw_factors_with_availability_position.
                   size()) {
                std::pair<size_t, Slice> next_possible_factor = lzw_factors_with_availability_position[previous_factors.
                    size() - dictionary_initialized_size];
                if (i + l >= next_possible_factor.first) {
                    const bool has_been_inserted = previous_factors.insert(next_possible_factor.second).second;
                    assert(has_been_inserted && "Temporary dictionary factor should not exist in dictionary");
                    temporary_factors.push_back(next_possible_factor.second);
#ifndef NDEBUG
                    std::cout << "    -> Temporarily available dictionary factor: " << next_possible_factor.second <<
                            std::endl;
#endif
                } else {
                    break;
                }
            }

            std::optional<Slice> next_next_new_factor = lzw_naive_internal::next_longest_new_factor(
                next_rest_input, previous_factors);
            size_t next_next_output_factor_length = next_rest_input.size();
            if (next_next_new_factor) {
                next_next_output_factor_length = next_next_new_factor->size() - 1;
            }
            const size_t current_total_length = l + next_next_output_factor_length;
#ifndef NDEBUG
            std::cout << "    -> Next rest: " << next_rest_input << std::endl;
            if (next_next_new_factor) {
                std::cout << "    -> Next output factor: "
                        << next_next_new_factor->slice(0, next_next_new_factor->size() - 1)
                        << " (total length: " << current_total_length << ")" << std::endl;
            } else {
                std::cout << "    -> Next output factor: " << next_rest_input
                        << " (total length: " << current_total_length << ")" << std::endl;
            }
#endif

            if (best_output_factor_length == 0 || current_total_length > best_total_output_length) {
                best_output_factor_length = l;
                best_total_output_length = current_total_length;
            }

            // Remove factors that were temporarily added
            for (Slice &temporary_factor: temporary_factors) {
                const size_t amount_removed = previous_factors.erase(temporary_factor);
                assert(amount_removed == 1 && "Temporary dictionary factor should exist in dictionary");
            }
            temporary_factors.clear();
        }

        assert(best_output_factor_length > 0 && "Factor length must be greater than 0");

#ifndef NDEBUG
        std::cout << "Output factor: " << rest_input.slice(0, best_output_factor_length) << std::endl;
#endif

        ++factor_count;
        i += best_output_factor_length;
    }

    return factor_count;
}
