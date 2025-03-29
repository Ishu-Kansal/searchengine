#ifndef PARSER_H_
#define PARSER_H_

#include "tokenstream.h"
#include "expression.h"
#include <string>

class QueryParser {
private:
   TokenStream stream;

public:
   explicit QueryParser(const std::string &query);

   std::string FindNextToken();
   Tuple *FindConstraint();
   bool FindOrOp();
   Tuple *FindBaseConstraint();
   bool FindAndOp();
   Tuple *FindSimpleConstraint();
   Tuple *FindPhrase();
   Tuple *FindNestedConstraint();
   Tuple *FindSearchWord();
};

#endif // PARSER_H_
