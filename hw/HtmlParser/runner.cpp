
#include <vector>
#include <cstring>
#include "HtmlTags.h"
#include <iostream>
bool is_tag_ending(const char c)
      {
         return c == ' ' || c == '/' || c == '>';
    }
int findBase(const char* buffer, int length) {
    size_t i = 0;
    std::string base;
    while (i < length)
    {
        
        if (buffer[i] == '<')
        {  
            i++;
            if (i == length) {
                return i;
            }
            size_t tag_size = 0;
            if (buffer[i] == '/')
            {
                i++;
            }
            if (i == length)
            {
                return i;
            }

            // Run until there is a whitespace or / or >
            while (i + tag_size < length && !is_tag_ending(buffer[i + tag_size]))
            {
                
                tag_size++;
            }
            
            //rn buffer is going from start to tag size, and reads like "link" or "head"
            DesiredAction action = LookupPossibleTag(buffer + i, buffer + i + tag_size-1);
            // std::cout << "hello: " << (buffer+i) << '\n';
            // std::cout << "tag size: " << tag_size << '\n';
            // std::cout << printAction(action) << '\n';
            // printf("%.*s\n", (int)tag_size, buffer+i);
            // std::cout << "init";
            
            if (action == DesiredAction::Base) {
                // Go until find <base href="url" />
                bool in_quotes = false;
                char quote_type;
                while (i < length)
                {
                    if (buffer[i] == '>' && !in_quotes)
                    {
                        i++;
                        break;
                    }
                    if (buffer[i] == '"' || buffer[i] == '\'')
                    {
                        if (in_quotes)
                        {
                        if (buffer[i] == quote_type)
                        {
                            in_quotes = false;
                        }
                        }
                        else
                        {
                        in_quotes = true;
                        quote_type = buffer[i];
                        }
                    }
                    // This will be wrong if href is found in quotes before the actual href
                    if (!strncmp(buffer + i, "href", 4))
                    {
                        // std::cout << buffer + i;
                        i += 4;
                        while (buffer[i] != '=' && i < length)
                        {
                        i++;
                        }
                        while (buffer[i] != '"' && buffer[i] != '\'' && i < length)
                        {
                        i++;
                        }

                        in_quotes = true;
                        quote_type = buffer[i];
                        
                        i++;
                        int url_start = i;

                        while (buffer[i] != quote_type)
                        {
                        i++;
                        }
                        int url_end = i;

                        base = std::string(buffer + url_start, buffer + url_end);
                        while(buffer[i] != '>')
                            i++;
                        std::cout << base << '\n';
                        return i;
                    }
                    i++;
                }
            }
        }
        i++;
    }
    return i;
}
int main() {
   const char* buf = "hello world. bullshit text. <base href=\"home/searchenginesishard\" target='your> mom'>";
   int base = findBase(buf, strlen(buf));
   std::cout << base << std::endl;
   std::cout << strlen(buf);
}