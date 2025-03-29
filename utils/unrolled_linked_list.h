#pragma once

/*

More efficient DS for posting list.

Cons of another methods:

Single linked-list = high memory consumption

Fixed-size array = len of each terms posting list is not known ahead of time,
would require two passes over input data.

Solution: Unrolled link list
next pointer points to a group of posts

*/

#include <cassert>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>
#include <algorithm>

constexpr float PRE_ALLOC_FACTOR = 1.2f; // TEMP
constexpr uint8_t MAX_LIMIT = 64;      // TEMP
constexpr uint8_t START_SIZE = 4;      // TEMP

template <typename T>
class UnrolledLinkList {
public:
    struct Node {
        Node* next;
        std::vector<T> group;

        explicit Node(uint8_t group_size) : next(nullptr) {
            group.reserve(group_size);
        }
    };

    UnrolledLinkList() : num_elements(0), curr_capacity(0), first(nullptr), last(nullptr) {}

    ~UnrolledLinkList() { clear(); }

    bool empty() const { return (first == nullptr); }

    size_t size() const { return num_elements; }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (empty()) {
            first = create_new_node(START_SIZE, std::forward<Args>(args)...);
            last = first;
            curr_capacity += START_SIZE;
        } else {
            if (last->group.size() < last->group.capacity()) {
                last->group.emplace_back(std::forward<Args>(args)...);
            } else {
                uint8_t new_size = std::min(MAX_LIMIT, std::max(START_SIZE,
                    static_cast<uint8_t>((PRE_ALLOC_FACTOR - 1) * curr_capacity)));
                curr_capacity += new_size;
                last->next = create_new_node(new_size, std::forward<Args>(args)...);
                last = last->next;
            }
        }
        ++num_elements;
    }

    void push_back(T&& datum) {
        emplace_back(std::move(datum));
    }

    void pop_front() {
        if (empty()) return;

        Node* to_delete = first;
        num_elements -= first->group.size();
        first = first->next;
        delete to_delete;

        if (!first) {
            last = nullptr;
            curr_capacity = 0;
        }
    }

    void clear() {
        while (first) {
            pop_front();
        }
    }

    class Iterator {
        friend class UnrolledLinkList;
    public:
        Iterator() : node_ptr(nullptr), curr_index(0) {}

        T& operator*() {
            assert(node_ptr);
            return node_ptr->group[curr_index];
        }

        bool operator==(const Iterator& rhs) const {
            return (node_ptr == rhs.node_ptr && curr_index == rhs.curr_index);
        }

        bool operator!=(const Iterator& rhs) const { return !(*this == rhs); }

        Iterator& operator++() {
            if (!node_ptr) return *this;

            ++curr_index;
            if (curr_index >= node_ptr->group.size()) {
                if (node_ptr->next)
                    node_ptr = node_ptr->next;
                else
                    node_ptr = nullptr;
                curr_index = 0;
            }
            return *this;
        }

    private:
        Node* node_ptr;
        size_t curr_index;
        Iterator(Node* p, size_t i) : node_ptr(p), curr_index(i) {}
    };

    Iterator begin() const { return Iterator(first, 0); }

    Iterator end() const { return Iterator(); }

private:
    template<typename... Args>
    Node* create_new_node(uint8_t group_size, Args&&... args) {
        Node* new_node = new Node(group_size);
        new_node->group.emplace_back(std::forward<Args>(args)...);
        return new_node;
    }

    uint64_t num_elements;
    uint64_t curr_capacity;
    Node* first;
    Node* last;
};