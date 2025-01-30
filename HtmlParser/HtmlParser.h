// HtmlParser.h
// Nicole Hamilton, nham@umich.edu

#pragma once

#include <vector>
#include <cstring>
#include "HtmlTags.h"

// This is a simple HTML parser class.  Given a text buffer containing
// a presumed HTML page, the constructor will parse the text to create
// lists of words, title words and outgoing links found on the page.  It
// does not attempt to parse the entire the document structure.
//
// The strategy is to word-break at whitespace and HTML tags and discard
// most HTML tags.  Three tags require discarding everything between
// the opening and closing tag. Five tags require special processing.
//
// We will use the list of possible HTML element names found at
// https://developer.mozilla.org/en-US/docs/Web/HTML/Element +
// !-- (comment), !DOCTYPE and svg, stored as a table in HtmlTags.h.

// Here are the rules for recognizing HTML tags.
//
// 1. An HTML tag starts with either < if it's an opening tag or </ if
//    it's closing token.  If it starts with < and ends with /> it is both.
//
// 2. The name of the tag must follow the < or </ immediately.  There can't
//    be any whitespace.
//
// 3. The name is terminated by whitespace, > or / and is case-insensitive.
//
// 4. If it is terminated by whitepace, arbitrary text representing various
//    arguments may follow, terminated by a > or />.
//
// 5. If the name isn't on the list we recognize, we assume it's the whole
//    is just ordinary text.
//
// 6. Every token is taken as a word-break.
//
// 7. Most opening or closing tokens can simply be discarded.
//
// 8. <script>, <style>, and <svg> require discarding everything between the
//    opening and closing tag.  Unmatched closing tags are discarded.
//
// 9. <!--, <title>, <a>, <base> and <embed> require special processing.
// 
//      <-- is the beginng of a comment.  Everything up to the ending -->
//          is discarded.
//
//      <title> should cause all the words between the opening and closing
//          tags to be added to the titleWords vector rather than the default
//          words vector.  A closing </title> without an opening <title> is discarded.
//
//      <a> is expected to contain an href="...url..."> argument with the
//          URL inside the double quotes that should be added to the list
//          of links.  All the words in between the opening and closing tags
//          should be collected as anchor text associated with the link
//          in addition to being added to the words or titleWords vector,
//          as appropriate.  A closing </a> without an opening <a> is
//          discarded.
//
//     <base> may contain an href="...url..." parameter.  If present, it should
//          be captured as the base variable.  Only the first is recognized; any
//          others are discarded.
//
//     <embed> may contain a src="...url..." parameter.  If present, it should be
//          added to the links with no anchor text.



class Link
   {
   public:
      std::string URL;
      std::vector< std::string > anchorText;

      Link( std::string URL ) : URL( URL )
         {
         }
   };


class HtmlParser
   {
   public:

      std::vector< std::string > words, titleWords;
      std::vector< Link > links;
      std::string base;

   private:
      // Your code here.

      bool is_tag_ending(const char c)
      {
         return c == ' ' || c == '/' || c == '>';
      }

      size_t extract_base(const char* buffer, size_t length, size_t i)
      {
         // <base href="url" />
         bool in_quotes = false;
         char quote_type;
         while (i < length && (buffer[i] != '>' && !in_quotes))
         {
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
            if (!strcmp(buffer + i, "href"))
            {
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

               int url_start = i + 1;

               while (buffer[i] != quote_type)
               {
                  i++;
               }
               int url_end = i;

               base = std::string(url_start, url_end);
            }
            i++;
         }
         return i;
      }

      /*
      std::string extract_base(const char* ptr) {
         if (!ptr) return "";

         // Skip spaces after "base"
         while (*ptr && std::isspace(*ptr))
         {
            ++ptr;
         }

         // Look for closing tag
         std::strstr(ptr, ">");

         // Look for href
         

         if (std::strcmp(ptr, "href") != 0)
         {
            return "";
         }

         // Move past "href"
         ptr += 4;

         // Skip spaces after "href"
         while (*ptr && std::isspace(*ptr))
         {
            ++ptr;
         }

         // Ensure '=' is present
         if (std::strcmp(ptr, "=") != 0) 
         {
            return "";
          }

         // Move past '='
         ++ptr;

         // Skip spaces after '='
         while (*ptr && std::isspace(*ptr)) {
            ++ptr;
         }

         // Ensure it starts with a valid quote
         if (*ptr != '"' && *ptr != '\'') return "";

         char quoteType = *ptr;  // Remember whether it's a single or double quote
         ++ptr;  // Move past the quote

         // Find the closing quote
         const char* endPtr = ptr;
         while (*endPtr && *endPtr != quoteType) {
            ++endPtr;
         }

         if (*endPtr != quoteType) return "";  // No closing quote found

         return std::string(ptr, endPtr);
      }
      */

   public:

      // The constructor is given a buffer and length containing
      // presumed HTML.  It will parse the buffer, stripping out
      // all the HTML tags and producing the list of words in body,
      // words in title, and links found on the page.

      // < html>

      HtmlParser( const char *buffer, size_t length ) // Your code here
      {
         bool closing_tag = false;
         size_t i = 0;

         while (i < length)
         {
            if (buffer[i] == '<')
            {  
               i++;
               if (i == length) 
               {
                  return;
               }
               size_t tag_size = 0;
               if (buffer[i] == '/')
               {
                  closing_tag = true;
                  i++;
               }
               if (i == length)
               {
                  return;
               }

               // Run until there is a whitespace or / or >
               while (i + tag_size < length && !is_tag_ending(buffer[i + tag_size]))
               {
                  tag_size++;
               }
               
               //rn buffer is going from start to tag size, and reads like "link" or "head"
               DesiredAction action = LookupPossibleTag(buffer + i, buffer + i + tag_size);
               
               if (action == DesiredAction::Base) {
                  i = extract_base(buffer, length, i);
               }

               else if (action == DesiredAction::Title) {
                  
                  while (buffer[i] != '<' && i < length && !closing_tag) {
                     size_t word_len = 0;

                     while ((i + word_len) < length && buffer[i + word_len] != ' ' && buffer[i + word_len] != '<'
                            && buffer[i + word_len] != '\n' && buffer[i + word_len] != '\t') {

                        word_len++;
                     }
                     std::string word(buffer + i, buffer + i + word_len);
                     titleWords.push_back(word);

                     if (buffer[i + word_len] == '<')
                        i += word_len;
                        break;

                     i += (word_len + 1);
                  }
                  while (buffer[i] != '>') {
                     i++;
                  }
               } // Title

               else if (action == DesiredAction::Comment) {
                  
               }
            }
            i++;
         }
      }
   };


