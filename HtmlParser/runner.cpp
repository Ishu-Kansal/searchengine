
#include <vector>
#include <cstring>
#include "HtmlTags.h"
#include <iostream>
bool is_tag_ending(const char c)
      {
         return c == ' ' || c == '/' || c == '>';
    }
std::string findBase(char* buffer, int length) {
    size_t i = 0;
    std::string base;
    while (i < length)
    {
        std::cout << buffer[i];
        if (buffer[i] == '<')
        {  
            i++;
            if (i == length) {
                return std::string("");
            }
            size_t tag_size = 0;
            if (buffer[i] == '/')
            {
                i++;
            }
            if (i == length)
            {
                return std::string("");
            }

            // Run until there is a whitespace or / or >
            while (i + tag_size < length && !is_tag_ending(buffer[i + tag_size]))
            {
                tag_size++;
            }
            
            //rn buffer is going from start to tag size, and reads like "link" or "head"
            DesiredAction action = LookupPossibleTag(buffer + i, buffer + i + tag_size);
            
            if (action == DesiredAction::Base) {
                // Go until find <base href="url" />
                bool in_quotes = false;
                char quote_type;
                while (i < length && (buffer[i] != '>' && !in_quotes)) {
                    if (buffer[i] == '"' || buffer[i] == '\'') {
                        if (in_quotes) {
                            if (buffer[i] == quote_type) {
                                in_quotes = false;
                            }
                        }
                        else {
                            in_quotes = true;
                            quote_type = buffer[i];
                        }
                        }
                        // This will be wrong if href is found in quotes before the actual href
                        if(!strcmp(buffer + i, "href")) {
                        i+=4;
                        while(buffer[i] != '=' && i < length) {
                            i++;
                        }
                        while (buffer[i] != '"' && buffer[i] != '\'' && i < length)
                        {
                            i++;
                        }
                        
                        in_quotes = true;
                        quote_type = buffer[i];

                        int url_start = i + 1;

                        while (buffer[i] != quote_type) {
                            i++;
                        }
                        int url_end = i;
                        
                        base = std::string(url_start, url_end);
                    }
                    i++;
                }
            }
        }
        i++;
    }
    return base;
}
int main() {
   char* buf = "ello world. bullsit text. <base href=\"ome/searchenginesisard\">";
   std::string base = findBase(buf, strlen(buf));
   std::cout << base << std::endl;
}