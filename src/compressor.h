#ifndef COMPRESSOR_H
#define COMPRESSOR_H
#include "slice.h"
#include "radix_trie.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

void u32_to_bytes(uint32_t value, uint8_t *bytes);

uint32_t u32_from_bytes(const uint8_t *bytes);

class Compressor {
    std::vector<uint8_t> internal_data;
    size_t written_bytes;

public:
    explicit Compressor(const size_t count) : internal_data(std::vector<uint8_t>(count, 0)), written_bytes(0) {
    }

    [[nodiscard]] std::vector<uint8_t> data() {
        return std::move(internal_data);
    }

    void write_byte(const uint8_t byte) {
        if (written_bytes >= internal_data.size()) {
            throw std::out_of_range("Index out of bounds");
        }
        internal_data[written_bytes] = byte;
        written_bytes += 1;
    }

    void write_u32(const uint32_t value) {
        if (written_bytes + 3 >= internal_data.size()) {
            throw std::out_of_range("Index out of bounds");
        }
        u32_to_bytes(value, internal_data.data() + written_bytes);
        written_bytes += 4;
    }
};

struct NextFactorResult {
    Slice factor_slice;
    std::vector<uint8_t> compressed_data;
    bool used_extra_truncation;
};

struct NextFactorResult2 {
    Slice factor_slice;
    std::vector<uint8_t> compressed_data;
    bool used_extra_truncation;
    RadixTrieNode* insertion_node;
    Slice insertion_slice;
};

#endif //COMPRESSOR_H
