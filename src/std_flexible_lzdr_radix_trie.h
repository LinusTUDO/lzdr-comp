#ifndef STD_FLEXIBLE_LZDR_RADIX_TRIE_H
#define STD_FLEXIBLE_LZDR_RADIX_TRIE_H
#include "slice.h"
#include "radix_trie.h"

#include <cstddef>

size_t std_flexible_lzdr_radix_trie(Slice input, bool check_decompressed_equals_input);

namespace std_flexible_lzdr_radix_trie_internal {
    bool insert_into_radix_trie(CountedRadixTrie &trie, const Slice &insert);

    bool remove_from_radix_trie(CountedRadixTrie &trie, const Slice &remove);
}

#endif //STD_FLEXIBLE_LZDR_RADIX_TRIE_H
