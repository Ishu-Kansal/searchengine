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
\
#include "cunique_ptr.h"

constexpr float PRE_ALLOC_FACTOR = 1.2; // TEMP
constexpr uint32_t MAX_LIMIT = 64; // TEMP 
constexpr u_int32_t START_SIZE = 4; // TEMP

template <typename T>
class UnrolledLinkList
{
public:
    bool empty() const;

    int size() const;

    void push_back(const T &datum);

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
        template <typename T>
        struct Node
        {
            Node *next;

            uint32_t num_group_elements;
            uint32_t max_group_elements;
            T * group;
        };

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
            // TODO
            assert(node_ptr);
            return 0;
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
            // TODO
            return 0;
        }

    private:
    Node *node_ptr;

    // construct an Iterator at a specific position
    Iterator(Node *p) : node_ptr(p) {}
    };


    // return an Iterator pointing to the first element
    Iterator begin() const
    {
        return Iterator(first);
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
    void UnrolledLinkList<T>::push_back(const T &datum)
    {
        if (empty())
        {
            // Makes new node and init vars
            cunique_ptr<Node> p = make_cunique<Node>Node;
            p->num_group_elements = 0;
            p->max_group_elements = START_SIZE;
            p->group = make_cunique<T[]>(START_SIZE);

            p->group[0] = datum;
            p->next = nullptr;
            p->num_group_elements++;
            first = last = p;
            ++num_groups;  
            curr_size + START_SIZE;
        }
        else
        {
            // Node isn't filled yet, so add to current node
            if (last->num_group_elements < last->max_group_elements)
            {
                last->group[last->num_group_elements++] = datum;
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
                cunique_ptr<Node> p = make_cunique<Node>Node;
                p->max_group_elements = new_size;
                p->num_group_elements = 0;
                p->group = make_cunique<T[]>(new_size);
                
                p->group[0] = datum;
                p->next = nullptr;
                
                p->num_group_elements++;
                last->next = p;
                last = p;
                ++num_groups;
            }
        }
        ++num_elements;
    }

#endif
