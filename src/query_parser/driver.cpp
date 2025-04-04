#include <iostream>

#include "parser.h"
// #include "tokenstream.h"

int main( ) {
   std::string input;
   std::getline(std::cin, input);

   //TokenStream stream(input);

   QueryParser parser( input );

   Constraint *c = parser.Parse();

   if (c) {
      c->Eval();
   }
   else {
      std::cout << "Invalid Query" << std::endl;
   }
   
   return 0;
}
