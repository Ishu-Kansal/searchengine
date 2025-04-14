#include "expression.h"
#include "../isr/isr.h"
#include <iostream>
#include <vector>
#include <string>

extern std::vector<vector<ISRWord*>> sequences;

// ---------- Base Constraint ----------

class Constraint {
protected:
    const IndexFileReader& reader_;
public:
    Constraint(const IndexFileReader& reader) : reader_(reader) {}
    virtual ~Constraint() {}
    virtual ISR* Eval() const = 0;
};

// ---------- Sequence Constraint -----

SequenceConstraint::SequenceConstraint(const std::vector<std::string> &words, const IndexFileReader& reader) 
    : Constraint(reader), words(words) {}

SequenceConstraint::~SequenceConstraint() {}

ISR* SequenceConstraint::Eval() const {
    sequences.emplace_back();

    if (words.size() == 1) {
        std::cout << "Run ISR on word: ";
        std::cout << words[0] << std::endl;

        auto wordIsr = new ISRWord(words[0], reader_);
        sequences.back().push_back(wordIsr);
        return wordIsr;
    }
    else {
        // Call sequence ISR on the sequence of words
        std::cout << "Run ISR on sequence: ";

        std::vector<std::unique_ptr<ISR>> terms;
        for (const std::string& word : words) {
            std::cout << word << " ";
            auto wordIsr = std::make_unique<ISRWord>(word, reader_);
            sequences.back().push_back(new ISRWord(word, reader_));
            terms.push_back(std::move(wordIsr));
        }
        std::cout << std::endl;
        
        // build isr 
        return ISROr(std::move(terms), reader_);
    }
}

// ---------- AND Constraint ----------

AndConstraint::AndConstraint(Constraint *l, Constraint *r, const IndexFileReader& reader) 
    : Constraint(reader), left(l), right(r) {}

AndConstraint::~AndConstraint() {
    delete left;
    delete right;
}

ISR* AndConstraint::Eval() const {
    std::cout << "Evaluating AND" << std::endl;
    
    std::vector<std::unique_ptr<ISR>> terms;
    terms.push_back(std::unique_ptr<ISR>(left->Eval()));
    terms.push_back(std::unique_ptr<ISR>(right->Eval()));
    return new ISRAnd(std::move(terms), reader_);
}

// ---------- OR Constraint ----------

OrConstraint::OrConstraint(Constraint *l, Constraint *r, const IndexFileReader& reader) 
    : Constraint(reader), left(l), right(r) {}

OrConstraint::~OrConstraint() {
    delete left;
    delete right;
}

ISR* OrConstraint::Eval() const {
    std::cout << "Evaluating OR" << std::endl;
    
    std::vector<std::unique_ptr<ISR>> terms;
    terms.push_back(std::unique_ptr<ISR>(left->Eval()));
    terms.push_back(std::unique_ptr<ISR>(right->Eval()));
    return new ISROr(std::move(terms), reader_);
}

// ---------- Phrase Constraint ----------

PhraseConstraint::PhraseConstraint(const std::vector<std::string> &w, const IndexFileReader& reader) 
    : Constraint(reader), words(w) {}

PhraseConstraint::~PhraseConstraint() {}

ISR* PhraseConstraint::Eval() const {
    std::cout << "Run ISR on phrase: ";

    sequences.emplace_back();

    std::vector<std::unique_ptr<ISR>> terms;
    for (const std::string &word : words) {
        std::cout << word << " ";
        auto wordIsr = std::make_unique<ISRWord>(word, reader_);
        sequences.back().push_back(new ISRWord(word, reader_));
        terms.push_back(std::move(wordIsr));
    }
    std::cout << std::endl;

    return new ISRPhrase(std::move(terms), reader_);
}
