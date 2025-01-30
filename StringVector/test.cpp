#include <iostream>

#include "string.h"

int main() {

    string str("Hello World");
    string str2("Append");

    str += str2;
    str += str2;
    str += str2;

    str.popBack();
    str.popBack();

    std::cout << str << std::endl;

    return 0;
}