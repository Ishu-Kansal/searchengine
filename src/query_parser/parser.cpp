#include "parser.h"
#include "expression.h"
#include <cctype>
#include <iostream>
#include <sstream>

QueryParser::QueryParser(const std::string &query) : stream(query) {}

// Find the next token from the input stream
std::string QueryParser::FindNextToken() {
   return stream.GetWord();
}

// Parses a full query constraint (handles OR expressions)
Tuple *QueryParser::FindConstraint() {
   Tuple *left = FindBaseConstraint();
   if (!left) return nullptr;

   while (FindOrOp()) {
      Tuple *right = FindBaseConstraint();
      if (!right) {
         delete left;
         return nullptr;
      }
      left = new OrConstraint(left, right);
   }
   return left;
}

// Parses OR operators (OR, |, ||)
bool QueryParser::FindOrOp() {
   return stream.Match("OR") || stream.Match("|") || stream.Match("||");
}

// Parses a base constraint (handles AND expressions)
Tuple *QueryParser::FindBaseConstraint() {
   Tuple *left = FindSimpleConstraint();
   if (!left) {
      return nullptr;
   }

   while (true) {
      if (!FindAndOp()) break;

      Tuple *right = FindSimpleConstraint();
      if (!right) {
         delete left;
         return nullptr;
      }
      left = new AndConstraint(left, right);
   }
   return left;
}

// Parses AND operators (AND, &, &&)
bool QueryParser::FindAndOp() {
   return stream.Match("AND") || stream.Match("&") || stream.Match("&&");
}

// Parses a simple constraint: phrase, nested constraint, unary operators, or a search word
Tuple *QueryParser::FindSimpleConstraint() {
   if (stream.Match("NOT") || stream.Match("-")) {
      Tuple *constraint = FindSimpleConstraint();
      if (constraint) {
         return new NotConstraint(constraint);
      }
      return nullptr;
   }
   else if (stream.Match("+")) {
      Tuple *constraint = FindSimpleConstraint();
      if (constraint) {
         return new RequiredConstraint(constraint);
      }
      return nullptr;
   }
   else if (stream.Peek() == std::string("'")) {
      return FindPhrase();
   }
   else if (stream.Peek() == "(") {
      return FindNestedConstraint();
   }
   else {
      return FindSearchWord();
   }
}

// Parses a quoted phrase (e.g., `"search engine"`)
Tuple *QueryParser::FindPhrase() {
    if (!stream.Match("\"")) return nullptr;
    
    std::vector<std::string> words;
    while (!stream.Match("\"")) {
        std::string word = stream.GetWord();
        if (word.empty()) return nullptr;
        words.push_back(word);
    }
    return new PhraseConstraint(words);
}

// Parses a nested constraint inside parentheses
Tuple *QueryParser::FindNestedConstraint() {
    if (!stream.Match("(")) return nullptr;

    Tuple *constraint = FindConstraint();
    if (!constraint || !stream.Match(")")) {
        delete constraint;
        return nullptr;
    }
    return constraint;
}

// Parses an individual search word
Tuple *QueryParser::FindSearchWord() {
   std::string word = stream.GetWord();
   if (word.empty()) {
   return nullptr;
   }
   return new SearchWordConstraint(word);
}
