#include "slice.h"

#include <cstddef>
#include <functional>
#include <limits>

// Implement Hash for Slice
namespace {
    // https://stackoverflow.com/a/78509978/245265
    //
    // Borrowed from Boost.ContainerHash
    // https://github.com/boostorg/container_hash/blob/ee5285bfa64843a11e29700298c83a37e3132fcd/include/boost/container_hash/hash.hpp#L471
    template<typename T>
    void hash_combine(std::size_t &seed, const T &v) {
        static constexpr auto digits = std::numeric_limits<std::size_t>::digits;
        static_assert(digits == 64 || digits == 32);

        if constexpr (digits == 64) {
            // https://github.com/boostorg/container_hash/blob/ee5285bfa64843a11e29700298c83a37e3132fcd/include/boost/container_hash/detail/hash_mix.hpp#L67
            std::size_t x = seed + 0x9e3779b9 + std::hash<T>()(v);
            constexpr std::size_t m = 0xe9846af9b1a615d;
            x ^= x >> 32;
            x *= m;
            x ^= x >> 32;
            x *= m;
            x ^= x >> 28;
            seed = x;
        } else {
            // 32-bit variant
            // https://github.com/boostorg/container_hash/blob/ee5285bfa64843a11e29700298c83a37e3132fcd/include/boost/container_hash/detail/hash_mix.hpp#L88
            std::size_t x = seed + 0x9e3779b9 + std::hash<T>()(v);
            constexpr std::size_t m1 = 0x21f0aaad;
            constexpr std::size_t m2 = 0x735a2d97;
            x ^= x >> 16;
            x *= m1;
            x ^= x >> 15;
            x *= m2;
            x ^= x >> 15;
            seed = x;
        }
    }
}

std::size_t SliceHash::operator()(const Slice &slice) const {
    std::size_t seed = 0;
    for (size_t i = 0; i < slice.size(); ++i) {
        hash_combine(seed, slice[i]);
    }
    return seed;
}
