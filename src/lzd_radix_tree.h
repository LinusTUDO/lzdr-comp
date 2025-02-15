#ifndef LZD_RADIX_TREE_H
#define LZD_RADIX_TREE_H
#include "compressor.h"
#include "slice.h"
#include "radix_trie.h"

#include <cstddef>

size_t lzd_radix_tree(Slice input, bool check_decompressed_equals_input);

namespace lzd_radix_tree_internal {
    NextFactorResult next_longest_factor(const Slice &rest_input, RadixTrie &previous_factors);
}

#endif //LZD_RADIX_TREE_H
