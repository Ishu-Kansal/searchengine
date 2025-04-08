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
      std::vector<std::pair<std::string, int>> raw_results = constraint_solver(isrs, sequences);

      std::vector<std::string> urls;
      urls.reserve(raw_results.size());
      std::transform(raw_results.begin(), raw_results.end(), std::back_inserter(urls),
                     [](const std::pair<std::string, int>& p) { return p.first; });
      
      return urls;
   } else {
      return {};  // Return empty vector on failure
   }
}
