#include "flexible_lzw_naive.h"
#include "std_flexible_lzw_naive.h"
#include "slice.h"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <optional>
#include <unordered_set>
#include <utility>

// Returns the number of factors
size_t flexible_lzw_naive(const Slice input) {
    std::unordered_set<Slice, SliceHash> previous_factors;
    lzw_naive_internal::initialize_dictionary(previous_factors);

    size_t i = 0;
    size_t factor_count = 0;
    while (i < input.size()) {
        Slice rest_input = input.slice(i);
        const std::optional<Slice> normal_longest_new_factor = lzw_naive_internal::next_longest_new_factor(
            rest_input, previous_factors);
#ifndef NDEBUG
        std::cout << "Rest input: " << rest_input << std::endl;
#endif

        if (!normal_longest_new_factor) {
            // Entire rest input has entry in dictionary,
            // so there will be no new factor to add to the dictionary anymore
#ifndef NDEBUG
            std::cout << "Output factor: " << rest_input << std::endl;
#endif
            ++factor_count;
            break;
        }

#ifndef NDEBUG
        std::cout << "Dictionary factor " << (previous_factors.size() + 1) << ": " << *normal_longest_new_factor
                << std::endl;
#endif
        const size_t longest_output_factor_length = normal_longest_new_factor->size() - 1;
        const bool has_been_inserted = previous_factors.insert(*normal_longest_new_factor).second;
        assert(has_been_inserted && "New factor should not exist in dictionary");

        // Go through all possible output factors
        size_t best_output_factor_length = 0;
        size_t best_total_output_length = 0;
        for (size_t l = longest_output_factor_length; l >= 1; --l) {
#ifndef NDEBUG
            std::cout << "  Testing output factor: " << rest_input.slice(0, l) << std::endl;
#endif

            Slice next_rest_input = rest_input.slice(l);
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
