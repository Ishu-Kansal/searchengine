#ifndef PARSER_H_
#define PARSER_H_

#include "tokenstream.h"
#include "expression.h"
#include <memory>
#include <string>

class QueryParser {
private:
   TokenStream stream;

public:
   explicit QueryParser(std::string &query);

   std::unique_ptr<Constraint> Parse();

   std::string FindNextToken();
   std::unique_ptr<Constraint> FindConstraint();
   bool FindOrOp();
   std::unique_ptr<Constraint> FindBaseConstraint();
   bool FindAndOp();
   std::unique_ptr<Constraint> FindSimpleConstraint();
   std::unique_ptr<Constraint> FindPhrase();
   std::unique_ptr<Constraint> FindNestedConstraint();
   // std::unique_ptr<Constraint> FindSearchWord(); 
};

#endif // PARSER_H_
