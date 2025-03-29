#include "expression.h"
#include <iostream>

// ---------- Base Constraint ----------

Constraint::~Constraint() {
    // Virtual destructor
}

// ---------- AND Constraint ----------

AndConstraint::AndConstraint(Tuple *l, Tuple *r) : left(l), right(r) {}

AndConstraint::~AndConstraint() {
    delete left;
    delete right;
}

bool AndConstraint::Eval() const {
    // Placeholder: just AND the results (if Eval returns 0/1)
    // Call ISRS here and return exit status
    return left->Eval() && right->Eval();
}

// ---------- OR Constraint ----------

OrConstraint::OrConstraint(Tuple *l, Tuple *r) : left(l), right(r) {}

OrConstraint::~OrConstraint() {
    delete left;
    delete right;
}

bool OrConstraint::Eval() const {
    // Placeholder: OR the results (if Eval returns 0/1)
    return left->Eval() || right->Eval();
}

// ---------- NOT Constraint ----------

NotConstraint::NotConstraint(Tuple *e) : expr(e) {}

NotConstraint::~NotConstraint() {
    delete expr;
}

bool NotConstraint::Eval() const {
    // Placeholder: NOT the result
    return !expr->Eval();
}

// ---------- Required Constraint ----------

RequiredConstraint::RequiredConstraint(Tuple *e) : expr(e) {}

RequiredConstraint::~RequiredConstraint() {
    delete expr;
}

bool RequiredConstraint::Eval() const {
    // Placeholder: acts like a regular constraint
    return expr->Eval();
}

// ---------- Phrase Constraint ----------

PhraseConstraint::PhraseConstraint(const std::vector<std::string> &w) : words(w) {}

PhraseConstraint::~PhraseConstraint() {
    // Nothing to delete, vector handles its own memory
}

bool PhraseConstraint::Eval() const {
    // Placeholder: just print the phrase and return true
    std::cout << "Evaluating phrase: ";
    for (const std::string &word : words) {
        std::cout << word << " ";
    }
    std::cout << std::endl;
    return 1;
}

// ---------- Search Word Constraint ----------

SearchWordConstraint::SearchWordConstraint(const std::string &w) : word(w) {}

SearchWordConstraint::~SearchWordConstraint() {
    // Nothing to delete
}

bool SearchWordConstraint::Eval() const {
    // Placeholder: just print the word and return true
    std::cout << "Evaluating search word: " << word << std::endl;
    return 1;
}
