/*
 * tokenstream.cpp
 *
 * Implementation of tokenstream.h
 *
 * You do not have to modify this file, but you may choose to do so.
 */

#include <assert.h>

#include <algorithm>

#include "tokenstream.h"

bool CharIsRelevant( char c ) {
   switch ( c ) {
      case '+':
      case '-':
      case '*':
      case '/':
      case '(':
      case ')':
         return true;
      default:
         return isdigit( c );
   }
}

bool CharIsIrrelevant( char c ) {
   return !CharIsRelevant( c );
}

TokenStream::TokenStream( const std::string &in ) : input( in ) {
   // Erase irrelevant chars.
   input.erase( std::remove_if( input.begin( ), input.end( ), CharIsIrrelevant ), input.end( ) );
}

bool TokenStream::Match( char c ) {
   if ( location >= input.size( ) ) {
      return false;
   }
   if ( input[ location ] == c ) {
      ++location;
      return true;
   }
   return false;
}

bool TokenStream::AllConsumed( ) const {
   return location == input.size( );
}

Number *TokenStream::ParseNumber( ) {
   if ( location >= input.size( ) ) {
      return nullptr;
   }
   // Parsing is done using strtoll, rather than atoi or variants
   // This way, we can easily check for parsing success, since strtoll
   // gives us a pointer to past how many characters it has consumed
   char *end;
   int64_t val = std::strtoll( input.c_str( ) + location, &end, 10 );
   // Check for parse success. If we start and end at input.c_str( ) + location,
   // then we have not processed any characters, and have failed to find a number
   if ( end == input.c_str( ) + location ) {
      return nullptr;
   }
   // Update location to the first unparsed char
   location = end - input.c_str( );
   return new Number( val );
}
