#include "expression.h"
#include "../isr/isr.h"
#include "driver.cpp"
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

ISR* SequenceConstraint::Eval() const {
    sequences.push_back({});

    if (words.size() == 1) {
        std::cout << "Run ISR on word: ";
        std::cout << words[0] << std::endl;

        sequences[sequences.size()].push_back(new ISRWord(words[0]));

        // build ISRWord
        return new ISRWord(words[0]);
    }
    else {
        // Call sequence ISR on the sequence of words
        std::cout << "Run ISR on sequence: ";

        std::vector<ISR*> terms;
        for (const std::string& word : words) {
            std::cout << word << " ";

            terms.push_back(new ISRWord(word));
            sequences[sequences.size()].push_back(new ISRWord(word));
        }
        std::cout << std::endl;
        
        // build isr 
        return new ISROr(terms);
    }
}

// ---------- AND Constraint ----------

AndConstraint::AndConstraint(Constraint *l, Constraint *r) : left(l), right(r) {}

AndConstraint::~AndConstraint() {
    delete left;
    delete right;
}

ISR* AndConstraint::Eval() const {
    // Call ISRS here and return exit status
    std::cout << "Evaluating AND" << std::endl;
    
    // build and isr on both left and right
    return new ISRAnd({left->Eval(), right->Eval()}, new ISREndDoc());
}

// ---------- OR Constraint ----------

OrConstraint::OrConstraint(Constraint *l, Constraint *r) : left(l), right(r) {}

OrConstraint::~OrConstraint() {
    delete left;
    delete right;
}

ISR* OrConstraint::Eval() const {
    std::cout << "Evaluating OR" << std::endl;
    
    return new ISROr({left->Eval(), right->Eval()});
}

// ---------- NOT Constraint ----------

// NotConstraint::NotConstraint(Constraint *e) : expr(e) {}

// NotConstraint::~NotConstraint() {
//     delete expr;
// }

// ISR NotConstraint::Eval() const {
//     std::cout << "Evaluating NOT" << std::endl;
//     return expr->Eval();
// }

// ---------- Required Constraint ----------

// RequiredConstraint::RequiredConstraint(Constraint *e) : expr(e) {}

// RequiredConstraint::~RequiredConstraint() {
//     delete expr;
// }

// ISR RequiredConstraint::Eval() const {
//     // Placeholder: acts like a regular constraint
//     std::cout << "Evaluating Required" << std::endl;
//     expr->Eval();
//     return {};
// }

// ---------- Phrase Constraint ----------

PhraseConstraint::PhraseConstraint(const std::vector<std::string> &w) : words(w) {}

PhraseConstraint::~PhraseConstraint() {
    // Nothing to delete, vector handles its own memory
}

ISR* PhraseConstraint::Eval() const {
    std::cout << "Run ISR on phrase: ";

    sequences.push_back({});

    std::vector<ISR*> terms;
    for (const std::string &word : words) {
        std::cout << word << " ";

        terms.push_back(new ISRWord(word));
        sequences[sequences.size()].push_back(new ISRWord(word));
    }
    std::cout << std::endl;

    return new ISRPhrase(terms);
}

// ---------- Search Word Constraint ----------

// SearchWordConstraint::SearchWordConstraint(const std::string &w) : word(w) {}

// SearchWordConstraint::~SearchWordConstraint() {
//     // Nothing to delete
// }

// ISR SearchWordConstraint::Eval() const {
//     // Placeholder: just print the word and return true
//     std::cout << "Evaluating search word: " << word << std::endl;
//     return {};
// }
