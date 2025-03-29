/*
 * expression.h
 *
 * Class declarations for expressions
 *
 * You should declare more classes and functionality to match
 * the target grammar. Don't forget to implement them in expression.cpp.
 */

#ifndef EXPRESSION_H_
#define EXPRESSION_H_

#include <stdint.h>

/**
 * Just a plain old expression
 */
class Expression {
public:

   virtual ~Expression( );

   virtual int64_t Eval( ) const = 0;
};
// class Expression

/**
 * A number
 */
class Number: public Expression {
protected:

   int64_t value;

public:

   Number(int64_t val);

   int64_t Eval() const override;
};
// class Number

class AddSub : public Expression {
private:

   Expression *left;
   Expression *right;

   // '+' or '-'
   char op;

public:
   AddSub(Expression *l, Expression *r, char o) : left(l), right(r), op(o) {}

   ~AddSub();

   int64_t Eval() const override;
};

class MulDiv : public Expression {
private:
   Expression *left;
   Expression *right;

   // '*' or '/'
   char op;

public:
   MulDiv(Expression *l, Expression *r, char o) : left(l), right(r), op(o) {}

   ~MulDiv();

   int64_t Eval() const override;
};

#endif /* EXPRESSION_H_ */
