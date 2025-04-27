#include "parser.h"
#include "expression.h"
#include <cctype>
#include <iostream>
#include <sstream>
#include "../isr/isr.h"
#include <unordered_set>

QueryParser::QueryParser(std::string &query, uint32_t numIndexChunks, const IndexFileReader & reader) 
    : stream(query), numIndexChunks(numIndexChunks), reader(reader) {}

std::string QueryParser::FindNextToken() {
    return stream.GetWord();
}

std::unique_ptr<Constraint> QueryParser::Parse() {
    return FindConstraint();
}

std::unique_ptr<Constraint> QueryParser::FindConstraint() {
    auto left = FindBaseConstraint();
    if (!left) return nullptr;

    while (FindOrOp()) {
        auto right = FindBaseConstraint();
        if (!right) return nullptr;
        left = std::make_unique<OrConstraint>(std::move(left), std::move(right), reader);
    }

    return left;
}

bool QueryParser::FindOrOp() {
    return stream.Match("OR") || stream.Match("|") || stream.Match("||");
}

std::unique_ptr<Constraint> QueryParser::FindBaseConstraint() {
    auto left = FindSimpleConstraint();
    if (!left) return nullptr;

    while (FindAndOp()) {
        auto right = FindSimpleConstraint();
        if (!right) return nullptr;
        left = std::make_unique<AndConstraint>(std::move(left), std::move(right), reader);
    }

    return left;
}

bool QueryParser::FindAndOp() {
    return stream.Match("AND") || stream.Match("&") || stream.Match("&&");
}

std::unique_ptr<Constraint> QueryParser::FindSimpleConstraint() {
    std::vector<std::string> positive_words;
    std::vector<std::string> negative_words;

    bool parsing_negative = false;

    std::unordered_set<std::string> special_tokens = {"OR", "|", "||", "AND", "&", "&&", "-", "\"", "(", ")"};

    while (!stream.Peek().empty() && !special_tokens.count(stream.Peek())) {
        std::cout << "inside special tokens" << std::endl;
        std::string word = stream.GetWord();
        if (word.empty()) break;

        if (word == "NOT") {
            parsing_negative = true;
            continue;
        }

        if (parsing_negative) {
            negative_words.push_back(word);
        } else {
            positive_words.push_back(word);
        }
    }

    if (positive_words.empty()) return nullptr;

    if (negative_words.empty()) {
        return std::make_unique<SequenceConstraint>(positive_words, reader);
    } else {
        return std::make_unique<ContainerConstraint>(positive_words, negative_words, reader);
    }
}


std::unique_ptr<Constraint> QueryParser::FindPhrase() {
    std::string quoteChar = stream.GetWord();
    if (quoteChar != "\"" && quoteChar != "'") return nullptr;

    std::vector<std::string> words;
    while (!stream.Match(quoteChar)) {
        std::string word = stream.GetWord();
        if (word.empty()) return nullptr;
        words.push_back(word);
    }

    return std::make_unique<PhraseConstraint>(words, reader);
}

std::unique_ptr<Constraint> QueryParser::FindNestedConstraint() {
    if (!stream.Match("(")) return nullptr;

    auto constraint = FindConstraint();
    if (!constraint || !stream.Match(")")) return nullptr;

    return constraint;
}
