#include <string>
#include <iostream>
#include "query_parser/parser.h"
#include "isr/isr.h"
#include "constraint_solver.h"

std::vector<vector<ISRWord*>> sequences;

// driver function for the search engine
std::vector<std::string> run_engine(const std::string& query) {
   QueryParser parser(query);
   Constraint *c = parser.Parse();

   if (c) {
      ISR* isrs = c->Eval();
      return results = constraint_solver(isrs, sequences);
   } else {
      return {};  // Return empty vector on failure
   }
}
