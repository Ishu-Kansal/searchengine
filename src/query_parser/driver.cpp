#include <iostream>
#include <vector>

#include "parser.h"

std::vector<ISRWord> word_isrs;

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
   // Nabeel's function accepts isr and word_isrs

   return 0;
}
