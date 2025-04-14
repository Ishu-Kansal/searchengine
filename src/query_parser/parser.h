#pragma once

#include "tokenstream.h"
#include "expression.h"
#include <memory>
#include <string>

class QueryParser {
private:
   TokenStream stream;
   //IndexFileReader queryReader;

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

