#ifndef LIST_H
#define LIST_H
/* List.h
 *
 * doubly-linked, double-ended list with Iterator interface
 * Project UID c1f28c309e55405daf00c565d57ff9ad
 * EECS 280 Project 4
 */

#include <iostream>
#include <cassert> //assert
#include <cstddef> //NULL


template <typename T>
class List {
  //OVERVIEW: a doubly-linked, double-ended list with Iterator interface

private:
   //a private type
   struct Node {
      Node *next;
      Node *prev;
      T datum;

   Node( T datum ) : next( nullptr ), prev( nullptr), datum( datum )
      {
      }
   };


   Node* first;   // points to first Node in list, or nullptr if list is empty
   Node* last;    // points to last Node in list, or nullptr if list is empty
   int count;     // number of nodes on the list.

public:

  //EFFECTS:  returns true if the list is empty
  bool empty( ) const
      {
      return !first;
      }

  //EFFECTS: returns the number of elements in this List
  int size( ) const
      {
      return count;
      }

  //REQUIRES: list is not empty
  //EFFECTS: Returns the first element in the list by reference
  T &front( )
      {
      assert( first );
      return first->datum;
      }

  //REQUIRES: list is not empty
  //EFFECTS: Returns the last element in the list by reference
  T &back( )
      {
      assert( last );
      return last->datum;
      }

  //EFFECTS:  inserts datum into the front of the list
  void push_front( const T &datum )
      {
      Node *n = new Node( datum );
      n->next = first;
      if ( first )
         first->prev = n;
      else
         last = n;
      first = n;
      count++;
      } 

  //EFFECTS:  inserts datum into the back of the list
  void push_back( const T &datum )
      {
      Node *n = new Node( datum );
      n->next = nullptr;
      if ( last )
         {
         last->next = n;
         n->prev = last;
         }
      else
         first = n;
      last = n;
      count++;
      }

  //REQUIRES: list is not empty
  //MODIFIES: may invalidate list iterators
  //EFFECTS:  removes the item at the front of the list
  void pop_front( )
      {
      assert( first );
      Node *n = first;
      first = first->next;
      if ( first )
         first->prev = nullptr;
      else
         last = nullptr;
      delete n;
      count--;
      }

  //REQUIRES: list is not empty
  //MODIFIES: may invalidate list iterators
  //EFFECTS:  removes the item at the back of the list
  void pop_back( )
      {
      assert( last );
      Node *n = last;
      last = last->prev;
      if ( last )
         last->next = nullptr;
      else
         first = nullptr;
      delete n;
      count--;
      }

   //MODIFIES: may invalidate list iterators
   //EFFECTS:  removes all nodes
   void pop_all( )
      {
      Node *n;
      while ( ( n = first ) )
         {
         first = first->next;
         delete n;
         }
      last = nullptr;
      count = 0;
      }

   //EFFECTS:  copies all nodes from other onto the
   // end of this
   void copy_all( const List<T>& other )
      {
      for ( Node *n = other.first;  n;  n = n->next )
         push_back( n->datum );
      }


  // You should add in a default constructor, destructor, copy constructor,
  // and overloaded assignment operator, if appropriate. If these operations
  // will work correctly without defining these, you can omit them. A user
  // of the class must be able to create, copy, assign, and destroy Lists

  List( ) : first( nullptr ), last( nullptr ), count( 0 )
      {
      }

  ~List( )
      {
      pop_all( );
      }

  List( const List &other ) : List( )
      {
      copy_all( other );
      }

  List &operator=( List &rhs )
      {
      if ( &rhs != this )
         {
         pop_all( );
         for ( Node* n = rhs.first; n; n = n->next )
            push_back( n->datum );
         }
      return *this;
      }

  

  ////////////////////////////////////////
  class Iterator {
       //OVERVIEW: Iterator interface to List

       // You should add in a default constructor, destructor, copy constructor,
       // and overloaded assignment operator, if appropriate. If these operations
       // will work correctly without defining these, you can omit them. A user
       // of the class must be able to create, copy, assign, and destroy Iterators.

       // Your iterator should implement the following public operators: *,
       // ++ (prefix), default constructor, == and !=.

     public:
       // This operator will be used to test your code. Do not modify it.
       // Requires that the current element is dereferenceable.
       Iterator &operator--( )
            {
            assert(node_ptr);
            node_ptr = node_ptr->prev;
            return *this;
            }

       T &operator*( )
            {
            return node_ptr->datum;
            }

       Iterator &operator++( )
            {
            assert( node_ptr );
            node_ptr = node_ptr->next;
            return *this;
            }

       bool operator==( const Iterator &rhs ) const
            {
            return node_ptr == rhs.node_ptr;
            }

       bool operator!=( const Iterator &rhs ) const
            {
            return node_ptr != rhs.node_ptr;
            }

     private:
      Node *node_ptr; //current Iterator position is a List node
       // add any additional necessary member variables here

       // add any friend declarations here
       friend class List;

       // construct an Iterator at a specific position
       Iterator( Node *p = nullptr ) : node_ptr( p )
         {
         }

     };//List::Iterator
     ////////////////////////////////////////

  // return an Iterator pointing to the first element
  Iterator begin( ) const
      {
      return Iterator( first );
      }

  // return an Iterator pointing to "past the end"
  Iterator end( ) const
      {
      return Iterator( );
      }

  //REQUIRES: i is a valid, dereferenceable iterator associated with this list
  //MODIFIES: may invalidate other list iterators
  //EFFECTS: Removes a single element from the list container
  void erase( Iterator i )
      {
      Node *n = i.node_ptr;
      assert( n );
      if ( n->prev )
         n->prev->next = n->next;
      else
         first = n->next;
      if ( n->next )
         n->next->prev = n->prev;
      else
         last = n->prev;
      delete n;
      count--;
      }


  //REQUIRES: i is a valid iterator associated with this list
  //EFFECTS: inserts datum before the element at the specified position.
  void insert( Iterator i, const T &datum )
      {
      Node *in = i.node_ptr;
      if ( in )
         {
         Node *n = new Node( datum );
         if ( in->prev )
            in->prev->next = n;
         else
            first = n;
         n->next = in;
         n->prev = in->prev;
         in->prev = n;
         count++;
         }
      else
         // Adding to the end of the list.
         push_back( datum );
      }

};//List


////////////////////////////////////////////////////////////////////////////////
// Add your member function implementations below or in the class above
// (your choice). Do not change the public interface of List, although you
// may add the Big Three if needed.  Do add the public member functions for
// Iterator.


#endif // Do not remove this. Write all your code above this line.
