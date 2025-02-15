cmake_minimum_required(VERSION 3.29)
project(lzdr-comp)

set(CMAKE_CXX_STANDARD 17)

option(IWYU_ENABLED "Enable IWYU" OFF)

add_executable(
        lzdr-comp
        src/main.cpp
        src/slice.cpp
        src/slice.h
        src/compressor.cpp
        src/compressor.h
        src/radix_trie.cpp
        src/radix_trie.h
        src/lzdr_linear_time.cpp
        src/lzdr_linear_time.h
        src/std_flexible_lzdr_radix_trie.cpp
        src/std_flexible_lzdr_radix_trie.h
        src/flexible_lzdr_radix_trie.cpp
        src/flexible_lzdr_radix_trie.h
        src/flexible_lzdr_max_radix_trie.cpp
        src/flexible_lzdr_max_radix_trie.h
        src/lzd_plus_linear_time.cpp
        src/lzd_plus_linear_time.h
        src/lzd_radix_tree.cpp
        src/lzd_radix_tree.h
        src/std_flexible_lzw_naive.cpp
        src/std_flexible_lzw_naive.h
        src/flexible_lzw_naive.cpp
        src/flexible_lzw_naive.h
        src/test.cpp
        src/test.h
        src/cli.cpp
        src/cli.h
)

if(IWYU_ENABLED)
    find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
    set_property(TARGET lzdr-comp PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()
