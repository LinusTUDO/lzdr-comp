#include "test.h"
#include "flexible_lzw_naive.h"
#include "lzdr_linear_time.h"
#include "std_flexible_lzw_naive.h"
#include "std_flexible_lzdr_radix_trie.h"
#include "flexible_lzdr_radix_trie.h"
#include "flexible_lzdr_max_radix_trie.h"
#include "lzd_plus_linear_time.h"
#include "lzd_radix_tree.h"
#include "radix_trie.h"
#include "slice.h"

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>

void run_tests() {
#ifdef NDEBUG
    std::cerr << "Tests have to be run in debug mode" << std::endl;
    std::exit(1);
#endif

    // LZDR test case from my example
    constexpr char input_1[] = "abcdabababefababababababababghababacdabi";
    std::cout << "LZDR (radix trie)" << std::endl;
    const size_t lzdr_linear_time_num_factors = lzdr_linear_time(Slice(input_1), true);
    std::cout << "Num factors: " << lzdr_linear_time_num_factors << std::endl;
    assert(lzdr_linear_time_num_factors == 9);

    std::cout << std::endl;

    std::cout << "Standard Flexible LZDR (radix trie)" << std::endl;
    const size_t std_flexible_lzdr_radix_trie_num_factors = std_flexible_lzdr_radix_trie(Slice(input_1), true);
    std::cout << "Num factors: " << std_flexible_lzdr_radix_trie_num_factors << std::endl;
    assert(std_flexible_lzdr_radix_trie_num_factors == 9);

    std::cout << std::endl;

    std::cout << "Alternative Flexible LZDR (radix trie)" << std::endl;
    const size_t flexible_lzdr_radix_trie_num_factors = flexible_lzdr_radix_trie(Slice(input_1), true);
    std::cout << "Num factors: " << flexible_lzdr_radix_trie_num_factors << std::endl;
    assert(flexible_lzdr_radix_trie_num_factors == 9);

    std::cout << std::endl;

    // Random LZW test case
    constexpr char input_2[] = "wabbawabba";
    std::cout << "Alternative Flexible LZW (naive)" << std::endl;
    const size_t flexible_lzw_naive_num_factors = flexible_lzw_naive(Slice(input_2));
    std::cout << "Num factors: " << flexible_lzw_naive_num_factors << std::endl;
    assert(flexible_lzw_naive_num_factors == 8);

    std::cout << std::endl;

    // Test alternative flexible LZDR
    constexpr char input_3[] = "abcdefabcdabcdeffgfghfghijklmjklmabcdefghijklm";
    std::cout << "LZDR (radix trie)" << std::endl;
    const size_t lzdr_linear_time_num_factors2 = lzdr_linear_time(Slice(input_3), true);
    std::cout << "Num factors: " << lzdr_linear_time_num_factors2 << std::endl;
    assert(lzdr_linear_time_num_factors2 == 14);

    std::cout << std::endl;

    std::cout << "Alternative Flexible LZDR (radix trie)" << std::endl;
    const size_t flexible_lzdr_radix_trie_num_factors2 = flexible_lzdr_radix_trie(Slice(input_3), true);
    std::cout << "Num factors: " << flexible_lzdr_radix_trie_num_factors2 << std::endl;
    assert(flexible_lzdr_radix_trie_num_factors2 == 13);

    std::cout << std::endl;

    // LZW test case from matias02effect
    constexpr char input_4[] = "abababaabaabaaab";
    std::cout << "Standard Flexible LZW (naive)" << std::endl;
    const size_t std_flexible_lzw_naive_num_factors = std_flexible_lzw_naive(Slice(input_4));
    std::cout << "Num factors: " << std_flexible_lzw_naive_num_factors << std::endl;
    assert(std_flexible_lzw_naive_num_factors == 7);

    std::cout << std::endl;

    std::cout << "Alternative Flexible LZW (naive)" << std::endl;
    const size_t flexible_lzw_naive_num_factors2 = flexible_lzw_naive(Slice(input_4));
    std::cout << "Num factors: " << flexible_lzw_naive_num_factors2 << std::endl;
    assert(flexible_lzw_naive_num_factors2 == 7);

    std::cout << std::endl;

    // LZD test case from goto15lzd
    constexpr char input_5[] = "abaaabababaabbbbabab$";
    std::cout << "LZD (radix trie)" << std::endl;
    const size_t lzd_radix_trie_num_factors = lzd_radix_tree(Slice(input_5), true);
    std::cout << "Num factors: " << lzd_radix_trie_num_factors << std::endl;
    assert(lzd_radix_trie_num_factors == 7);

    std::cout << std::endl;

    // Test alternative flexible LZD
    constexpr char input_6[] = "aabaaaba";
    std::cout << "LZD (radix trie)" << std::endl;
    const size_t lzd_radix_tree_num_factors2 = lzd_radix_tree(Slice(input_6), true);
    std::cout << "Num factors: " << lzd_radix_tree_num_factors2 << std::endl;
    assert(lzd_radix_tree_num_factors2 == 4);

    std::cout << std::endl;

    // LZDR single char repetition test
    constexpr char input_7[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    std::cout << "LZDR (radix trie)" << std::endl;
    const size_t lzdr_linear_time_num_factors3 = lzdr_linear_time(Slice(input_7), true);
    std::cout << "Num factors: " << lzdr_linear_time_num_factors3 << std::endl;
    assert(lzdr_linear_time_num_factors3 == 1);

    std::cout << std::endl;

    // LZDR combination truncation test
    // (1)ab (2)cd (3)abcd (4)wx (5)yz (6)wxyz (7)zk (8)zkl (9,truncated)abcdwxy (10)zklcd
    constexpr char input_8[] = "abcdabcdwxyzwxyzzkzklabcdwxyzklcd";
    std::cout << "Alternative Flexible LZDR (radix trie)" << std::endl;
    const size_t flexible_lzdr_radix_trie_num_factors3 = flexible_lzdr_radix_trie(Slice(input_8), true);
    std::cout << "Num factors: " << flexible_lzdr_radix_trie_num_factors3 << std::endl;
    assert(flexible_lzdr_radix_trie_num_factors3 == 10);

    std::cout << std::endl;

    std::cout << "LZDR (radix trie)" << std::endl;
    const size_t lzdr_linear_time_num_factors4 = lzdr_linear_time(Slice(input_8), true);
    std::cout << "Num factors: " << lzdr_linear_time_num_factors4 << std::endl;
    assert(lzdr_linear_time_num_factors4 == 11);

    std::cout << std::endl;

    // LZDR combination truncation test 2
    // (1)aaaaa (2)caaaa
    constexpr char input_8_2[] = "aaaaacaaaa";
    std::cout << "LZDR (radix trie)" << std::endl;
    const size_t lzdr_linear_time_num_factors4_2 = lzdr_linear_time(Slice(input_8_2), true);
    std::cout << "Num factors: " << lzdr_linear_time_num_factors4_2 << std::endl;
    assert(lzdr_linear_time_num_factors4_2 == 2);

    std::cout << std::endl;

    // LZDR repetition truncation test
    // (1)ab (2)abc (3)abcabcabcabcabca
    constexpr char input_9[] = "ababcabcabcabcabcabca";
    std::cout << "LZDR (radix trie)" << std::endl;
    const size_t lzdr_linear_time_num_factors5 = lzdr_linear_time(Slice(input_9), true);
    std::cout << "Num factors: " << lzdr_linear_time_num_factors5 << std::endl;
    assert(lzdr_linear_time_num_factors5 == 3);

    std::cout << std::endl;

    // Test standard flexible LZDR and LZD
    constexpr char input_10[] = "abcddedefdefgabcdefg";
    std::cout << "LZDR (radix trie)" << std::endl;
    const size_t lzdr_linear_time_num_factors6 = lzdr_linear_time(Slice(input_10), true);
    std::cout << "Num factors: " << lzdr_linear_time_num_factors6 << std::endl;
    assert(lzdr_linear_time_num_factors6 == 8);

    std::cout << std::endl;

    std::cout << "LZD (radix trie)" << std::endl;
    const size_t lzd_radix_tree_num_factors3 = lzd_radix_tree(Slice(input_10), true);
    std::cout << "Num factors: " << lzd_radix_tree_num_factors3 << std::endl;
    assert(lzd_radix_tree_num_factors3 == 8);

    std::cout << std::endl;

    std::cout << "Standard Flexible LZDR (radix trie)" << std::endl;
    const size_t std_flexible_lzdr_radix_trie_num_factors2 = std_flexible_lzdr_radix_trie(Slice(input_10), true);
    std::cout << "Num factors: " << std_flexible_lzdr_radix_trie_num_factors2 << std::endl;
    assert(std_flexible_lzdr_radix_trie_num_factors2 == 7);

    std::cout << std::endl;

    // Test LZD+
    constexpr char input_11[] = "aabbaabbbaabbbbbababaabccccbababc";
    std::cout << "LZD+ (linear-time)" << std::endl;
    const size_t lzdp_linear_time_num_factors = lzd_plus_linear_time(Slice(input_11), true);
    std::cout << "Num factors: " << lzdp_linear_time_num_factors << std::endl;
    assert(lzdp_linear_time_num_factors == 11);

    std::cout << std::endl;

    std::cout << "LZDR (radix trie)" << std::endl;
    const size_t lzdr_linear_time_num_factors7 = lzdr_linear_time(Slice(input_11), true);
    std::cout << "Num factors: " << lzdr_linear_time_num_factors7 << std::endl;
    assert(lzdr_linear_time_num_factors7 == 10);

    std::cout << std::endl;

    std::cout << "LZD (radix trie)" << std::endl;
    const size_t lzd_radix_tree_num_factors4 = lzd_radix_tree(Slice(input_11), true);
    std::cout << "Num factors: " << lzd_radix_tree_num_factors4 << std::endl;
    assert(lzd_radix_tree_num_factors4 == 12);

    std::cout << std::endl;

    // Test Alternative Flexible LZDR Max.
    constexpr char input_12[] = "aaababaaaaaabaaab";
    std::cout << "LZDR (radix trie)" << std::endl;
    const size_t lzdr_linear_time_num_factors8 = lzdr_linear_time(Slice(input_12), true);
    std::cout << "Num factors: " << lzdr_linear_time_num_factors8 << std::endl;
    assert(lzdr_linear_time_num_factors8 == 6);

    std::cout << std::endl;

    std::cout << "Standard Flexible LZDR (radix trie)" << std::endl;
    const size_t std_flexible_lzdr_radix_trie_num_factors3 = std_flexible_lzdr_radix_trie(Slice(input_12), true);
    std::cout << "Num factors: " << std_flexible_lzdr_radix_trie_num_factors3 << std::endl;
    assert(std_flexible_lzdr_radix_trie_num_factors3 == 5);

    std::cout << std::endl;

    std::cout << "Alternative Flexible LZDR (radix trie)" << std::endl;
    const size_t flexible_lzdr_radix_trie_num_factors4 = flexible_lzdr_radix_trie(Slice(input_12), true);
    std::cout << "Num factors: " << flexible_lzdr_radix_trie_num_factors4 << std::endl;
    assert(flexible_lzdr_radix_trie_num_factors4 == 5);

    std::cout << std::endl;

    std::cout << "Alternative Flexible LZDR Max. (radix trie)" << std::endl;
    const size_t flexible_lzdr_max_radix_trie_num_factors = flexible_lzdr_max_radix_trie(Slice(input_12));
    std::cout << "Num factors: " << flexible_lzdr_max_radix_trie_num_factors << std::endl;
    assert(flexible_lzdr_max_radix_trie_num_factors == 5);

    std::cout << std::endl;

    // Radix trie, Wikipedia test cases (https://en.wikipedia.org/wiki/Radix_tree)
    RadixTrie trie1;
    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie1, &trie1.root_node, Slice("test")));
    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie1, &trie1.root_node, Slice("slow")));
    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie1, &trie1.root_node, Slice("water")));
    std::cout << "Radix trie #1.1: " << trie1.root_node.debug_representation_json() << std::endl;
    assert(trie1.root_node.debug_representation_json() == "{\"0(0)\":{\"slow\":{\"2(2)\":{}},\"test\":{\"1(1)\":{}},\"water\":{\"3(3)\":{}}}}");

    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie1, &trie1.root_node, Slice("slower")));
    std::cout << "Radix trie #1.2: " << trie1.root_node.debug_representation_json() << std::endl;
    assert(trie1.root_node.debug_representation_json() == "{\"0(0)\":{\"slow\":{\"2(2)\":{\"er\":{\"4(4)\":{}}}},\"test\":{\"1(1)\":{}},\"water\":{\"3(3)\":{}}}}");

    RadixTrie trie2;
    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie2, &trie2.root_node, Slice("tester")));
    std::cout << "Radix trie #2.1: " << trie2.root_node.debug_representation_json() << std::endl;
    assert(trie2.root_node.debug_representation_json() == "{\"0(0)\":{\"tester\":{\"1(1)\":{}}}}");

    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie2, &trie2.root_node, Slice("test")));
    std::cout << "Radix trie #2.2: " << trie2.root_node.debug_representation_json() << std::endl;
    assert(trie2.root_node.debug_representation_json() == "{\"0(0)\":{\"test\":{\"2(2)\":{\"er\":{\"1(1)\":{}}}}}}");

    RadixTrie trie3;
    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie3, &trie3.root_node, Slice("test")));
    std::cout << "Radix trie #3.1: " << trie3.root_node.debug_representation_json() << std::endl;
    assert(trie3.root_node.debug_representation_json() == "{\"0(0)\":{\"test\":{\"1(1)\":{}}}}");

    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie3, &trie3.root_node, Slice("team")));
    std::cout << "Radix trie #3.2: " << trie3.root_node.debug_representation_json() << std::endl;
    assert(trie3.root_node.debug_representation_json() == "{\"0(0)\":{\"te\":{\"0(1)\":{\"am\":{\"2(2)\":{}},\"st\":{\"1(1)\":{}}}}}}");

    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie3, &trie3.root_node, Slice("toast")));
    std::cout << "Radix trie #3.3: " << trie3.root_node.debug_representation_json() << std::endl;
    assert(trie3.root_node.debug_representation_json() == "{\"0(0)\":{\"t\":{\"0(1)\":{\"e\":{\"0(1)\":{\"am\":{\"2(2)\":{}},\"st\":{\"1(1)\":{}}}},\"oast\":{\"3(3)\":{}}}}}}");

    // Additional custom tests apart from Wikipedia.
    // Convert splitting node to factor node after single byte edge
    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie3, &trie3.root_node, Slice("t")));
    std::cout << "Radix trie #3.4: " << trie3.root_node.debug_representation_json() << std::endl;
    assert(trie3.root_node.debug_representation_json() == "{\"0(0)\":{\"t\":{\"4(4)\":{\"e\":{\"0(1)\":{\"am\":{\"2(2)\":{}},\"st\":{\"1(1)\":{}}}},\"oast\":{\"3(3)\":{}}}}}}");
    // Assert stays same if inserted again
    assert(!lzdr_linear_time_internal::insert_into_radix_trie(trie3, &trie3.root_node, Slice("t")));
    assert(trie3.root_node.debug_representation_json() == "{\"0(0)\":{\"t\":{\"4(4)\":{\"e\":{\"0(1)\":{\"am\":{\"2(2)\":{}},\"st\":{\"1(1)\":{}}}},\"oast\":{\"3(3)\":{}}}}}}");

    // Add factor node for first character of edge
    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie3, &trie3.root_node, Slice("to")));
    std::cout << "Radix trie #3.5: " << trie3.root_node.debug_representation_json() << std::endl;
    assert(trie3.root_node.debug_representation_json() == "{\"0(0)\":{\"t\":{\"4(4)\":{\"e\":{\"0(1)\":{\"am\":{\"2(2)\":{}},\"st\":{\"1(1)\":{}}}},\"o\":{\"5(5)\":{\"ast\":{\"3(3)\":{}}}}}}}}");

    // Add factor node for the last character of edge (converting splitting node to factor node)
    RadixTrie trie4;
    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie4, &trie4.root_node, Slice("toast")));
    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie4, &trie4.root_node, Slice("tool")));
    std::cout << "Radix trie #4.1: " << trie4.root_node.debug_representation_json() << std::endl;
    assert(trie4.root_node.debug_representation_json() == "{\"0(0)\":{\"to\":{\"0(1)\":{\"ast\":{\"1(1)\":{}},\"ol\":{\"2(2)\":{}}}}}}");

    assert(lzdr_linear_time_internal::insert_into_radix_trie(trie4, &trie4.root_node, Slice("to")));
    std::cout << "Radix trie #4.2: " << trie4.root_node.debug_representation_json() << std::endl;
    assert(trie4.root_node.debug_representation_json() == "{\"0(0)\":{\"to\":{\"3(3)\":{\"ast\":{\"1(1)\":{}},\"ol\":{\"2(2)\":{}}}}}}");
    // Assert stays same if inserted again
    assert(!lzdr_linear_time_internal::insert_into_radix_trie(trie4, &trie4.root_node, Slice("to")));
    assert(trie4.root_node.debug_representation_json() == "{\"0(0)\":{\"to\":{\"3(3)\":{\"ast\":{\"1(1)\":{}},\"ol\":{\"2(2)\":{}}}}}}");
    // Assert empty insertion changes nothing
    //assert(!lzdr_linear_time_internal::insert_into_radix_trie(trie4, Slice("")));
    //assert(trie4.root_node.debug_representation_json() == "{\"0(0)\":{\"to\":{\"3(3)\":{\"ast\":{\"1(1)\":{}},\"ol\":{\"2(2)\":{}}}}}}");

    std::cout << std::endl;

    // Counted radix trie

    // Radix trie, Wikipedia test cases (https://en.wikipedia.org/wiki/Radix_tree)
    CountedRadixTrie ctrie1;
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie1, Slice("test")));
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie1, Slice("slow")));
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie1, Slice("water")));
    std::cout << "Counted radix trie #1.1: " << ctrie1.root_node.debug_representation_json() << std::endl;
    assert(ctrie1.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"slow\":{\"2(2)(0)\":{}},\"test\":{\"1(1)(0)\":{}},\"water\":{\"3(3)(0)\":{}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie1, Slice("slower")));
    std::cout << "Counted radix trie #1.2: " << ctrie1.root_node.debug_representation_json() << std::endl;
    assert(ctrie1.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"slow\":{\"2(2)(0)\":{\"er\":{\"4(4)(0)\":{}}}},\"test\":{\"1(1)(0)\":{}},\"water\":{\"3(3)(0)\":{}}}}");

    CountedRadixTrie ctrie2;
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie2, Slice("tester")));
    std::cout << "Counted radix trie #2.1: " << ctrie2.root_node.debug_representation_json() << std::endl;
    assert(ctrie2.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"tester\":{\"1(1)(0)\":{}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie2, Slice("test")));
    std::cout << "Counted radix trie #2.2: " << ctrie2.root_node.debug_representation_json() << std::endl;
    assert(ctrie2.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"test\":{\"2(2)(0)\":{\"er\":{\"1(1)(0)\":{}}}}}}");

    CountedRadixTrie ctrie3;
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie3, Slice("test")));
    std::cout << "Counted radix trie #3.1: " << ctrie3.root_node.debug_representation_json() << std::endl;
    assert(ctrie3.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"test\":{\"1(1)(0)\":{}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie3, Slice("team")));
    std::cout << "Counted radix trie #3.2: " << ctrie3.root_node.debug_representation_json() << std::endl;
    assert(ctrie3.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"te\":{\"0(1)(0)\":{\"am\":{\"2(2)(0)\":{}},\"st\":{\"1(1)(0)\":{}}}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie3, Slice("toast")));
    std::cout << "Counted radix trie #3.3: " << ctrie3.root_node.debug_representation_json() << std::endl;
    assert(ctrie3.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"t\":{\"0(1)(0)\":{\"e\":{\"0(1)(0)\":{\"am\":{\"2(2)(0)\":{}},\"st\":{\"1(1)(0)\":{}}}},\"oast\":{\"3(3)(0)\":{}}}}}}");

    // Additional custom tests apart from Wikipedia.
    // Convert splitting node to factor node after single byte edge
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie3, Slice("t")));
    std::cout << "Counted radix trie #3.4: " << ctrie3.root_node.debug_representation_json() << std::endl;
    assert(ctrie3.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"t\":{\"4(4)(0)\":{\"e\":{\"0(1)(0)\":{\"am\":{\"2(2)(0)\":{}},\"st\":{\"1(1)(0)\":{}}}},\"oast\":{\"3(3)(0)\":{}}}}}}");
    // Assert stays same if inserted again
    assert(!std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie3, Slice("t")));
    assert(ctrie3.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"t\":{\"4(4)(1)\":{\"e\":{\"0(1)(0)\":{\"am\":{\"2(2)(0)\":{}},\"st\":{\"1(1)(0)\":{}}}},\"oast\":{\"3(3)(0)\":{}}}}}}");

    // Add factor node for first character of edge
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie3, Slice("to")));
    std::cout << "Counted radix trie #3.5: " << ctrie3.root_node.debug_representation_json() << std::endl;
    assert(ctrie3.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"t\":{\"4(4)(1)\":{\"e\":{\"0(1)(0)\":{\"am\":{\"2(2)(0)\":{}},\"st\":{\"1(1)(0)\":{}}}},\"o\":{\"6(6)(0)\":{\"ast\":{\"3(3)(0)\":{}}}}}}}}");

    // Add factor node for the last character of edge (converting splitting node to factor node)
    CountedRadixTrie ctrie4;
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie4, Slice("toast")));
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie4, Slice("tool")));
    std::cout << "Counted radix trie #4.1: " << ctrie4.root_node.debug_representation_json() << std::endl;
    assert(ctrie4.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"to\":{\"0(1)(0)\":{\"ast\":{\"1(1)(0)\":{}},\"ol\":{\"2(2)(0)\":{}}}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie4, Slice("to")));
    std::cout << "Counted radix trie #4.2: " << ctrie4.root_node.debug_representation_json() << std::endl;
    assert(ctrie4.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"to\":{\"3(3)(0)\":{\"ast\":{\"1(1)(0)\":{}},\"ol\":{\"2(2)(0)\":{}}}}}}");
    // Assert stays same if inserted again
    assert(!std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie4, Slice("to")));
    assert(ctrie4.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"to\":{\"3(3)(1)\":{\"ast\":{\"1(1)(0)\":{}},\"ol\":{\"2(2)(0)\":{}}}}}}");

    // Test removal
    CountedRadixTrie ctrie5;
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie5, Slice("test")));
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie5, Slice("slow")));
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie5, Slice("water")));
    std::cout << "Counted radix trie #5.1: " << ctrie5.root_node.debug_representation_json() << std::endl;
    assert(ctrie5.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"slow\":{\"2(2)(0)\":{}},\"test\":{\"1(1)(0)\":{}},\"water\":{\"3(3)(0)\":{}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie5, Slice("water")));
    std::cout << "Counted radix trie #5.2: " << ctrie5.root_node.debug_representation_json() << std::endl;
    assert(ctrie5.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"slow\":{\"2(2)(0)\":{}},\"test\":{\"1(1)(0)\":{}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie5, Slice("tester")));
    std::cout << "Counted radix trie #5.3: " << ctrie5.root_node.debug_representation_json() << std::endl;
    assert(ctrie5.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"slow\":{\"2(2)(0)\":{}},\"test\":{\"1(1)(0)\":{\"er\":{\"3(3)(0)\":{}}}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie5, Slice("test")));
    std::cout << "Counted radix trie #5.4: " << ctrie5.root_node.debug_representation_json() << std::endl;
    assert(ctrie5.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"slow\":{\"2(2)(0)\":{}},\"tester\":{\"3(3)(0)\":{}}}}");

    CountedRadixTrie ctrie6;
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie6, Slice("a")));
    assert(!std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie6, Slice("a")));
    assert(!std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie6, Slice("a")));
    std::cout << "Counted radix trie #6.1: " << ctrie6.root_node.debug_representation_json() << std::endl;
    assert(ctrie6.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"a\":{\"1(1)(2)\":{}}}}");
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie6, Slice("a")));
    assert(ctrie6.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"a\":{\"1(1)(1)\":{}}}}");
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie6, Slice("a")));
    assert(ctrie6.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"a\":{\"1(1)(0)\":{}}}}");
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie6, Slice("a")));
    std::cout << "Counted radix trie #6.2: " << ctrie6.root_node.debug_representation_json() << std::endl;
    assert(ctrie6.root_node.debug_representation_json() == "{\"0(0)(0)\":{}}");

    CountedRadixTrie ctrie7;
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("ab")));
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("abb")));
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("aba")));
    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("abbb")));
    assert(!std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("ab")));
    std::cout << "Counted radix trie #7.1: " << ctrie7.root_node.debug_representation_json() << std::endl;
    assert(ctrie7.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"ab\":{\"1(1)(1)\":{\"a\":{\"3(3)(0)\":{}},\"b\":{\"2(2)(0)\":{\"b\":{\"4(4)(0)\":{}}}}}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("abbbaba")));
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie7, Slice("abbbaba")));
    assert(ctrie7.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"ab\":{\"1(1)(1)\":{\"a\":{\"3(3)(0)\":{}},\"b\":{\"2(2)(0)\":{\"b\":{\"4(4)(0)\":{}}}}}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("abbbab")));
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie7, Slice("abbbab")));
    assert(ctrie7.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"ab\":{\"1(1)(1)\":{\"a\":{\"3(3)(0)\":{}},\"b\":{\"2(2)(0)\":{\"b\":{\"4(4)(0)\":{}}}}}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("abbbab")));
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie7, Slice("abbbab")));
    assert(ctrie7.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"ab\":{\"1(1)(1)\":{\"a\":{\"3(3)(0)\":{}},\"b\":{\"2(2)(0)\":{\"b\":{\"4(4)(0)\":{}}}}}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("abbba")));
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie7, Slice("abbba")));
    assert(ctrie7.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"ab\":{\"1(1)(1)\":{\"a\":{\"3(3)(0)\":{}},\"b\":{\"2(2)(0)\":{\"b\":{\"4(4)(0)\":{}}}}}}}}");

    assert(!std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("abbb")));
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie7, Slice("abbb")));
    assert(ctrie7.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"ab\":{\"1(1)(1)\":{\"a\":{\"3(3)(0)\":{}},\"b\":{\"2(2)(0)\":{\"b\":{\"4(4)(0)\":{}}}}}}}}");

    assert(!std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("abb")));
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie7, Slice("abb")));
    assert(ctrie7.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"ab\":{\"1(1)(1)\":{\"a\":{\"3(3)(0)\":{}},\"b\":{\"2(2)(0)\":{\"b\":{\"4(4)(0)\":{}}}}}}}}");

    assert(!std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("ab")));
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie7, Slice("ab")));
    assert(ctrie7.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"ab\":{\"1(1)(1)\":{\"a\":{\"3(3)(0)\":{}},\"b\":{\"2(2)(0)\":{\"b\":{\"4(4)(0)\":{}}}}}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("a")));
    std::cout << "Counted radix trie #7.2: " << ctrie7.root_node.debug_representation_json() << std::endl;
    assert(std_flexible_lzdr_radix_trie_internal::remove_from_radix_trie(ctrie7, Slice("a")));
    std::cout << "Counted radix trie #7.3: " << ctrie7.root_node.debug_representation_json() << std::endl;
    assert(ctrie7.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"ab\":{\"1(1)(1)\":{\"a\":{\"3(3)(0)\":{}},\"b\":{\"2(2)(0)\":{\"b\":{\"4(4)(0)\":{}}}}}}}}");

    assert(std_flexible_lzdr_radix_trie_internal::insert_into_radix_trie(ctrie7, Slice("abbbab")));
    std::cout << "Counted radix trie #7.4: " << ctrie7.root_node.debug_representation_json() << std::endl;
    assert(ctrie7.root_node.debug_representation_json() == "{\"0(0)(0)\":{\"ab\":{\"1(1)(1)\":{\"a\":{\"3(3)(0)\":{}},\"b\":{\"2(2)(0)\":{\"b\":{\"4(4)(0)\":{\"ab\":{\"6(6)(0)\":{}}}}}}}}}}");
}
