#include <iostream>
#include <vector>

#include "parser.h"

std::vector<vector<ISRWord*>> sequences;

int main( ) {
   std::string input;
   std::getline(std::cin, input);

   QueryParser parser( input );

   Constraint *c = parser.Parse();

   if (c) {
      ISR isr = c->Eval();
   }
   else {
      std::cout << "Invalid Query" << std::endl;
   }

   // Constraint solver and dynamic ranker
   // Nabeel's function accepts isr and sequences

   return 0;
}
