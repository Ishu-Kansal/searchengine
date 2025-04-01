#include <assert.h>

#include <algorithm>
#include <string>
#include <cctype>
#include <sstream>
#include <iostream>
#include "tokenstream.h"
#include "unordered_set"

TokenStream::TokenStream( std::string &query ) : location( 0 ) {
   std::string current;
   size_t i = 0;

   const std::unordered_set<char> strippedPunctuation = {
      {'!', ',', '.', '?', ';'}
   };

   // Strip punctuation
   query.erase(
      std::remove_if(
         query.begin(), 
         query.end(), 
         [&strippedPunctuation](char c) {
            return strippedPunctuation.count(c) > 0;
         }
      ),
      query.end()
   );

   // // Convert to lowercase
   // std::transform(
   //    query.begin(), 
   //    query.end(), 
   //    query.begin(), 
   //    [](unsigned char c) { 
   //       return std::tolower(c); 
   //    }
   // );
   
   while (i < query.length()) {
      char c = query[i];

      // Handle spaces (skip consecutive spaces)
      if (std::isspace(c)) {
         if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
         }
         ++i;
         continue;
      }

      // Handle special tokens
      if (c == '&' || c == '|') {
         if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
         }
         if (i + 1 < query.length() && query[i + 1] == c) {
            tokens.push_back(std::string(2, c));  // "&&" or "||"
            i += 2;
         } else {
            tokens.push_back(std::string(1, c));  // "&" or "|"
            ++i;
         }
         continue;
      }

      if (c == '(' || c == ')' || c == '"' || c == '\'') {
         if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
         }
         tokens.push_back(std::string(1, c));  // Push the single character
         ++i;
         continue;
      }

      current += c;
      ++i;
   }

   // Push the last collected token if any
   if (!current.empty()) {
      tokens.push_back(current);
   }

   const std::unordered_set<std::string> keywords = {"AND", "OR", "NOT"};

   for (std::string& word : tokens) {
      if (keywords.find(word) == keywords.end()) {
         std::transform(word.begin(), word.end(), word.begin(), ::tolower);
      }
   }
   
   std::cout << "\nParsed search query:" << std::endl;
   for (std::string& token : tokens) {
      std::cout << token << ", ";
   }
   std::cout << '\n' << std::endl;
}

// Matches and consumes the next token if it equals `in`
bool TokenStream::Match(const std::string &in) {
   if (location < tokens.size() && tokens[location] == in) {
      ++location;  // Move to the next token
      return true;
   }
   return false;
}

// Returns and consumes the next word
std::string TokenStream::GetWord() {
   if (location < tokens.size()) {
      return tokens[location++];
   }
   return "";
}

// Peeks at the next token without consuming it
std::string TokenStream::Peek() {
   if (location < tokens.size()) {
      return tokens[location];
   }
   return "";
}

// Checks if all tokens have been consumed
bool TokenStream::AllConsumed() const {
   return location >= tokens.size();
}
