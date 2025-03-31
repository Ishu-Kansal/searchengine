/*
 * expression.cpp
 *
 * Class implementations for expression functionality.
 *
 * You will have to implement any additional functions you decide
 * to include in expression.h.
 */

#include <stdexcept>
#include "expression.h"

Expression::~Expression() {}

Number::Number(int64_t val) : value(val) {}

int64_t Number::Eval() const {
   return value;
}

AddSub::~AddSub() {
   delete left;
   delete right;
}

int64_t AddSub::Eval() const {
   if (op == '+') {
      return left->Eval() + right->Eval();
   }
   return left->Eval() - right->Eval();
}

MulDiv::~MulDiv() {
   delete left;
   delete right;
}

int64_t MulDiv::Eval() const {
   int64_t leftVal = left->Eval();
   int64_t rightVal = right->Eval();

   if (op == '*') {
      return leftVal * rightVal;
   }

   // Check for division by zero
   if (rightVal == 0) {
      throw std::runtime_error("Division by zero");
   }

   return leftVal / rightVal;
}

