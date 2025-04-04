#include "parser.h"
#include "expression.h"
#include <cctype>
#include <iostream>
#include <sstream>
#include <unordered_set>

QueryParser::QueryParser(std::string &query) : stream(query) {}

// Find the next token from the input stream
std::string QueryParser::FindNextToken() {
   return stream.GetWord();
}

// Parses a full query constraint (handles OR expressions)
Constraint *QueryParser::FindConstraint() {
   Constraint *left = FindBaseConstraint();
   if (!left) {
      return nullptr;
   }

   while (FindOrOp()) {
      Constraint *right = FindBaseConstraint();
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
Constraint *QueryParser::FindBaseConstraint() {
   Constraint *left = FindSimpleConstraint();

   if (!left) {
      return nullptr;
   }

   while (FindAndOp()) {
      Constraint *right = FindSimpleConstraint();
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
Constraint* QueryParser::FindSimpleConstraint() {
   if (stream.Match("NOT") || stream.Match("-")) {
      Constraint* constraint = FindSimpleConstraint();
      return constraint ? new NotConstraint(constraint) : nullptr;
   }
   else if (stream.Peek() == "'" || stream.Peek() == "\"") {
      return FindPhrase();
   } 
   else if (stream.Match("(")) {
      Constraint* constraint = FindConstraint(); // recurse
      if (!constraint || !stream.Match(")")) {
         delete constraint;
         return nullptr;
      }
      return constraint;
   }
    
    
   std::vector<std::string> words;
   std::unordered_set<std::string> not_words = {"OR", "|", "||", "AND", "&", "&&", "-", "\"", "(", ")"};
   while (stream.Peek() != "" && !not_words.count(stream.Peek())) {
      std::string word = stream.GetWord();
      if (word.empty()) break;
      words.push_back(word);
   }

   return new SequenceConstraint(words);
}

// Parses a quoted phrase (e.g., `"search engine"` or `'example phrase'`)
Constraint *QueryParser::FindPhrase() {
   std::string quoteChar = stream.GetWord();
   if (quoteChar != "\"" && quoteChar != "'") {
      return nullptr;
   } 

   std::vector<std::string> words;
   while (!stream.Match(quoteChar)) {
      std::string word = stream.GetWord();
      if (word.empty()) {
         return nullptr;
      }
      words.push_back(word);
   }
   return new PhraseConstraint(words);
}

// Parses a nested constraint inside parentheses
Constraint *QueryParser::FindNestedConstraint() {
   if (!stream.Match("(")) return nullptr;

   Constraint *constraint = FindConstraint();
   if (!constraint || !stream.Match(")")) {
      delete constraint;
      return nullptr;
   }
   return constraint;
}

// Parses an individual search word
// Constraint *QueryParser::FindSearchWord() {
//    std::string word = stream.GetWord();
//    std::cout << "find search word" << std::endl;
//    if (word.empty()) {
//       std::cout << "Returning nullptr" << std::endl;
//       return nullptr;
//    }
//    std::cout << "Returning search word" << std::endl;
//    return new SearchWordConstraint(word);
// }

// Main parse function
Constraint *QueryParser::Parse() {
   return FindConstraint();
}
