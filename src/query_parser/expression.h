#pragma once
#include "../isr/isr.h"
#include <iostream>
#include <vector>
#include <string>

// ---------- Base Constraint ----------

class Constraint {
protected:
    const IndexFileReader& reader_;
public:
    Constraint(const IndexFileReader &reader) : reader_(reader) {}
    virtual ~Constraint() = default;
    virtual std::unique_ptr<ISR> Eval(std::vector<std::vector<std::unique_ptr<ISRWord>>> &sequences) const = 0;
    // void ClearSequences() { sequences.clear(); }
};
// ---------- Sequence Constraint -----
class SequenceConstraint : public Constraint {
    std::vector<std::string> words;
public:
    SequenceConstraint(const std::vector<std::string> &words, const IndexFileReader &reader)
        : Constraint(reader), words(words) {}

    ~SequenceConstraint() override = default;

    std::unique_ptr<ISR> Eval(std::vector<std::vector<std::unique_ptr<ISRWord>>> &sequences) const override {

        sequences.emplace_back();
        if (words.size() == 1) {
            std::cout << "Run ISR on word: ";
            std::cout << words[0] << std::endl;

            auto wordIsr = std::make_unique<ISRWord>(words[0], reader_);
            auto wordIsr2 = std::make_unique<ISRWord>(words[0], reader_);

            sequences.back().push_back(std::move(wordIsr2));

            return wordIsr; // Ownership transferred out
        }
        else {
            std::cout << "Run ISR on sequence (as AND): ";
            std::vector<std::unique_ptr<ISR>> terms;
            for (const std::string& word : words) {
                std::cout << word << " ";
                auto wordIsr = std::make_unique<ISRWord>(word, reader_);
                auto wordIsr2 = std::make_unique<ISRWord>(word, reader_);

                terms.push_back(std::move(wordIsr)); // Move ownership into vector
                sequences.back().push_back(std::move(wordIsr2));
            }
            std::cout << std::endl;

            return std::make_unique<ISRAnd>(std::move(terms), reader_);
        }
    }
};

// ---------- AND Constraint ----------

class AndConstraint : public Constraint {
    std::unique_ptr<Constraint> left;
    std::unique_ptr<Constraint> right;
public:
    AndConstraint(std::unique_ptr<Constraint> l, std::unique_ptr<Constraint> r, const IndexFileReader &reader)
        : Constraint(reader), left(std::move(l)), right(std::move(r)) {}

    ~AndConstraint() override = default;

    std::unique_ptr<ISR> Eval(std::vector<std::vector<std::unique_ptr<ISRWord>>> &sequences) const override
    {
        std::cout << "Evaluating AND" << std::endl;

        std::vector<std::unique_ptr<ISR>> terms;
        terms.push_back(left->Eval(sequences));
        terms.push_back(right->Eval(sequences));

        return std::make_unique<ISRAnd>(std::move(terms), reader_);
    }
};

// ---------- OR Constraint ----------

class OrConstraint : public Constraint {
    std::unique_ptr<Constraint> left;
    std::unique_ptr<Constraint> right;
public:
    OrConstraint(std::unique_ptr<Constraint> l, std::unique_ptr<Constraint> r, const IndexFileReader &reader)
        : Constraint(reader), left(std::move(l)), right(std::move(r)) {}

    ~OrConstraint() override = default;

    std::unique_ptr<ISR> Eval(std::vector<std::vector<std::unique_ptr<ISRWord>>> &sequences) const override
    {
        std::cout << "Evaluating OR" << std::endl;

        std::vector<std::unique_ptr<ISR>> terms;
        terms.push_back(left->Eval(sequences));
        terms.push_back(right->Eval(sequences));

        return std::make_unique<ISROr>(std::move(terms), reader_);
    }
};


// ---------- Phrase Constraint ----------

class PhraseConstraint : public Constraint {
    std::vector<std::string> words;
public:
    PhraseConstraint(const std::vector<std::string> &w, const IndexFileReader &reader)
        : Constraint(reader), words(w) {}

    ~PhraseConstraint() override = default;

    std::unique_ptr<ISR> Eval(std::vector<std::vector<std::unique_ptr<ISRWord>>> &sequences) const override
    {
        std::cout << "Run ISR on phrase: ";

        sequences.emplace_back();

        std::vector<std::unique_ptr<ISR>> terms;
        for (const std::string &word : words) {
            std::cout << word << " ";
            auto wordIsr = std::make_unique<ISRWord>(word, reader_);
            auto wordIsr2 = std::make_unique<ISRWord>(word, reader_);

            terms.push_back(std::move(wordIsr));
            sequences.back().push_back(std::move(wordIsr2));
        }
        std::cout << std::endl;

        return std::make_unique<ISRPhrase>(std::move(terms), reader_);
    }
};

// ---------- NOT Constraint ----------

class ContainerConstraint : public Constraint {
    std::vector<std::string> positive;
    std::vector<std::string> negative;

public:
    ContainerConstraint(const std::vector<std::string>& pos, const std::vector<std::string>& neg, const IndexFileReader& reader)
        : Constraint(reader), positive(pos), negative(neg) {}

    ~ContainerConstraint() override = default;

    std::unique_ptr<ISR> Eval(std::vector<std::vector<std::unique_ptr<ISRWord>>>& sequences) const override {
        std::cout << "Run ISRContainer: positive terms and NOT negative terms" << std::endl;

        sequences.emplace_back();
        std::vector<std::unique_ptr<ISR>> contained;
        for (const std::string& word : positive) {
            auto wordIsr = std::make_unique<ISRWord>(word, reader_);
            auto wordIsr2 = std::make_unique<ISRWord>(word, reader_);
            contained.push_back(std::move(wordIsr));
            sequences.back().push_back(std::move(wordIsr2));
        }

        sequences.emplace_back();
        std::vector<std::unique_ptr<ISR>> excluded;
        for (const std::string& word : negative) {
            auto wordIsr = std::make_unique<ISRWord>(word, reader_);
            auto wordIsr2 = std::make_unique<ISRWord>(word, reader_);
            excluded.push_back(std::move(wordIsr));
            sequences.back().push_back(std::move(wordIsr2));
        }

        std::cout << "Run ISRContainer: [";
        for (const auto& word : positive) std::cout << word << " ";
        std::cout << "] NOT [";
        for (const auto& word : negative) std::cout << word << " ";
        std::cout << "]" << std::endl;        

        return std::make_unique<ISRContainer>(std::move(contained), std::move(excluded), reader_);
    }
};
