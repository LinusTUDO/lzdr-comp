#include "radix_trie.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

std::string RadixTrieNode::debug_representation_json() const {
    std::string result;
    result.append("{");
    result.append("\"");
    result.append(std::to_string(index));
    result.append("(");
    result.append(std::to_string(next_factor_node_index));
    result.append(")");
    result.append("\"");
    result.append(":");
    result.append("{");

    // Sort edges by edge text to make result consistent
    std::vector<std::pair<std::string, const RadixTrieNode*>> edges_list;
    for (auto &pair : edges) {
        std::string edge_text;
        edge_text.push_back(static_cast<char>(pair.first));
        for (size_t i = 0; i < pair.second.rest_text.size(); ++i) {
            edge_text.push_back(static_cast<char>(pair.second.rest_text[i]));
        }
        edges_list.emplace_back(edge_text, &pair.second.end_node);
    }
    std::sort(edges_list.begin(), edges_list.end(),
        [](const std::pair<std::string, const RadixTrieNode*>& a, const std::pair<std::string, const RadixTrieNode*>& b) {
            return a.first < b.first;
        });

    // Add edges to string
    bool first_edge = true;
    for (const auto& pair : edges_list) {
        if (!first_edge) {
            result.append(",");
        } else {
            first_edge = false;
        }
        result.append("\"");
        result.append(pair.first);
        result.append("\"");
        result.append(":");
        result.append(pair.second->debug_representation_json());
    }

    result.append("}");
    result.append("}");
    return result;
}

std::string CountedRadixTrieNode::debug_representation_json() const {
    std::string result;
    result.append("{");
    result.append("\"");
    result.append(std::to_string(index));
    result.append("(");
    result.append(std::to_string(next_factor_node_index));
    result.append(")");
    result.append("(");
    result.append(std::to_string(extra_count));
    result.append(")");
    result.append("\"");
    result.append(":");
    result.append("{");

    // Sort edges by edge text to make result consistent
    std::vector<std::pair<std::string, const CountedRadixTrieNode*>> edges_list;
    for (auto &pair : edges) {
        std::string edge_text;
        edge_text.push_back(static_cast<char>(pair.first));
        for (const auto rest_char : pair.second.rest_text) {
            edge_text.push_back(static_cast<char>(rest_char));
        }
        edges_list.emplace_back(edge_text, &pair.second.end_node);
    }
    std::sort(edges_list.begin(), edges_list.end(),
        [](const std::pair<std::string, const CountedRadixTrieNode*>& a, const std::pair<std::string, const CountedRadixTrieNode*>& b) {
            return a.first < b.first;
        });

    // Add edges to string
    bool first_edge = true;
    for (const auto& pair : edges_list) {
        if (!first_edge) {
            result.append(",");
        } else {
            first_edge = false;
        }
        result.append("\"");
        result.append(pair.first);
        result.append("\"");
        result.append(":");
        result.append(pair.second->debug_representation_json());
    }

    result.append("}");
    result.append("}");
    return result;
}
