/*
 * parser.cpp
 *
 * Implementation of parser.h
 *
 * See parser.h for a full BNF of the grammar to implement.
 *
 * You should implement the different Find( ) functions,
 * as well as any additional functions you declare in parser.h.
 */

#include "expression.h"
#include "parser.h"

Expression *Parser::FindFactor() {

   bool neg = false;
   while (stream.Match('-')) {
      neg = !neg;
   }
   
   Number* num = stream.ParseNumber();
   if (num) {
      if (!neg) {
         return num;
      }
      return new AddSub(new Number(0), num, '-');
   }

   if (stream.Match('(')) {
      Expression *inner = FindAdd();
      if (!stream.Match(')')) {
          delete inner;
          return nullptr;
      }
      if (!neg) {
         return inner;
      }
      return new AddSub(new Number(0), inner, '-');
  }
   
   return nullptr;
}

Expression *Parser::FindAdd() {
   
   Expression *left = FindMultiply();
   if (!left) {
      return nullptr;
   }

   while (true) {
      if (stream.Match('+')) {
         Expression *right = FindMultiply();
         if (!right) {
            delete left;
            return nullptr;
         }
         left = new AddSub(left, right, '+');
      }
      else if (stream.Match('-')) {
         Expression *right = FindMultiply();
         if (!right) {
            delete left;
            return nullptr;
         }
         left = new AddSub(left, right, '-');
      }
      else {
         break;
      }
   }

   return left;
}

Expression *Parser::FindMultiply() {

   Expression *left = FindFactor();
   if (!left) {
      return nullptr;
   }

   while (true) {
      if (stream.Match('*')) {
         Expression *right = FindFactor();
         if (!right) {
            delete left;
            return nullptr;
         }
         left = new MulDiv(left, right, '*');
      }
      else if (stream.Match('/')) {
         Expression *right = FindFactor();
         if (!right) {
            delete left;
            return nullptr;
         }
         left = new MulDiv(left, right, '/');
      }
      else {
         break;
      }
   }
   return left;
}

Expression *Parser::Parse() {
   Expression* expr = FindAdd();
   if (!expr || !stream.AllConsumed()) {
      delete expr;
      return nullptr;
   }
   return expr;
}

Parser::Parser(const std::string &in) : stream(in) {}
