#ifndef FLEXIBLE_LZDR_RADIX_TRIE_H
#define FLEXIBLE_LZDR_RADIX_TRIE_H
#include "slice.h"

#include <cstddef>

size_t flexible_lzdr_radix_trie(Slice input, bool check_decompressed_equals_input);

#endif //FLEXIBLE_LZDR_RADIX_TRIE_H
