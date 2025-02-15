#ifndef RADIX_TRIE_H
#define RADIX_TRIE_H
#include "slice.h"

#include <cstddef>
#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>

class RadixTrieEdge;

class RadixTrieNode {
    explicit RadixTrieNode(const size_t index, const size_t next_factor_node_index) : index(index), next_factor_node_index(next_factor_node_index) {
    }

public:
    // If the index is 0, it means that this node is either the root node or a splitting node (i.e. non-factor node)
    size_t index;
    // The next factor node index
    // * If this is the root node, this is equal to 0
    // * If this is a factor node, this is equal to the index of this node
    // * If this is a splitting node, this is equal to the index of one of this node's (maybe indirect) factor node children
    size_t next_factor_node_index;
    std::unordered_map<uint8_t, RadixTrieEdge> edges;

    static RadixTrieNode create_root_node() {
        return RadixTrieNode(0, 0);
    }

    static RadixTrieNode create_factor_node(const size_t index) {
        return RadixTrieNode(index, index);
    }

    static RadixTrieNode create_splitting_node(const size_t next_factor_node_index) {
        return RadixTrieNode(0, next_factor_node_index);
    }

    RadixTrieNode copy_without_edges() const {
        return RadixTrieNode(index, next_factor_node_index);
    }

    std::string debug_representation_json() const;
};

class RadixTrieEdge {
public:
    RadixTrieNode end_node;
    Slice rest_text;

    RadixTrieEdge(RadixTrieNode end_node, const Slice rest_text) : end_node(std::move(end_node)), rest_text(rest_text) {
    }
};

class RadixTrie {
public:
    RadixTrieNode root_node;
    // The number of factor nodes, including the root node
    size_t num_factor_nodes;

    RadixTrie() : root_node(RadixTrieNode::create_root_node()), num_factor_nodes(1) {
    }
};

// Alternative version of radix trie where nodes are counted:

class CountedRadixTrieEdge;

class CountedRadixTrieNode {
    explicit CountedRadixTrieNode(const size_t index, const size_t next_factor_node_index) : index(index), next_factor_node_index(next_factor_node_index), extra_count(0) {
    }

    explicit CountedRadixTrieNode(const size_t index, const size_t next_factor_node_index, const size_t extra_count) : index(index), next_factor_node_index(next_factor_node_index), extra_count(extra_count) {
    }

public:
    // If the index is 0, it means that this node is either the root node or a splitting node (i.e. non-factor node)
    size_t index;
    // The next factor node index
    // * If this is the root node, this is equal to 0
    // * If this is a factor node, this is equal to the index of this node
    // * If this is a splitting node, this is equal to the index of one of this node's (maybe indirect) factor node children
    size_t next_factor_node_index;
    std::unordered_map<uint8_t, CountedRadixTrieEdge> edges;
    // The number of additional insertions of the same text.
    // If a text is inserted once, the extra_count is 0.
    // If inserted twice, the extra_count is 1.
    // Should be 0 for splitting nodes.
    size_t extra_count;

    static CountedRadixTrieNode create_root_node() {
        return CountedRadixTrieNode(0, 0);
    }

    static CountedRadixTrieNode create_factor_node(const size_t index) {
        return CountedRadixTrieNode(index, index);
    }

    static CountedRadixTrieNode create_splitting_node(const size_t next_factor_node_index) {
        return CountedRadixTrieNode(0, next_factor_node_index);
    }

    CountedRadixTrieNode copy_without_edges() const {
        return CountedRadixTrieNode(index, next_factor_node_index, extra_count);
    }

    std::string debug_representation_json() const;
};

class CountedRadixTrieEdge {
public:
    CountedRadixTrieNode end_node;
    std::list<uint8_t> rest_text;

    CountedRadixTrieEdge(CountedRadixTrieNode end_node, std::list<uint8_t> rest_text) : end_node(std::move(end_node)),
                                                                          rest_text(std::move(rest_text)) {
    }
};

class CountedRadixTrie {
public:
    CountedRadixTrieNode root_node;
    // The number of factor nodes, including the root node, plus the number of extra counts.
    // For example, if there are two factor nodes besides the root node, and both of them have an extra count
    // of 2, then num_factor_nodes is equal to 1 (root node) + 1 (other node 1) + 1 (other node 2)
    // + 2 (extra count of 1) + 2 (extra count of 2) = 7.
    size_t num_factor_nodes;

    CountedRadixTrie() : root_node(CountedRadixTrieNode::create_root_node()), num_factor_nodes(1) {
    }
};

#endif //RADIX_TRIE_H
