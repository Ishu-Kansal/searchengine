#include <vector>
#include <string_view>
#include <string>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <sstream>

#include "driver.h"
#include "crawler/sockets.h"
#include "../utils/cstring_view.h"
#include "constraint_solver.h"
#include "query_parser/parser.h"
#include "inverted_index/Index.h"
#include "../HtmlParser/HtmlParser.h"
#include "json.hpp"

int get_num_unique_docs() {
    // hard coded constant
    int num_chunks = 100; 
    IndexFileReader reader(num_chunks);
    std::unique_ptr<ISREndDoc> docISR = make_unique<ISREndDoc>(reader); 
    int num_unique_docs = 0;
    for (int i = 0; i < num_chunks; i++) {
        docISR->Seek(0);
        num_unique_docs += docISR->GetNumberOfOccurrences(); 
    }
    return num_unique_docs; 
}

int get_num_unique_words() {
    // hard coded constant
    int num_chunks = 100; 
    IndexFileReader reader(num_chunks);
    int num_unique_words = 0; 
    // ngl idk how the index works lol
    for (auto curr_blob: reader->)

}
