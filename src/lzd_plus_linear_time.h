#ifndef LZD_PLUS_LINEAR_TIME_H
#define LZD_PLUS_LINEAR_TIME_H
#include "compressor.h"
#include "slice.h"
#include "radix_trie.h"

#include <cstddef>

size_t lzd_plus_linear_time(Slice input, bool check_decompressed_equals_input);

namespace lzd_plus_linear_time_internal {
    NextFactorResult2 next_longest_factor(const Slice &rest_input, RadixTrie &previous_factors);
}

#endif //LZD_PLUS_LINEAR_TIME_H
