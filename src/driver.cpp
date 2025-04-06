#include <string>
#include "query_parser/parser.h"
#include <iostream>

// driver function for the search engine
int main(std::string query) {
    
    QueryParser parser(query);
    Constraint *c = parser.Parse();

    if (c) {
        c->Eval();
     }
     else {
        std::cout << "Invalid Query" << std::endl;
     }

}