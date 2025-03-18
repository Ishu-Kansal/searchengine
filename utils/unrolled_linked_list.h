#ifndef LIST_H
#define LIST_H

/*

More efficient DS for posting list. 

Cons of another methods: 

Single linked-list = high memory consumption

Fixed-size array = len of each terms posting list is not known ahead of time,
would require two passes over input data.

Solution: Unrolled link list
next pointer points to a group of posts

*/

#include <iostream>
#include <cassert>
#include <cstddef>
#include<utility>

#include "cunique_ptr.h"

constexpr float PRE_ALLOC_FACTOR = 1.2; // TEMP
constexpr uint32_t MAX_LIMIT = 64; // TEMP 
constexpr u_int32_t START_SIZE = 4; // TEMP

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

    int size() const;

    void push_back(T &&datum);

    void push_sentinel();

    void clear(); // TODO

    UnrolledLinkList() : num_groups(0), num_of_elements(0), first(nullptr), last(nullptr) {}

    ~UnrolledLinkList()
    {
        clear();
    }

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
        bool operator==(Iterator rhs) const 
        {
            return node_ptr == rhs.node_ptr;
        }
        bool operator!= (Iterator rhs) const
        {
            return !(node_ptr == rhs.node_ptr);
        }
        Iterator & operator++()
        {  
            if (!node_ptr) return *this;


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
    Iterator(Node *, i) : node_ptr(p) , curr_index(i) {}
    };


    // return an Iterator pointing to the first element
    Iterator begin() const
    {
        return Iterator(first, 0);
    }

    // return an Iterator pointing to "past the end"
    Iterator end() const
    {
        return Iterator();
    }

};

    template <typename T>
    bool UnrolledLinkList<T>::empty() const
    {
        return (num_groups == 0);
    }
    template <typename T>
    int UnrolledLinkList<T>::size() const
    {
        return num_of_elements;
    }

    template <typename T>
    void UnrolledLinkList<T>::push_back(T &&datum)
    {
        cunique_ptr<Node> create_new_node(uint32_t group_size, T datum) {
            auto new_node = make_cunique<Node>();
            new_node->num_group_elements = 1;
            new_node->max_group_elements = group_size;
            new_node->group = make_cunique<T[]>(group_size);
            new_node->group[0] = std::move(datum);
            new_node->next = nullptr;
            return new_node;
        }
        
        if (empty())
        {
            // Makes new node and init vars
            first = last = create_new_node(START_SIZE, datum);
            last->next = nullptr;
            ++num_groups;
            curr_size += START_SIZE;

        }
        else
        {
            // Node isn't filled yet, so add to current node
            if (last->num_group_elements < last->max_group_elements)
            {
                last->group[last->num_group_elements++] = std::move(datum);
            }
            // Node is filled
            else
            {
                // Caculate new array size using 
                // min{limit, max{START_SIZE, (PRE_ALLOC_FACTOR - 1) * curr_size}}

                uint32_t new_size = std::min(MAX_LIMIT, std::max(START_SIZE, (PRE_ALLOC_FACTOR - 1)* curr_size));

                // Add new array size to curr size
                curr_size += new_size;
                
                // Makes new node and init vars
                last->next = create_new_node(new_size, datum);
                last = last->next;  // Move the last pointer to the newly added node
                last->next = nullptr;
                ++num_groups;
            }
        }
        ++num_elements;
        return last;

    }

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
