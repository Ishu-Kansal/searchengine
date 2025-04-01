#ifndef PARSER_H_
#define PARSER_H_

#include "tokenstream.h"
#include "expression.h"
#include <string>

class QueryParser {
private:
   TokenStream stream;

public:
   explicit QueryParser(std::string &query);

   Constraint *Parse();

   std::string FindNextToken();
   Constraint *FindConstraint();
   bool FindOrOp();
   Constraint *FindBaseConstraint();
   bool FindAndOp();
   Constraint *FindSimpleConstraint();
   Constraint *FindPhrase();
   Constraint *FindNestedConstraint();
   Constraint *FindSearchWord();
};

#endif // PARSER_H_
