// TokenStream.cpp
// Parses a user search query string into individual tokens for a search engine.
// Handles punctuation stripping, keyword detection, stopword filtering (except inside quotes)

#include <assert.h>
#include <algorithm>
#include <string>
#include <cctype>
#include <sstream>
#include <iostream>
#include "tokenstream.h"
#include "unordered_set"

// Takes in a reference to a query string and tokenizes it for parsing.
TokenStream::TokenStream( std::string &query ) : location( 0 ) {
   std::string current;
   size_t i = 0;

   // Set of punctuation characters to remove from the query
   const std::unordered_set<char> strippedPunctuation = {
      {'!', ',', '.', '?', ';', ':', '-', '+', '/', '~', '`', '>', '<', '@', '#', '$', '%', '^', '&', '*'}
   };

   // Common stopwords to remove unless inside quotes
   std::unordered_set<std::string> stopWords = {
      "a", "able", "about", "above", "according", "accordingly", "across", "actually",
      "after", "afterwards", "again", "against", "aint", "all", "allow", "allows",
      "almost", "alone", "along", "already", "also", "although", "always", "am",
      "among", "amongst", "an", "and", "another", "any", "anybody", "anyhow",
      "anyone", "anything", "anyway", "anyways", "anywhere", "apart", "appear",
      "appreciate", "appropriate", "are", "arent", "around", "as", "aside", "ask",
      "asking", "associated", "at", "available", "away", "awfully", "b", "be", "became",
      "because", "become", "becomes", "becoming", "been", "before", "beforehand",
      "behind", "being", "believe", "below", "beside", "besides", "best", "better",
      "between", "beyond", "both", "brief", "but", "by", "c", "came", "can", "cannot",
      "cant", "cause", "causes", "certain", "certainly", "changes", "clearly", "cmon",
      "co", "com", "come", "comes", "concerning", "consequently", "consider",
      "considering", "contain", "containing", "contains", "corresponding", "could",
      "couldnt", "course", "cs", "currently", "d", "definitely", "described", "despite",
      "did", "didnt", "different", "do", "does", "doesnt", "doing", "dont", "done",
      "down", "downwards", "during", "each", "edu", "eg", "eight", "either", "else",
      "elsewhere", "enough", "entirely", "especially", "et", "etc", "even", "ever",
      "every", "everybody", "everyone", "everything", "everywhere", "ex", "exactly",
      "example", "except", "f", "far", "few", "fifth", "first", "five", "followed",
      "following", "follows", "for", "former", "formerly", "forth", "four", "from",
      "further", "furthermore", "g", "get", "gets", "getting", "given", "gives", "go",
      "goes", "going", "gone", "got", "gotten", "greetings", "h", "had", "hadnt",
      "happens", "hardly", "has", "hasnt", "have", "havent", "having", "he", "her",
      "here", "hereafter", "hereby", "herein", "hereupon", "hers", "herself", "hes",
      "hi", "him", "himself", "his", "hither", "hopefully", "how", "howbeit", "however",
      "i", "id", "ie", "if", "ignored", "ill", "im", "immediate", "in", "inasmuch", "inc",
      "indeed", "indicate", "indicated", "indicates", "inner", "insofar", "instead",
      "into", "inward", "is", "isnt", "it", "itd", "itll", "its", "itself", "j", "just",
      "k", "keep", "keeps", "kept", "know", "known", "knows", "l", "last", "lately",
      "later", "latter", "latterly", "least", "less", "lest", "let", "lets", "like",
      "liked", "likely", "little", "look", "looking", "looks", "ltd", "m", "mainly",
      "many", "may", "maybe", "me", "mean", "meanwhile", "merely", "might", "more",
      "moreover", "most", "mostly", "much", "must", "my", "myself", "n", "name",
      "namely", "nd", "near", "nearly", "necessary", "need", "needs", "neither", "never",
      "nevertheless", "new", "next", "nine", "no", "nobody", "non", "none", "noone",
      "nor", "normally", "not", "nothing", "novel", "now", "nowhere", "o", "obviously",
      "of", "off", "often", "oh", "ok", "okay", "old", "on", "once", "one", "ones", "only",
      "onto", "or", "other", "others", "otherwise", "ought", "our", "ours", "ourselves",
      "out", "outside", "over", "overall", "own", "p", "particular", "particularly", "per",
      "perhaps", "placed", "please", "plus", "possible", "presumably", "probably",
      "provides", "q", "que", "quite", "qv", "r", "rather", "rd", "re", "really",
      "reasonably", "regarding", "regardless", "regards", "relatively", "respectively",
      "right", "s", "said", "same", "saw", "say", "saying", "says", "second", "secondly",
      "see", "seeing", "seem", "seemed", "seeming", "seems", "seen", "self", "selves",
      "sensible", "sent", "serious", "seriously", "seven", "several", "shall", "she",
      "should", "shouldnt", "since", "six", "so", "some", "somebody", "somehow",
      "someone", "something", "sometime", "sometimes", "somewhat", "somewhere",
      "soon", "sorry", "specified", "specify", "specifying", "still", "sub", "such",
      "sup", "sure", "t", "take", "taken", "tell", "tends", "than", "thank", "thanks",
      "thanx", "that", "the", "their", "theirs", "them", "themselves", "then", "thence",
      "there", "thereafter", "thereby", "therefore", "therein", "thereupon", "these",
      "they", "theyd", "theyll", "theyre", "theyve", "think", "third", "this", "thorough",
      "thoroughly", "those", "though", "three", "through", "throughout", "thru", "thus",
      "to", "together", "too", "took", "toward", "towards", "tried", "tries", "truly",
      "try", "trying", "ts", "twice", "two", "u", "un", "under", "unfortunately", "unless",
      "unlikely", "until", "unto", "up", "upon", "us", "use", "used", "useful", "uses",
      "using", "usually", "uucp", "v", "value", "various", "very", "via", "viz", "vs",
      "w", "want", "wants", "was", "wasnt", "way", "we", "wed", "well", "went", "were",
      "werent", "weve", "what", "whatever", "when", "whence", "whenever", "where",
      "whereafter", "whereas", "whereby", "wherein", "whereupon", "wherever",
      "whether", "which", "while", "whither", "who", "whoever", "whole", "whom",
      "whos", "whose", "why", "will", "willing", "wish", "with", "within", "without",
      "wont", "wonder", "would", "wouldnt", "x", "y", "yes", "yet", "you", "youd",
      "youll", "your", "youre", "yours", "yourself", "yourselves", "youve", "z", "zero"
  };


   // Remove unnecessary punctuation from the query string
   query.erase(
      std::remove_if(
         query.begin(), 
         query.end(), 
         [&strippedPunctuation](char c) {
            return strippedPunctuation.count(c) > 0;
         }
      ),
      query.end()
   );

   
   while (i < query.length()) {
      char c = query[i];

      // Finds space in query
      if (std::isspace(c)) {
         if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
         }
         ++i;
         continue;
      }

      // Handle special tokens
      if (c == '&' || c == '|') {
         if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
         }
         if (i + 1 < query.length() && query[i + 1] == c) {
            tokens.push_back(std::string(2, c));  // "&&" or "||"
            i += 2;
         } else {
            tokens.push_back(std::string(1, c));  // "&" or "|"
            ++i;
         }
         continue;
      }

      // Handle parentheses and quote characters as separate tokens
      if (c == '(' || c == ')' || c == '"' || c == '\'') {
         if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
         }
         tokens.push_back(std::string(1, c));  // Push the single character
         ++i;
         continue;
      }

      // Append regular characters to current token
      current += c;
      ++i;
   }

   // Push the last collected token if any
   if (!current.empty()) {
      tokens.push_back(current);
   }

   // Boolean search keywords that should not be lowercased
   const std::unordered_set<std::string> keywords = {"AND", "OR", "NOT"};


   // Lowercase tokens and remove stopwords unless inside quotes
   if (tokens.size() > 1) {
      std::vector<std::string> filtered;
      bool inQuotes = false;
   
      for (const std::string& word : tokens) {
         if (word == "\"" || word == "'") {
            inQuotes = !inQuotes;  // Toggle quote state
            filtered.push_back(word);
            continue;
         }
   
         // preserve keywords
         if (keywords.find(word) != keywords.end()) {
            filtered.push_back(word);
            continue;
         }
   
         // Convert to lowercase
         std::string lower = word;
         std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
   
         // Preserve word if it's not a stopword, OR it's inside quotes
         if (inQuotes || stopWords.find(lower) == stopWords.end()) {
            filtered.push_back(lower);
         }
      }
   
      bool containsNonStopword = false;
      bool containsQuoted = false;

      for (const std::string& token : filtered) {
         if (token == "\"" || token == "'") {
            inQuotes = !inQuotes;
            containsQuoted = true;  // Just mark that any quote block exists
            continue;
         }
      
         if (keywords.count(token)) continue;
      
         if (inQuotes || stopWords.find(token) == stopWords.end()) {
            containsNonStopword = true;
         }
      }

      if (!containsNonStopword && !containsQuoted && !tokens.empty()) {
         std::string first = tokens[0];
         std::transform(first.begin(), first.end(), first.begin(), ::tolower);
         tokens = {first};
      } else {
         tokens = std::move(filtered);
      }

   }
   else {
      std::transform(tokens[0].begin(), tokens[0].end(), tokens[0].begin(), ::tolower);
   }
   
   std::cout << "\nParsed search query:" << std::endl;
   for (std::string& token : tokens) {
      std::cout << token << ", ";
   }
   std::cout << '\n' << std::endl;
}

// Matches and consumes the next token if it equals `in`
bool TokenStream::Match(const std::string &in) {
   if (location < tokens.size() && tokens[location] == in) {
      ++location;  // Move to the next token
      return true;
   }
   return false;
}

// Returns and consumes the next word
std::string TokenStream::GetWord() {
   if (location < tokens.size()) {
      return tokens[location++];
   }
   return "";
}

// Peeks at the next token without consuming it
std::string TokenStream::Peek() {
   if (location < tokens.size()) {
      return tokens[location];
   }
   return "";
}

// Checks if all tokens have been consumed
bool TokenStream::AllConsumed() const {
   return location >= tokens.size();
}
