#ifndef SLICE_H
#define SLICE_H
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

// Represents a slice of bytes
class Slice {
    const uint8_t *internal_data;
    size_t internal_length;

public:
    Slice(const uint8_t *data, const size_t length) : internal_data(data), internal_length(length) {
    }

    explicit Slice(const uint8_t *data) : internal_data(data), internal_length(sizeof(data)) {
    }

    explicit Slice(const std::vector<uint8_t> &vec) : internal_data(vec.data()), internal_length(vec.size()) {
    }

    explicit Slice(const std::string &str) : internal_data(reinterpret_cast<const uint8_t *>(str.data())),
                                             internal_length(str.size()) {
    }

    explicit Slice(const char *str) : internal_data(reinterpret_cast<const uint8_t *>(str)),
                                      internal_length(strlen(str)) {
    }

    static Slice create_empty() {
        return Slice{nullptr, 0};
    }

    [[nodiscard]] const uint8_t *data() const {
        return internal_data;
    }

    [[nodiscard]] size_t size() const {
        return internal_length;
    }

    [[nodiscard]] bool empty() const {
        return internal_length == 0;
    }

    const uint8_t &operator[](const size_t index) const {
        if (index >= internal_length) {
            throw std::out_of_range("Index out of bounds");
        }
        return internal_data[index];
    }

    bool operator==(const Slice &other) const {
        return internal_length == other.internal_length
               && std::memcmp(internal_data, other.internal_data, internal_length) == 0;
    }

    // Returns a new slice that starts at pos and has length count.
    // Basically, a view of [pos, pos + count).
    [[nodiscard]] Slice slice(const size_t pos) const {
        if (pos > internal_length) {
            throw std::out_of_range("Slice out of bounds");
        }
        const Slice s(internal_data + pos, internal_length - pos);
        return s;
    }

    // Returns a new slice with view [pos, pos + rlen), where rlen is the smaller of count and size() - pos.
    [[nodiscard]] Slice slice(const size_t pos, const size_t count) const {
        if (pos > internal_length) {
            throw std::out_of_range("Slice out of bounds");
        }
        const Slice s(internal_data + pos, std::min(count, internal_length - pos));
        return s;
    }

    friend std::ostream &operator<<(std::ostream &os, const Slice &slice) {
        for (size_t i = 0; i < slice.internal_length; ++i) {
            if (slice.internal_data[i] >= 32 && slice.internal_data[i] < 127) {
                os << static_cast<char>(slice.internal_data[i]);
            } else {
                os << "\\x" << std::setw(2) << std::setfill('0') << std::hex
                        << static_cast<int>(slice.internal_data[i]) << std::dec;
            }
        }
        return os;
    }
};

struct SliceHash {
    std::size_t operator()(const Slice &slice) const;
};

#endif //SLICE_H
