#ifndef EXPRESSION_H_
#define EXPRESSION_H_

#include <stdint.h>
#include <string>
#include <vector>
//#include "../isr/isr.h" // Include ISR definitions for compilation and evaluation

// Delete this ISR class after irs.h is done and can be included
class ISR {
   
};

class Tuple {
public:
   Tuple *Next;
   virtual bool Eval( );
   Tuple( );
   virtual ~Tuple( );

   // virtual ISR *Compile( );
};

// class TupleList : Tuple {
// public:
//    Tuple *Top, *Bottom;
//    Fvoid Empty();
//    void Append(Tuple *t);

//    TupleList();
//    ~TupleLIst();
// }

/**
 * Base class for all query constraints
 */
class Constraint : public Tuple {
public:
   virtual ~Constraint();
   virtual bool Eval() const = 0;

   // ISR *Compile( );
};

// AND constraint (e.g., A AND B)
class AndConstraint : public Constraint {
private:
   Tuple *left;
   Tuple *right;

public:
   AndConstraint(Tuple *l, Tuple *r);
   ~AndConstraint();
   bool Eval() const override;
};

// OR constraint (e.g., A OR B)
class OrConstraint : public Constraint {
private:
   Tuple *left;
   Tuple *right;

public:
   OrConstraint(Tuple *l, Tuple *r);
   ~OrConstraint();
   bool Eval() const override;
};

// NOT constraint (e.g., NOT A)
class NotConstraint : public Constraint {
private:
   Tuple *expr;

public:
   NotConstraint(Tuple *e);
   ~NotConstraint();
   bool Eval() const override;
};

// Required constraint (e.g., +A)
class RequiredConstraint : public Constraint {
private:
   Tuple *expr;

public:
   RequiredConstraint(Tuple *e);
   ~RequiredConstraint();
   bool Eval() const override;
};

// Phrase constraint (e.g., "search engine")
class PhraseConstraint : public Constraint {
private:
   std::vector<std::string> words;

public:
   PhraseConstraint(const std::vector<std::string> &w);
   ~PhraseConstraint();
   bool Eval() const override;
};

// Search word constraint (e.g., individual words)
class SearchWordConstraint : public Constraint {
private:
   std::string word;

public:
   SearchWordConstraint(const std::string &w);
   ~SearchWordConstraint();
   bool Eval() const override;
};

#endif /* EXPRESSION_H_ */
