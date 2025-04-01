
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include "../../utils/cunique_ptr.h"
enum TokenType
{
// an explicit tken to mark the end of file/end of file stream for the token stream 
TokenInvalid = -1, TokenEOF = 0,
// defined specific terms in the query language
TokenOR, TokenVerticalBar, TokenOr, TokenAND, TokenAmpersand, 
TokenAnd, TokenPlus, TokenMinus, TokenNOT, 
TokenDoubleQuote, TokenLeftParenthesis, TokenRightParenthesis, 
// anything not in these defined specific terms is marked as a word in the query
TokenWord
};
class Token
{
public:
    TokenType GetTokenType() const { return type; }
    // 
    Token(std::string input) {
        // hash table for the constructor for easy lookup
        static const std::unordered_map<std::string, TokenType> tokenMap = {
            {"OR", TokenOR}, {"|", TokenVerticalBar}, {"||", TokenOr},
            {"AND", TokenAND}, {"&", TokenAmpersand}, {"&&", TokenAnd},
            {"+", TokenPlus}, {"-", TokenMinus}, {"NOT", TokenNOT},
            {"\"", TokenDoubleQuote}, {"(", TokenLeftParenthesis}, {")", TokenRightParenthesis}, 
            {"", TokenEOF}
        };
        if (input.empty()) {
            tokenString = "";
            type = TokenEOF;
            return;
        }
        // if it exists in the hash table just give it its own pre-existing type 
        auto iter = tokenMap.find(input); 
        if (iter != tokenMap.end()) {
            tokenString = iter->first; 
            type = iter->second; 
        }
        // otherwise treat it as a search query
        else {
            tokenString = input; 
            static const std::unordered_set<char> strippedPunctuation = {
                {'!', ',', '.', '?', ';'}
                };
                if (strippedPunctuation.count(tokenString[tokenString.size() - 1]) > 0) {
                    tokenString.erase(tokenString.size() - 1); 
                }
            type = TokenWord; 
        }
    }

    const char *TokenString() const {
    return tokenString.c_str(); 
    }
    std::string tokenString;
    TokenType type;
private:
};

class TokenStream {
public:
    Token *CurrentToken() {
        return currentToken; 
    }
    Token *TakeToken( ) {
        std::string token;
        auto old = currentToken; 
        if (input >> token) {
            delete currentToken;
            currentToken = new Token(token); 
        }
        else {
            std::cout << "Search query stream is empty.";
            delete currentToken; 
            currentToken = new Token(""); 
        }
        return old; 
    }
    bool Match( TokenType t ) {
        if (currentToken->type == t) {
            TakeToken(); 
            return true;
        } 
        else {
            return false; 
        }
    }
    TokenStream( );
    ~TokenStream() { delete currentToken; } 
    TokenStream( char *filename );
    // TokenStream( ifstream &is ); 
    TokenStream(std::string query) {
        // declare the input and curent token string
        currentTokenString = query; 
        std::stringstream s(query);  
        // string stream doesn't have an = operator, not copy assignable
        std::swap(input, s); 
    }
private:
   // std::ifstream input; commented out for now dont know why we need this but if we do then uncomment
    std::stringstream input;
    std::string currentTokenString;

    Token* currentToken = new Token(""); // TBD: make this a unique ptr for easy deallocation
};
