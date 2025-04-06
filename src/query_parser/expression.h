#ifndef EXPRESSION_H_
#define EXPRESSION_H_

#include <stdint.h>
#include <string>
#include <vector>
#include "../isr/isr.h" // Include ISR definitions for compilation and evaluation

/**
 * Base class for all query constraints
 */
class Constraint {
public:
   virtual ~Constraint();
   virtual ISR Eval() const = 0;

   // ISR *Compile( );
};

class SequenceConstraint : public Constraint {
public:
    SequenceConstraint(const std::vector<std::string>& words);

    virtual ~SequenceConstraint();

    ISR Eval() const override;

private:
    std::vector<std::string> words;
};

// AND constraint (e.g., A AND B)
class AndConstraint : public Constraint {
private:
   Constraint *left;
   Constraint *right;

public:
   AndConstraint(Constraint *l, Constraint *r);
   ~AndConstraint();
   ISR Eval() const override;
};

// OR constraint (e.g., A OR B)
class OrConstraint : public Constraint {
private:
   Constraint *left;
   Constraint *right;

public:
   OrConstraint(Constraint *l, Constraint *r);
   ~OrConstraint();
   ISR Eval() const override;
};

// // NOT constraint (e.g., NOT A)
// class NotConstraint : public Constraint {
// private:
//    Constraint *expr;

// public:
//    NotConstraint(Constraint *e);
//    ~NotConstraint();
//    ISR Eval() const override;
// };

// Required constraint (e.g., +A)
class RequiredConstraint : public Constraint {
private:
   Constraint *expr;

public:
   RequiredConstraint(Constraint *e);
   ~RequiredConstraint();
   ISR Eval() const override;
};

// Phrase constraint (e.g., "search engine")
class PhraseConstraint : public Constraint {
private:
   std::vector<std::string> words;

public:
   PhraseConstraint(const std::vector<std::string> &w);
   ~PhraseConstraint();
   ISR Eval() const override;
};

// // Search word constraint (e.g., individual words)
// class SearchWordConstraint : public Constraint {
// private:
//    std::string word;

// public:
//    SearchWordConstraint(const std::string &w);
//    ~SearchWordConstraint();
//    ISR Eval() const override;
// };

#endif /* EXPRESSION_H_ */
