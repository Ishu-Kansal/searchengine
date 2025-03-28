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

#include "cunique_ptr.h"


constexpr float PRE_ALLOC_FACTOR = 1.2; // TEMP
constexpr uint32_t MAX_LIMIT = 64;      // TEMP
constexpr u_int32_t START_SIZE = 4;     // TEMP

template <typename T>
class UnrolledLinkList
{
public:
    struct Node
    {
        Node *next;

        uint32_t num_group_elements;
        uint32_t max_group_elements;
        cunique_ptr<T[]> group;
    };

    bool empty() const;

    size_t size() const;

    void push_back(T &&datum);

    void push_sentinel();

    void pop_front();

    void clear();

    UnrolledLinkList()
        : num_groups(0), num_elements(0), first(nullptr), last(nullptr) {}

    ~UnrolledLinkList() { clear(); }

private:
    uint32_t num_groups = 0;
    uint32_t num_elements = 0;
    uint32_t curr_size = 0;

    Node *first;
    Node *last;

public:
    class Iterator
    {
        friend class UnrolledLinkList;

    public:
        Iterator() : node_ptr(nullptr) {}
        T &operator*()
        {
            assert(node_ptr);

            return node_ptr->group[curr_index];
        }
        bool operator==(Iterator rhs) const { return node_ptr == rhs.node_ptr; }
        bool operator!=(Iterator rhs) const { return !(node_ptr == rhs.node_ptr); }
        Iterator &operator++()
        {
            if (!node_ptr)
                return *this;

            if (++curr_index >= node_ptr->num_group_elements)
            {
                node_ptr = node_ptr->next;
                curr_index = 0;
            }
            return *this;
        }

    private:
        Node *node_ptr;
        size_t curr_index = 0;
        // construct an Iterator at a specific position
        Iterator(Node *p, size_t i) : node_ptr(p), curr_index(i) {}
    };

    // return an Iterator pointing to the first element
    Iterator begin() const { return Iterator(first, 0); }

    // return an Iterator pointing to "past the end"
    Iterator end() const { return Iterator(); }
};

template <typename T>
bool UnrolledLinkList<T>::empty() const
{
    return (num_groups == 0);
}
template <typename T>
size_t UnrolledLinkList<T>::size() const
{
    return num_elements;
}

template <typename T>
void UnrolledLinkList<T>::push_back(T &&datum)
{
    auto create_new_node = [](uint32_t group_size, T &&d) -> Node *
    {
        auto new_node = new Node;
        new_node->num_group_elements = 1;
        new_node->max_group_elements = group_size;
        new_node->group = new T[group_size];
        new_node->group[0] = std::move(d);
        new_node->next = nullptr;
        return new_node;
    };

    if (empty())
    {
        first = last = create_new_node(START_SIZE, std::move(datum));
        last->next = nullptr;
        ++num_groups;
        curr_size += START_SIZE;
    }
    else
    {
        if (last->num_group_elements < last->max_group_elements)
        {
            last->group[last->num_group_elements++] = std::move(datum);
        }
        else
        {
            uint32_t new_size = std::min(MAX_LIMIT, std::max(START_SIZE,
                static_cast<uint32_t>((PRE_ALLOC_FACTOR - 1) * curr_size)));
            curr_size += new_size;
            last->next = create_new_node(new_size, std::move(datum));
            last = last->next;
            last->next = nullptr;
            ++num_groups;
        }
    }
    ++num_elements;
}

template <typename T>
void UnrolledLinkList<T>::pop_front() {
    Node *to_delete = first;
    first = first->next;
    if (first == nullptr)
    {
        last = nullptr;
    }
    num_elements -= to_delete->max_group_elements
    num_groups--;
    delete[] to_delete->group;
    delete to_delete;

}

template <typename T>
void UnrolledLinkList<T>::clear() {
    while(!empty())
    {
        pop_front();
    }
    curr_size = 0;
}

/*
template <typename T>
void UnrolledLinkList<T>::push_sentinel()
{
    assert(!empty());
    auto new_node = make_cunique<Node>();
    new_node->max_group_elements = 0;
    new_node->num_group_elements = 0;
    new_node->group = nullptr;
}
#endif
*/

