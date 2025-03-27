#include "Index.h"
#include <iostream>
#include <string>

void TestSingleDocument() {
    IndexChunk indexChunk;
    indexChunk.add_url("http://test.com/single");
    indexChunk.add_enddoc(500, 10, 10, 3, 2);
    indexChunk.add_word("word1", 10, true, false, true);
    indexChunk.add_word("word2", 20, false, true, true);
    indexChunk.add_word("word1", 30, true, false, false);
    std::cout << "TestSingleDocument completed" << std::endl;
}

void TestMultipleDocuments() {
    IndexChunk indexChunk;
    indexChunk.add_url("http://test.com/doc1");
    indexChunk.add_enddoc(1000, 20, 15, 5, 3);
    indexChunk.add_word("alpha", 100, true, false, true);
    indexChunk.add_word("beta", 150, false, true, false);
    indexChunk.add_word("alpha", 200, false, false, false);
    indexChunk.add_url("http://test.com/doc2");
    indexChunk.add_enddoc(2000, 30, 25, 10, 6);
    indexChunk.add_word("alpha", 50, true, true, true);
    indexChunk.add_word("gamma", 80, false, false, true);
    indexChunk.add_url("http://test.com/doc3");
    indexChunk.add_enddoc(1500, 25, 20, 8, 4);
    indexChunk.add_word("delta", 120, true, true, true);
    indexChunk.add_word("alpha", 180, false, false, true);
    std::cout << "TestMultipleDocuments completed" << std::endl;
}

void TestEdgeCases() {
    IndexChunk indexChunk;
    indexChunk.add_url("http://test.com/emptydoc");
    indexChunk.add_enddoc(0, 0, 0, 0, 0);
    indexChunk.add_url("http://test.com/singledigit");
    indexChunk.add_enddoc(1, 1, 1, 1, 1);
    indexChunk.add_word("edge", 1, false, false, true);
    indexChunk.add_word("edge", 2, true, true, false);
    std::cout << "TestEdgeCases completed" << std::endl;
}

void TestSequentialDeltas() {
    IndexChunk indexChunk;
    indexChunk.add_url("http://test.com/sequential");
    indexChunk.add_enddoc(300, 5, 5, 2, 1);
    uint64_t pos = 0;
    for (int i = 0; i < 10; ++i) {
        pos += (i + 1) * 10;
        indexChunk.add_word("sequential", pos, (i % 2) == 0, (i % 3) == 0, i == 0);
    }
    std::cout << "TestSequentialDeltas completed" << std::endl;
}

int main() {
    TestSingleDocument();
    TestMultipleDocuments();
    TestEdgeCases();
    TestSequentialDeltas();
    std::cout << "All tests passed." << std::endl;
    return 0;
}