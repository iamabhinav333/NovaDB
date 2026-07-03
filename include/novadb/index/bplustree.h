#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace novadb::index {

class BPlusTree {
public:
    explicit BPlusTree(std::size_t max_keys_per_node = 4);
    BPlusTree(const BPlusTree& other);
    BPlusTree& operator=(const BPlusTree& other);
    BPlusTree(BPlusTree&&) noexcept = default;
    BPlusTree& operator=(BPlusTree&&) noexcept = default;
    ~BPlusTree();

    bool insert(std::uint64_t key, std::string value);
    bool update(std::uint64_t key, std::string value);
    bool remove(std::uint64_t key);

    std::optional<std::string> search(std::uint64_t key) const;
    std::vector<std::pair<std::uint64_t, std::string>> range_query(std::uint64_t start_key, std::uint64_t end_key) const;

    std::size_t size() const;

private:
    struct Node {
        bool leaf = true;
        std::vector<std::uint64_t> keys;
        std::vector<std::string> values;
        std::vector<std::unique_ptr<Node>> children;
        Node* parent = nullptr;
        Node* next_leaf = nullptr;
    };

    std::size_t max_keys_;
    std::map<std::uint64_t, std::string> entries_;
    std::unique_ptr<Node> root_;

    void rebuild_tree();
    std::unique_ptr<Node> build_leaf(std::vector<std::pair<std::uint64_t, std::string>>::const_iterator begin,
                                     std::vector<std::pair<std::uint64_t, std::string>>::const_iterator end);
    std::unique_ptr<Node> build_internal_level(std::vector<std::unique_ptr<Node>>& level);
    std::unique_ptr<Node> build_from_entries(const std::vector<std::pair<std::uint64_t, std::string>>& entries);

    const Node* descend_to_leaf(std::uint64_t key) const;
    Node* descend_to_leaf(std::uint64_t key);
};

}  // namespace novadb::index
