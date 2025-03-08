// HtmlParser.h
// Nicole Hamilton, nham@umich.edu

#pragma once

#include <cstring>
#include <string>
#include <vector>

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
//          words vector.  A closing </title> without an opening <title> is
//          discarded.
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
//     <embed> may contain a src="...url..." parameter.  If present, it should
//     be
//          added to the links with no anchor text.

class Link {
 public:
  std::string URL;
  std::vector<std::string> anchorText;

  Link(std::string URL) : URL(URL) {}
};

class HtmlParser {
 public:
  std::vector<std::string> words, titleWords;
  std::vector<Link> links;
  std::string base;

 private:
  // Your code here.

  bool found = false;

  bool is_tag_ending(const char c) {
    return c == ' ' || c == '\n' || c == '/' || c == '>' || c == '\r' ||
           c == '\f' || c == '\t';
  }

  size_t extract_anchor(const char *buffer, size_t length, size_t index,
                        bool in_title) {
    // move past the first href
    while (strncmp(buffer + index, "href", 4) != 0) {
      if (buffer[index] == '>') {
        return index + 1;
      }
      // if (strncmp(buffer + index, "href", 4) == 0)
      // {
      //    break;
      // }
      index++;
    }

    // move past the first open quote
    size_t url_start;
    while (buffer[index] != '"' && buffer[index] != '\'') {
      if (buffer[index] == '>') {
        return index + 1;
      }
      // if (buffer[index] == '"' || buffer[index] == '\'')
      // {
      //    url_start = ++index;
      //    break;
      // }
      index++;
    }
    char quote_type = buffer[index];
    url_start = ++index;

    // move past the next end quote
    size_t url_end;
    while (buffer[index] != quote_type) {
      // if (buffer[index] == '"' || buffer[index] == '\'')
      // {
      //    url_end = index;
      //    break;
      // }
      index++;
    }
    url_end = index;

    // create link with url
    std::string url(buffer + url_start, buffer + url_end);

    links.emplace_back(url);

    size_t link_idx = links.size() - 1;

    // move past the next >
    while (buffer[index] != '>') {
      // if (buffer[index] == '>')
      // {
      //    index++;
      //    break;
      // }
      index++;
    }
    index++;

    // add link text to link object
    std::string word = "";
    bool inside_bracket = false;
    while (index < length) {
      if (strncmp(buffer + index, "</a>", 4) == 0) {
        if (word != "") {
          if (!in_title) {
            words.push_back(word);
          }
          links[link_idx].anchorText.push_back(word);
        }
        index += 4;
        break;
      }

      else if ((strncmp(buffer + index, "</a\n>", 4) == 0) ||
               (strncmp(buffer + index, "</a\r>", 4) == 0) ||
               (strncmp(buffer + index, "</a\f>", 4) == 0)) {
        if (word != "") {
          if (!in_title) {
            words.push_back(word);
          }
          links[link_idx].anchorText.push_back(word);
        }
        index += 6;
        break;
      }

      if (buffer[index] == '<') {
        if (strncmp(buffer + index, "<a", 2) == 0) {
          if (in_title && !found) {
            found = true;
            index = extract_anchor(buffer, length, index + 2, in_title) - 1;
          } else {
            return extract_anchor(buffer, length, index + 2, in_title);
          }
        }

        else if (strncmp(buffer + index, "<base", 5) == 0) {
          return extract_base(buffer, length, index + 5);
        } else {
          index++;
          if (buffer[index] == '/') {
            index++;
            if (index == length) return index;
          }

          size_t tag_size = 0;
          while (index + tag_size < length &&
                 !is_tag_ending(buffer[index + tag_size])) {
            tag_size++;
          }

          // store tag in a c string
          std::string tag_string =
              std::string(buffer + index, buffer + index + tag_size);

          // grab tag action based on name
          DesiredAction action =
              LookupPossibleTag(buffer + index, buffer + index + tag_size - 1);

          if (action != DesiredAction::OrdinaryText || tag_string == "path" ||
              tag_string == "defs" || tag_string == "g") {
            if (word.size() > 0) {
              if (!in_title) {
                words.push_back(word);
              }
              links[link_idx].anchorText.push_back(word);
              word = "";
            }
            inside_bracket = true;
          } else {
            index += tag_size;
            word += '<' + tag_string + buffer[index];
          }
        }
      } else if (buffer[index] == '>') {
        inside_bracket = false;
      } else if (!inside_bracket) {
        if (buffer[index] == ' ' || buffer[index] == '\t' ||
            buffer[index] == '\r' || buffer[index] == '\n' ||
            strncmp(buffer + index, "\r\n", 2) == 0) {
          if (word.size() > 0) {
            if (!in_title) {
              words.push_back(word);
            }
            links[link_idx].anchorText.push_back(word);
            word = "";
          }
        } else {
          word += buffer[index];
        }
      }

      index++;
    }

    return index;
  }

  size_t extract_base(const char *buffer, size_t length, size_t i) {
    // <base href="url" />
    bool in_quotes = false;
    char quote_type;
    while (i < length) {
      if (buffer[i] == '>' && !in_quotes) {
        i++;
        break;
      }
      if (buffer[i] == '"' || buffer[i] == '\'') {
        if (in_quotes && (buffer[i] == quote_type)) {
          in_quotes = false;
        } else {
          in_quotes = true;
          quote_type = buffer[i];
        }
      }
      // This will be wrong if href is found in quotes before the actual href
      if (!strncmp(buffer + i, "href", 4)) {
        // std::cout << buffer + i;
        i += 4;
        while (buffer[i] != '=' && i < length) {
          i++;
        }
        while (buffer[i] != '"' && buffer[i] != '\'' && i < length) {
          i++;
        }
        in_quotes = true;
        quote_type = buffer[i];
        // now that we are inside the quotes, read in the base url
        i++;
        int url_start = i;
        while (buffer[i] != quote_type) {
          i++;
        }
        int url_end = i;
        base = std::string(buffer + url_start, buffer + url_end);
        while (buffer[i] != '>') i++;
        // std::cout << base << '\n';
        return i + 1;
      }
      i++;
    }
    return i;
  }

  size_t extract_embed(const char *buffer, size_t length, size_t i) {
    // <base href="url" />
    bool in_quotes = false;
    char quote_type;
    while (!(buffer[i] == '>' && !in_quotes)) {
      // if (buffer[i] == '>' && !in_quotes) {
      //    i++;
      //    break;
      // }
      if (buffer[i] == '"' || buffer[i] == '\'') {
        if (in_quotes && (buffer[i] == quote_type)) {
          in_quotes = false;
        } else {
          in_quotes = true;
          quote_type = buffer[i];
        }
      }
      // This will be wrong if href is found in quotes before the actual href
      if (!strncmp(buffer + i, "src", 3)) {
        // std::cout << buffer + i;
        i += 3;
        while (buffer[i] != '=' && i < length) {
          i++;
        }
        while (buffer[i] != '"' && buffer[i] != '\'' && i < length) {
          i++;
        }
        in_quotes = true;
        quote_type = buffer[i];
        // now that we are inside the quotes, read in the base url
        i++;
        int url_start = i;
        while (buffer[i] != quote_type) {
          i++;
        }
        int url_end = i;
        std::string url = std::string(buffer + url_start, buffer + url_end);
        while (buffer[i] != '>') i++;
        Link link(url);
        links.push_back(link);
        return i;
      }
      i++;
    }
    i++;
    return i;
  }

 public:
  // The constructor is given a buffer and length containing
  // presumed HTML.  It will parse the buffer, stripping out
  // all the HTML tags and producing the list of words in body,
  // words in title, and links found on the page.

  HtmlParser(const char *buffer, size_t length)  // Your code here
  {
    words.reserve(160000);
    titleWords.reserve(400);
    links.reserve(16000);

    base = "";

    std::string word = "";
    size_t index = 0;
    while (index < length) {
      bool closing_token = false;

      // start of a tag
      // std::cout << "Curr Character: " << buffer[index] << '\n';
      if (buffer[index] == '<') {
        // std::cout << "hello" << std::endl;
        index++;
        if (index == length) return;

        if (buffer[index] == '/') {
          closing_token = true;
          index++;
          if (index == length) return;
        }

        size_t tag_size = 0;
        while (index + tag_size < length &&
               !is_tag_ending(buffer[index + tag_size])) {
          tag_size++;
        }

        // store tag in a c string

        // TODO: Maybe make this a c string
        std::string tag_string =
            std::string(buffer + index, buffer + index + tag_size);

        // grab tag action based on name
        // std::cout << tag_size << std::endl;
        DesiredAction action;
        if (tag_size == 0) {
          action = DesiredAction::OrdinaryText;
        } else {
          action =
              LookupPossibleTag(buffer + index, buffer + index + tag_size - 1);
        }

        // std::cout << tag_string << '\n';
        // std::cout << printAction(action) << '\n';

        index += tag_size;

        if (action != DesiredAction::OrdinaryText) {
          if (word != "") {
            words.push_back(word);
            word = "";
          }
        }

        bool inside_bracket = false;

        // std::cout << "start\n";
        switch (action) {
          case DesiredAction::Discard:
            while (buffer[index] != '>') {
              // This code does not ensure that </tag> is outside of any quotes
              // if (buffer[index] == '>')
              // {
              //    index++;
              //    break;
              // }
              index++;
            }
            index++;
            break;

          case DesiredAction::DiscardSection:

            if (tag_string == "svg") {
              while (index < length) {
                // This code does not ensure that </tag> is outside of any
                // quotes
                if (strncmp(buffer + index, "</", 2) == 0 &&
                    strncmp(buffer + index + 2, "svg", tag_size) == 0) {
                  index += tag_size + 3;
                  break;
                }
                index++;
              }
            } else if (tag_string == "style") {
              while (index < length) {
                // This code does not ensure that </tag> is outside of any
                // quotes
                if (strncmp(buffer + index, "</", 2) == 0 &&
                    strncmp(buffer + index + 2, "style", tag_size) == 0) {
                  index += tag_size + 3;
                  break;
                }
                index++;
              }
            } else {
              while (index < length) {
                // This code does not ensure that </tag> is outside of any
                // quotes
                if (strncmp(buffer + index, "</", 2) == 0 &&
                    strncmp(buffer + index + 2, "script", tag_size) == 0) {
                  index += tag_size + 3;
                  break;
                }
                index++;
              }
            }

            break;

          case DesiredAction::Comment:
            // loop until find ending of comment
            while ((index + 2) < length &&
                   (buffer[index] != '-' || buffer[index + 1] != '-' ||
                    buffer[index + 2] != '>')) {
              index++;
            }
            index += 4;

            break;

          case DesiredAction::Title:

            while (buffer[index] != '>') {
              // if (buffer[index] == '>')
              // {
              //    index++;
              //    break;
              // }
              index++;
            }
            index++;

            // grab each word in between title tags
            inside_bracket = false;
            while (index < length && strncmp(buffer + index, "</title>", 8) &&
                   !closing_token) {
              if (!(strncmp(buffer + index, "</title\n>", 8) &&
                    strncmp(buffer + index, "</title\r>", 8) &&
                    strncmp(buffer + index, "</title\f>", 8))) {
                index += 2;
                break;
              }
              if (buffer[index] == '<') {
                if (strncmp(buffer + index, "<a", 2) == 0) {
                  extract_anchor(buffer, length, index + 2, true);
                  links.pop_back();
                }
                if (word.size() > 0) {
                  titleWords.push_back(word);
                  word = "";
                }
                inside_bracket = true;
              } else if (buffer[index] == '>') {
                inside_bracket = false;
              } else if (!inside_bracket) {
                if (buffer[index] == ' ' || buffer[index] == '\t' ||
                    buffer[index] == '\r' || buffer[index] == '\n' ||
                    strncmp(buffer + index, "\r\n", 2) == 0) {
                  if (word.size() > 0) {
                    titleWords.push_back(word);
                    word = "";
                  }
                } else {
                  word += buffer[index];
                }
              }
              index++;
            }
            if (word.size() > 0) {
              titleWords.push_back(word);
              word = "";
            }
            index += 8;

            break;

          case DesiredAction::Anchor:
            index = extract_anchor(buffer, length, index, false);
            break;

          case DesiredAction::Base:
            if (base.size() > 0) {
              std::string save = base;
              index = extract_base(buffer, length, index);
              base = save;
            } else {
              index = extract_base(buffer, length, index);
            }
            break;

          case DesiredAction::Embed:
            index = extract_embed(buffer, length, index);
            break;

          case DesiredAction::OrdinaryText:
            if (buffer[index] == ' ') {
              words.push_back(word + '<' + tag_string);
              word = "";
              index++;
            } else {
              word += '<' + tag_string + buffer[index];
              index++;
            }
            break;
        }

      } else {
        if (buffer[index] == ' ' || buffer[index] == '\t' ||
            buffer[index] == '\r' || buffer[index] == '\n' ||
            strncmp(buffer + index, "\r\n", 2) == 0) {
          if (word.size() > 0) {
            words.push_back(word);
            // std::cout << "Add word: " << word << '\n';
            word = "";
          }
        } else {
          word += buffer[index];
        }
        index++;
      }
    }
    if (word.size() > 0) {
      words.push_back(word);
    }
  }
};
