#ifndef LZDR_LINEAR_TIME_H
#define LZDR_LINEAR_TIME_H
#include "compressor.h"
#include "slice.h"
#include "radix_trie.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

size_t lzdr_linear_time(Slice input, bool check_decompressed_equals_input);

namespace lzdr_linear_time_internal {
    NextFactorResult2 next_longest_factor(
        const Slice &entire_input, size_t bytes_already_read,
        const Slice &rest_input, RadixTrie &previous_factors);

    NextFactorResult next_longest_factor_counted_trie(
        const Slice &entire_input, size_t bytes_already_read,
        const Slice &rest_input, size_t usable_rest_input_len,
        CountedRadixTrie &previous_factors);

    bool insert_into_radix_trie(RadixTrie &trie, RadixTrieNode *from_node, const Slice &insert);
}

namespace lzdr_compressor {
    Compressor create_compressor_for_combination(bool first_is_byte, bool second_is_byte);

    Compressor create_compressor_for_truncation();

    Compressor create_compressor_for_repetition(bool is_byte);
}

std::vector<uint8_t> lzdr_decompress(const std::vector<uint8_t> &compressed);

std::string debug_lzdr_data(const std::vector<uint8_t> &compressed, const std::vector<uint8_t> &current_data);

#endif //LZDR_LINEAR_TIME_H
