// vector.h
// 
// Starter file for a vector template

#include <iostream>
#include <new>

template<typename T>
class vector {

public:

   // Default Constructor
   // REQUIRES: Nothing
   // MODIFIES: *this
   // EFFECTS: Constructs an empty vector with capacity 0
   vector( ) : vec_capacity(0), vec_size(0) {
      data = nullptr;
   }

   // Destructor
   // REQUIRES: Nothing
   // MODIFIES: Destroys *this
   // EFFECTS: Performs any neccessary clean up operations
   ~vector( ) {
      delete[] data;
   }

   // Resize Constructor
   // REQUIRES: Nothing
   // MODIFIES: *this
   // EFFECTS: Constructs a vector with size num_elements,
   //    all default constructed
   vector( size_t num_elements ) {
      vec_capacity = num_elements;
      vec_size = num_elements;

      data = new T[vec_capacity];
   }

   // Fill Constructor
   // REQUIRES: capacity > 0
   // MODIFIES: *this
   // EFFECTS: Creates a vector with size num_elements, all assigned to val
   vector( size_t num_elements, const T &val ) {
      vec_capacity = num_elements;
      vec_size = num_elements;

      data = new T[vec_capacity];

      for (size_t i = 0; i < vec_size; i++) {
         data[i] = val;
      }
   }

   // Copy Constructor
   // REQUIRES: Nothing
   // MODIFIES: *this
   // EFFECTS: Creates a clone of the vector other
   vector( const vector<T> &other ) {
      vec_capacity = other.capacity();
      vec_size = other.size();

      data = new T[vec_capacity];

      for (size_t i = 0; i < vec_size; i++) {
         data[i] = other[i];
      }
   }

   // Assignment operator
   // REQUIRES: Nothing
   // MODIFIES: *this
   // EFFECTS: Duplicates the state of other to *this
   vector operator=( const vector<T> &other ) {
      if (this == &other) {
         return *this;
      }

      vec_capacity = other.capacity();
      vec_size = other.size();

      delete[] data;
      data = new T[vec_capacity];
      
      for (size_t i = 0; i < vec_size; i++) {
         data[i] = other[i];
      }

      return *this;
   }

   // Move Constructor
   // REQUIRES: Nothing
   // MODIFIES: *this, leaves other in a default constructed state
   // EFFECTS: Takes the data from other into a newly constructed vector
   vector( vector<T> &&other ) {
      vec_capacity = other.capacity();
      vec_size = other.size();
      data = other.begin(); 
      
      other.vec_capacity = 0;
      other.vec_size = 0;
      other.data = nullptr;
   }

   // Move Assignment Operator
   // REQUIRES: Nothing
   // MODIFIES: *this, leaves otherin a default constructed state
   // EFFECTS: Takes the data from other in constant time
   vector operator=( vector<T> &&other ) {
      vec_capacity = other.capacity();
      vec_size = other.size();

      delete[] data;
      data = other.begin();

      other.vec_capacity = 0;
      other.vec_size = 0;
      other.data = nullptr;

      return *this;
   }

   // REQUIRES: new_capacity > capacity( )
   // MODIFIES: capacity( )
   // EFFECTS: Ensures that the vector can contain size( ) = new_capacity
   //    elements before having to reallocate
   void reserve( size_t new_capacity ) {
      T* newData = new T[new_capacity];
      for (size_t i = 0; i < vec_size; i++) {
         newData[i] = data[i];
      }

      vec_capacity = new_capacity;

      delete[] data;
      data = newData;
   }

   // REQUIRES: Nothing
   // MODIFIES: Nothing
   // EFFECTS: Returns the number of elements in the vector
   size_t size( ) const {
      return vec_size;
   }

   // REQUIRES: Nothing
   // MODIFIES: Nothing
   // EFFECTS: Returns the maximum size the vector can attain before resizing
   size_t capacity( ) const {
      return vec_capacity;
   }

   // REQUIRES: 0 <= i < size( )
   // MODIFIES: Allows modification of data[i]
   // EFFECTS: Returns a mutable reference to the i'th element
   T &operator[ ]( size_t i ) {
      return data[i];
   }

   // REQUIRES: 0 <= i < size( )
   // MODIFIES: Nothing
   // EFFECTS: Get a const reference to the ith element
   const T &operator[ ]( size_t i ) const {
      return data[i];
   }

   // REQUIRES: Nothing
   // MODIFIES: this, size( ), capacity( )
   // EFFECTS: Appends the element x to the vector, allocating
   //    additional space if neccesary
   void pushBack( const T &x ) {
      if (vec_size == vec_capacity) {
         if (vec_capacity == 0) {
            reserve(1);
         }
         else {
            reserve(vec_capacity * 2);
         }
      }

      data[vec_size++] = x;
   }

   // REQUIRES: Nothing
   // MODIFIES: this, size( )
   // EFFECTS: Removes the last element of the vector,
   //    leaving capacity unchanged
   void popBack( ) {
      vec_size--;
   }

   // REQUIRES: Nothing
   // MODIFIES: Allows mutable access to the vector's contents
   // EFFECTS: Returns a mutable random access iterator to the 
   //    first element of the vector
   T* begin( ) {
      return data;
   }

   // REQUIRES: Nothing
   // MODIFIES: Allows mutable access to the vector's contents
   // EFFECTS: Returns a mutable random access iterator to 
   //    one past the last valid element of the vector
   T* end( ) {
      return data + vec_size;
   }

   // REQUIRES: Nothing
   // MODIFIES: Nothing
   // EFFECTS: Returns a random access iterator to the first element of the vector
   const T* begin( ) const {
      return data;
   }

   // REQUIRES: Nothing
   // MODIFIES: Nothing
   // EFFECTS: Returns a random access iterator to 
   //    one past the last valid element of the vector
   const T* end( ) const {
      return data + vec_size;
   }

private:

   //TODO
   size_t vec_capacity;
   size_t vec_size;

   T* data;

};
