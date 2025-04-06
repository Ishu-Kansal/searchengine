#include "expression.h"
#include "../isr/isr.h"
#include <iostream>
#include <vector>
#include <string>

// ---------- Base Constraint ----------

Constraint::~Constraint() {
    // Virtual destructor
}

// ---------- Sequence Constraint -----

SequenceConstraint::SequenceConstraint(const std::vector<std::string> &words) : words(words) {}

SequenceConstraint::~SequenceConstraint() {}

ISR SequenceConstraint::Eval() const {
    if (words.size() == 1) {
        std::cout << "Run ISR on word: ";
        std::cout << words[0] << std::endl;
        
        // build ISRWord
        return ISRWord(words[0]);
    }
    else {
        std::vector<ISR> terms;

         // Call sequence ISR on the sequence of words
        std::cout << "Run ISR on sequence: ";
        for (const std::string& word : words) {
            std::cout << word << " ";
            terms.emplace_back(ISRWord(word));
        }
        std::cout << std::endl;
        
        // build isr 
        return ISROr(terms, terms.size());
    }
}

// ---------- AND Constraint ----------

AndConstraint::AndConstraint(Constraint *l, Constraint *r) : left(l), right(r) {}

AndConstraint::~AndConstraint() {
    delete left;
    delete right;
}

ISR AndConstraint::Eval() const {
    // Placeholder: just AND the results (if Eval returns 0/1)
    // Call ISRS here and return exit status
    std::cout << "Evaluating AND" << std::endl;
    // Return the intersection of the lists returned by each
    std::vector<ISR> isrs(2);
    isrs[0] = left->Eval();
    isrs[1] = right->Eval(); 
    
    // build and isr on both left and right
    return ISRAnd(isrs, isrs.size()); 
}

// ---------- OR Constraint ----------

OrConstraint::OrConstraint(Constraint *l, Constraint *r) : left(l), right(r) {}

OrConstraint::~OrConstraint() {
    delete left;
    delete right;
}

ISR OrConstraint::Eval() const {
    // Placeholder: OR the results (if Eval returns 0/1)
    std::cout << "Evaluating OR" << std::endl;
    // Combine the lists returned by each and return combined list
    left->Eval();
    right->Eval();
    // build or isr on both left and right
    return {};
}

// ---------- NOT Constraint ----------

NotConstraint::NotConstraint(Constraint *e) : expr(e) {}

NotConstraint::~NotConstraint() {
    delete expr;
}

ISR NotConstraint::Eval() const {
    // Placeholder: NOT the result
    std::cout << "Evaluating NOT" << std::endl;
    expr->Eval();
    return {};
}

// ---------- Required Constraint ----------

RequiredConstraint::RequiredConstraint(Constraint *e) : expr(e) {}

RequiredConstraint::~RequiredConstraint() {
    delete expr;
}

ISR RequiredConstraint::Eval() const {
    // Placeholder: acts like a regular constraint
    std::cout << "Evaluating Required" << std::endl;
    expr->Eval();
    return {};
}

// ---------- Phrase Constraint ----------

PhraseConstraint::PhraseConstraint(const std::vector<std::string> &w) : words(w) {}

PhraseConstraint::~PhraseConstraint() {
    // Nothing to delete, vector handles its own memory
}

ISR PhraseConstraint::Eval() const {
    // Placeholder: just print the phrase and return true
    std::cout << "Run ISR on phrase: ";
    for (const std::string &word : words) {
        std::cout << word << " ";
    }
    std::cout << std::endl;
    return {};
}

// ---------- Search Word Constraint ----------

SearchWordConstraint::SearchWordConstraint(const std::string &w) : word(w) {}

SearchWordConstraint::~SearchWordConstraint() {
    // Nothing to delete
}

ISR SearchWordConstraint::Eval() const {
    // Placeholder: just print the word and return true
    std::cout << "Evaluating search word: " << word << std::endl;
    return {};
}
