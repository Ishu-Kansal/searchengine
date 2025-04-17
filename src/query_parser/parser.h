#pragma once

#include "tokenstream.h"
#include "expression.h"
#include "../isr/isr.h"

class QueryParser {
    TokenStream stream;
    uint32_t numIndexChunks;
    const IndexFileReader& reader;

public:
    explicit QueryParser(std::string &query, uint32_t numIndexChunks, const IndexFileReader & reader);

    std::unique_ptr<Constraint> Parse();

private:
    std::string FindNextToken();
    std::unique_ptr<Constraint> FindConstraint();
    bool FindOrOp();
    std::unique_ptr<Constraint> FindBaseConstraint();
    bool FindAndOp();
    std::unique_ptr<Constraint> FindSimpleConstraint();
    std::unique_ptr<Constraint> FindPhrase();
    std::unique_ptr<Constraint> FindNestedConstraint();
};

