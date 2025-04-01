#ifndef TOKENSTREAM_H_
#define TOKENSTREAM_H_

#include <string>

#include "expression.h"

/**
 * Check if a character is relevant to a math expression
 *
 * Relevant characters are defined as
 * +
 * -
 * *
 * (
 * )
 *
 * and all digits
 */
bool TokenIsRelevant( const std::string &token );

/**
 * Opposite of char is relevant. Needed for filtering the input.
 */
// bool CharIsIrrelevant( char c );

/**
 * The token stream, which you can both Match( ) a single character from,
 * or ParseNumber( ) to consume a whole int64_t.
 *
 * The input string by default is filtered of any characters that are
 * deemed "irrelevant" by the CharIsIrrelevant function above.
 */
class TokenStream {
   // The input we receive, with only relevant characters left
   std::vector<std::string> tokens;

   // Where we currently are in the input
   size_t location;

public:

   /**
    * Construct a token stream that uses a copy of the input
    * that contains only characters relevant to math expressions
    */
   TokenStream( std::string &in );

   /**
    * Attempt to match and consume a specific character
    *
    * Returns true if the char was matched and consumed, false otherwise
    */
   bool Match( const std::string &in );

   /**
    * Check whether all the input was consumed
    */
   bool AllConsumed( ) const;

   std::string GetWord();

   std::string Peek();
};

#endif /* TOKENSTREAM_H_ */
