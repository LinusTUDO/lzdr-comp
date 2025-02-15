#include "cli.h"
#include "slice.h"
#include "flexible_lzw_naive.h"
#include "lzdr_linear_time.h"
#include "lzd_radix_tree.h"
#include "std_flexible_lzw_naive.h"
#include "std_flexible_lzdr_radix_trie.h"
#include "flexible_lzdr_radix_trie.h"
#include "flexible_lzdr_max_radix_trie.h"
#include "lzd_plus_linear_time.h"
#include "test.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {
    void print_help() {
        std::cout << "  --factors\n      Print factors" << std::endl;
        std::cout << std::endl;
        std::cout << "  -c\n      Check decompressing compressed output equals input\n      (not available for LZW variants and Alternative Flexible LZDR Max.)" << std::endl;
        std::cout << std::endl;
        std::cout << "  -a <ALGO_NAME>\n      Run single algorithm\n      (available: lzdr, lzd+)" << std::endl;
        std::cout << std::endl;
        std::cout << "  --test\n      Run tests" << std::endl;
        std::cout << std::endl;
        std::cout << "  --help\n      Show help" << std::endl;
        std::cout << std::endl;
        std::cout << std::endl;
        std::cout << "The input is read from stdin." << std::endl;
    }

    void run_algo(const char* algo, const std::vector<uint8_t> &data, const bool check_decompressed_equals_input) {
        if (strcmp(algo, "lzdr") == 0) {
            std::cout << "LZDR (radix trie)" << std::endl;
            const size_t lzdr_linear_time_num_factors = lzdr_linear_time(Slice(data), check_decompressed_equals_input);
            std::cout << "Num factors: " << lzdr_linear_time_num_factors << std::endl;
        } else if (strcmp(algo, "lzd+") == 0) {
            std::cout << "LZD+ (linear-time)" << std::endl;
            const size_t lzd_plus_linear_time_num_factors = lzd_plus_linear_time(Slice(data), check_decompressed_equals_input);
            std::cout << "Num factors: " << lzd_plus_linear_time_num_factors << std::endl;
        } else {
            std::cout << "The algorithm \"" << algo << "\" is not implemented right now." << std::endl;
            std::exit(1);
        }
    }

    void print_factors(const std::vector<uint8_t> &data, const bool check_decompressed_equals_input) {
        std::cout << "LZDR (radix trie)" << std::endl;
        const size_t lzdr_linear_time_num_factors = lzdr_linear_time(Slice(data), check_decompressed_equals_input);
        std::cout << "Num factors: " << lzdr_linear_time_num_factors << std::endl;

        std::cout << std::endl;

        std::cout << "Standard Flexible LZDR (radix trie)" << std::endl;
        const size_t std_flexible_lzdr_radix_trie_num_factors = std_flexible_lzdr_radix_trie(Slice(data), check_decompressed_equals_input);
        std::cout << "Num factors: " << std_flexible_lzdr_radix_trie_num_factors << std::endl;

        std::cout << std::endl;

        std::cout << "Alternative Flexible LZDR (radix trie)" << std::endl;
        const size_t flexible_lzdr_radix_trie_num_factors = flexible_lzdr_radix_trie(Slice(data), check_decompressed_equals_input);
        std::cout << "Num factors: " << flexible_lzdr_radix_trie_num_factors << std::endl;

        std::cout << std::endl;

        std::cout << "Alternative Flexible LZDR Max. (radix trie)" << std::endl;
        const size_t flexible_lzdr_max_radix_trie_num_factors = flexible_lzdr_max_radix_trie(Slice(data));
        std::cout << "Num factors: " << flexible_lzdr_max_radix_trie_num_factors << std::endl;

        std::cout << std::endl;

        std::cout << "LZD+ (linear-time)" << std::endl;
        const size_t lzd_plus_linear_time_num_factors = lzd_plus_linear_time(Slice(data), check_decompressed_equals_input);
        std::cout << "Num factors: " << lzd_plus_linear_time_num_factors << std::endl;

        std::cout << std::endl;

        std::cout << "LZD (radix trie)" << std::endl;
        const size_t lzd_radix_trie_num_factors = lzd_radix_tree(Slice(data), check_decompressed_equals_input);
        std::cout << "Num factors: " << lzd_radix_trie_num_factors << std::endl;

        std::cout << std::endl;

        std::cout << "Standard Flexible LZW (naive)" << std::endl;
        const size_t std_flexible_lzw_naive_num_factors = std_flexible_lzw_naive(Slice(data));
        std::cout << "Num factors: " << std_flexible_lzw_naive_num_factors << std::endl;

        std::cout << std::endl;

        std::cout << "Alternative Flexible LZW (naive)" << std::endl;
        const size_t flexible_lzw_naive_num_factors = flexible_lzw_naive(Slice(data));
        std::cout << "Num factors: " << flexible_lzw_naive_num_factors << std::endl;
    }
}

void parse_args(const int argc, char *argv[]) {
    bool check_decompressed_equals_input = false;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) {
            check_decompressed_equals_input = true;
            break;
        }
    }
    bool cmd_found = false;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0) {
            if (i + 1 < argc) {
                const std::vector<uint8_t> data = read_stdin();
                run_algo(argv[i+1], data, check_decompressed_equals_input);
                cmd_found = true;
                break;
            } else {
                std::cout << "No algorithm name provided." << std::endl;
                std::exit(1);
            }
        }
        if (strcmp(argv[i], "--help") == 0) {
            print_help();
            cmd_found = true;
            break;
        }
        if (strcmp(argv[i], "--test") == 0) {
            run_tests();
            cmd_found = true;
            break;
        }
        if (strcmp(argv[i], "--factors") == 0) {
            const std::vector<uint8_t> data = read_stdin();
            print_factors(data, check_decompressed_equals_input);
            cmd_found = true;
            break;
        }
    }
    if (!cmd_found) {
        print_help();
        std::exit(1);
    }
}

std::vector<uint8_t> read_stdin() {
    // Read from stdin until EOF in chunks
    std::vector<uint8_t> data;
    std::vector<uint8_t> buffer(4096);
    data.reserve(4096);

    while (std::cin.read(reinterpret_cast<char *>(buffer.data()), buffer.size()) || std::cin.gcount() > 0) {
        data.insert(data.end(), buffer.begin(), buffer.begin() + std::cin.gcount());
    }

    // https://en.cppreference.com/w/cpp/io/basic_ios/fail
    if (std::cin.bad()) {
        std::cerr << "I/O error while reading" << std::endl;
        std::exit(1);
    }
    if (std::cin.fail() && !std::cin.eof()) {
        std::cerr << "Non-integer data encountered" << std::endl;
        std::exit(1);
    }

    return data;
}
