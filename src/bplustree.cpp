#include "novadb/index/bplustree.h"

#include <algorithm>
#include <iterator>
#include <utility>
#include <stdexcept>

namespace novadb::index {

BPlusTree::BPlusTree(std::size_t max_keys_per_node) : max_keys_(max_keys_per_node) {
    if (max_keys_ < 2) {
        throw std::invalid_argument("BPlusTree requires at least two keys per node");
    }
    rebuild_tree();
}

BPlusTree::BPlusTree(const BPlusTree& other) : max_keys_(other.max_keys_), entries_(other.entries_) {
    rebuild_tree();
}

BPlusTree& BPlusTree::operator=(const BPlusTree& other) {
    if (this != &other) {
        max_keys_ = other.max_keys_; 
        entries_ = other.entries_;
        rebuild_tree();
    }
    return *this;
}

BPlusTree::~BPlusTree() = default;

bool BPlusTree::insert(std::uint64_t key, std::string value) {
    auto [it, inserted] = entries_.emplace(key, std::move(value));
    if (!inserted) {
        return false;
    }
    rebuild_tree();
    return true;
}

bool BPlusTree::update(std::uint64_t key, std::string value) {
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return false;
    }
    it->second = std::move(value);
    rebuild_tree();
    return true;
}

bool BPlusTree::remove(std::uint64_t key) {
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return false;
    }
    entries_.erase(it);
    rebuild_tree();
    return true;
}

std::optional<std::string> BPlusTree::search(std::uint64_t key) const {
    const Node* leaf = descend_to_leaf(key);
    if (leaf == nullptr) {
        return std::nullopt;
    }

    const auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it == leaf->keys.end() || *it != key) {
        return std::nullopt;
    }

    const std::size_t index = static_cast<std::size_t>(std::distance(leaf->keys.begin(), it));
    return leaf->values[index];
}

std::vector<std::pair<std::uint64_t, std::string>> BPlusTree::range_query(std::uint64_t start_key,
                                                                           std::uint64_t end_key) const {
    std::vector<std::pair<std::uint64_t, std::string>> results;
    if (entries_.empty() || start_key > end_key) {
        return results;
    }

    const Node* leaf = descend_to_leaf(start_key);
    if (leaf == nullptr) {
        return results;
    }

    std::size_t index = static_cast<std::size_t>(std::lower_bound(leaf->keys.begin(), leaf->keys.end(), start_key) -
                                                leaf->keys.begin());

    while (leaf != nullptr) {
        while (index < leaf->keys.size()) {
            const std::uint64_t key = leaf->keys[index];
            if (key > end_key) {
                return results;
            }
            results.emplace_back(key, leaf->values[index]);
            ++index;
        }
        leaf = leaf->next_leaf;
        index = 0;
    }

    return results;
}

std::size_t BPlusTree::size() const { return entries_.size(); }

void BPlusTree::rebuild_tree() {
    std::vector<std::pair<std::uint64_t, std::string>> ordered_entries(entries_.begin(), entries_.end());
    root_ = build_from_entries(ordered_entries);
}

std::unique_ptr<BPlusTree::Node> BPlusTree::build_leaf(
    std::vector<std::pair<std::uint64_t, std::string>>::const_iterator begin,
    std::vector<std::pair<std::uint64_t, std::string>>::const_iterator end) {
    auto node = std::make_unique<Node>();
    node->leaf = true;
    for (auto it = begin; it != end; ++it) {
        node->keys.push_back(it->first);
        node->values.push_back(it->second);
    }
    return node;
}

std::unique_ptr<BPlusTree::Node> BPlusTree::build_internal_level(std::vector<std::unique_ptr<Node>>& level) {
    if (level.empty()) {
        return std::make_unique<Node>();
    }

    const std::size_t max_children = max_keys_ + 1;
    std::vector<std::unique_ptr<Node>> next_level;
    next_level.reserve((level.size() + max_children - 1) / max_children);

    for (std::size_t start = 0; start < level.size(); start += max_children) {
        const std::size_t end = std::min(level.size(), start + max_children);
        auto parent = std::make_unique<Node>();
        parent->leaf = false;
        parent->children.reserve(end - start);

        for (std::size_t index = start; index < end; ++index) {
            level[index]->parent = parent.get();
            parent->children.push_back(std::move(level[index]));
        }

        for (std::size_t index = 1; index < parent->children.size(); ++index) {
            parent->keys.push_back(parent->children[index]->keys.front());
        }

        next_level.push_back(std::move(parent));
    }

    while (next_level.size() > 1) {
        level = std::move(next_level);
        next_level.clear();
        next_level.reserve((level.size() + max_children - 1) / max_children);

        for (std::size_t start = 0; start < level.size(); start += max_children) {
            const std::size_t end = std::min(level.size(), start + max_children);
            auto parent = std::make_unique<Node>();
            parent->leaf = false;
            parent->children.reserve(end - start);

            for (std::size_t index = start; index < end; ++index) {
                level[index]->parent = parent.get();
                parent->children.push_back(std::move(level[index]));
            }

            for (std::size_t index = 1; index < parent->children.size(); ++index) {
                parent->keys.push_back(parent->children[index]->keys.front());
            }

            next_level.push_back(std::move(parent));
        }
    }

    return next_level.empty() ? std::move(level.front()) : std::move(next_level.front());
}

std::unique_ptr<BPlusTree::Node> BPlusTree::build_from_entries(
    const std::vector<std::pair<std::uint64_t, std::string>>& entries) {
    if (entries.empty()) {
        return std::make_unique<Node>();
    }

    std::vector<std::unique_ptr<Node>> level;
    level.reserve((entries.size() + max_keys_ - 1) / max_keys_);

    for (std::size_t start = 0; start < entries.size(); start += max_keys_) {
        const std::size_t end = std::min(entries.size(), start + max_keys_);
        level.push_back(build_leaf(entries.begin() + static_cast<std::ptrdiff_t>(start),
                                   entries.begin() + static_cast<std::ptrdiff_t>(end)));
    }

    for (std::size_t i = 0; i + 1 < level.size(); ++i) {
        level[i]->next_leaf = level[i + 1].get();
    }

    if (level.size() == 1) {
        return std::move(level.front());
    }

    return build_internal_level(level);
}

const BPlusTree::Node* BPlusTree::descend_to_leaf(std::uint64_t key) const {
    const Node* current = root_.get();
    while (current != nullptr && !current->leaf) {
        const auto it = std::upper_bound(current->keys.begin(), current->keys.end(), key);
        const std::size_t index = static_cast<std::size_t>(std::distance(current->keys.begin(), it));
        current = current->children[index].get();
    }
    return current;
}

BPlusTree::Node* BPlusTree::descend_to_leaf(std::uint64_t key) {
    return const_cast<Node*>(std::as_const(*this).descend_to_leaf(key));
}

}  // namespace novadb::index
