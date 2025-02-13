// string.h
// 
// Starter file for a string template


#include <cstddef>   // for size_t
#include <iostream>  // for ostream

class string
   {
   public:  

      // Default Constructor
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Creates an empty string
      string( ) : len( 0 ), capacity( 0 )
         {
            str = new char[ 1 ];
            str[0] = '\0';
         }

      // string Literal / C string Constructor
      // REQUIRES: cstr is a null terminated C style string
      // MODIFIES: *this
      // EFFECTS: Creates a string with equivalent contents to cstr
      string( const char *cstr ) : str( nullptr ), len( 0 ), capacity( 0 )
         {
            if ( cstr )
            {
               // calculate length of string
               while ( cstr[ len ] )
               {
                  len++;
               }

               capacity = len;
               str = new char[ capacity + 1 ];

               // put elements in string
               for ( size_t i = 0; i < len; i++ )
               {
                  str[ i ] = cstr[ i ];
               }
               str[ len ] = '\0';
            }
         }

      // Size
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns the number of characters in the string
      size_t size( ) const
         {
            return len;
         }

      // C string Conversion
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns a pointer to a null terminated C string of *this
      const char *cstr( ) const
         {
            return str;
         }

      // Iterator Begin
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns a random access iterator to the start of the string
      const char *begin( ) const
         {
            return str;
         }

      // Iterator End
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns a random access iterator to the end of the string
      const char *end( ) const
         {
            return str + len;
         }

      // Element Access
      // REQUIRES: 0 <= i < size()
      // MODIFIES: Allows modification of the i'th element
      // EFFECTS: Returns the i'th character of the string
      char &operator [ ]( size_t i )
         {
            return *( str + i );
         }

      // string Append
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Appends the contents of other to *this, resizing any
      //      memory at most once
      void operator+=( const string &other )
         {
            size_t new_size = other.len + len;
            if ( capacity < new_size )
            {
               capacity = new_size;
               char *larger_str = new char[ capacity + 1 ];
               for ( size_t i = 0; i < len; i++ )
               {
                  larger_str[ i ] = str[ i ];
               }
               
               delete[] str;
               str = larger_str;
            }
            for ( size_t i = 0; i < other.len; i++ )
            {
               str[ i + len ] = other.str[ i ];
            }

            len = new_size;
            str[ len ] = '\0';
         }

      // Push Back
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Appends c to the string
      void pushBack( char c )
         {
            if ( capacity <= len )
            {
               if ( capacity == 0 )
                  capacity = 1;
               else
                  capacity *= 2;

               char *larger_str = new char[ capacity + 1];
               for ( size_t i = 0; i < len; i++ )
               {
                  larger_str[ i ] = str[ i ];
               }
               delete[] str;
               str = larger_str;
            }
            
            str[ len ] = c;
            len += 1;
            str[ len ] = '\0';
         }

      // Pop Back
      // REQUIRES: string is not empty
      // MODIFIES: *this
      // EFFECTS: Removes the last charater of the string
      void popBack( )
         {
            len -= 1;
            str[ len ] = '\0';
         }

      // Equality Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether all the contents of *this
      //    and other are equal
      bool operator==( const string &other ) const
         {
            if ( len == other.len )
            {
               for ( size_t i = 0; i < len; i++ )
               {
                  if ( str[i] != other.str[i] )
                     return false;
               }
               return true;
            }
            return false;
         }

      // Not-Equality Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether at least one character differs between
      //    *this and other
      bool operator!=( const string &other ) const
         {
            return !(*this == other);
         }

      // Less Than Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether *this is lexigraphically less than other
      bool operator<( const string &other ) const
         {
            size_t min_length = len;
            if ( len > other.len )
               min_length = other.len;
            
            for ( size_t i = 0; i < min_length; i++ )
            {
               if ( str[ i ] < other.str[ i ] )
                  return true;
               else if ( str[ i ] > other.str[ i ] )
                  return false;
            }

            return len < other.len;
         }

      // Greater Than Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether *this is lexigraphically greater than other
      bool operator>( const string &other ) const
         {
            return other < *this;
         }

      // Less Than Or Equal Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether *this is lexigraphically less or equal to other
      bool operator<=( const string &other ) const
         {
            return !(other < *this);
         }

      // Greater Than Or Equal Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether *this is lexigraphically less or equal to other
      bool operator>=( const string &other ) const
         {
            return !(other > *this);
         }

   private:

      char* str;
      size_t len;
      size_t capacity;


   };

std::ostream &operator<<( std::ostream &os, const string &s )
   {
      const char *c = s.begin();
      os << c;
      return os;

   }