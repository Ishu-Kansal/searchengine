/*
 * tokenstream.cpp
 *
 * Implementation of tokenstream.h
 *
 * You do not have to modify this file, but you may choose to do so.
 */

#include <assert.h>

#include <algorithm>
#include <string>
#include <cctype>

#include "tokenstream.h"

bool TokenIsRelevant( const std::string &token ) {
   std::transform(token.begin(), token.end(), token.begin(), ::tolower);
   if (token == "&" || token == "&&" || token == "|" || token == "||" || token == "(" 
      || token == ")" || token == "and" || token == "or" || token == "not") {
      
      return true; // Logical operators
   }
   return false;
}

bool TokenIsIrrelevant( const std::string &token ) {
   return !TokenIsRelevant( token );
}

TokenStream::TokenStream( const std::string &in ) : input( in ) {
   // Erase irrelevant chars.
   input.erase( std::remove_if( input.begin( ), input.end( ), TokenIsIrrelevant ), input.end( ) );
}

void TokenStream::SkipWhitespace() {
    while (location < input.size() && std::isspace(input[location])) {
        location++;
    }
}

bool TokenStream::Match( const std::string &in ) {
   SkipWhitespace();
   if (input.compare(location, in.size(), in) == 0) {
      location += in.size();
      return true;
   }
   return false;
}

std::string TokenStream::GetWord() {
   SkipWhitespace();
   std::string word;
   
   while (location < input.size() && (std::isalnum(input[location]) || input[location] == '_')) {
      word += input[location++];
   }

   return word;
}

std::string TokenStream::Peek() {
   SkipWhitespace();
   return (location < input.size()) ? std::string(1, input[location]) : "";
}

bool TokenStream::AllConsumed( ) const {
   return location == input.size( );
}

// Number *TokenStream::ParseNumber( ) {
//    if ( location >= input.size( ) ) {
//       return nullptr;
//    }
//    // Parsing is done using strtoll, rather than atoi or variants
//    // This way, we can easily check for parsing success, since strtoll
//    // gives us a pointer to past how many characters it has consumed
//    char *end;
//    int64_t val = std::strtoll( input.c_str( ) + location, &end, 10 );
//    // Check for parse success. If we start and end at input.c_str( ) + location,
//    // then we have not processed any characters, and have failed to find a number
//    if ( end == input.c_str( ) + location ) {
//       return nullptr;
//    }
//    // Update location to the first unparsed char
//    location = end - input.c_str( );
//    return new Number( val );
// }
