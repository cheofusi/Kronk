#include "Lexer.h"

#include <iostream>

namespace TokenValue {
    std::string IdentifierStr; // Filled in when an identifier or string literal or kronk operator is seen
    double NumericLiteral; // Filled in when an integer or double is seen
    unsigned char NonAlphaNumchar; // Filled in otherwise 
}

char LastChar = ' ';

uint32_t currentLexerLine = 1;

static bool verifyNumericStr(std::string string); 


void LogTokenReadError(std::string str) {
    std::cout << "TokenReadError [ Line " << currentLexerLine <<" ]:  " << str << std::endl;
    exit(EXIT_FAILURE);
}


Token scanNextToken() {
    // Handle whitespace
    while (LastChar == ' ') {
        LastChar = getchar();
    }

    // identifier: [a-zA-Z_][a-zA-Z0-9_]* 
    if (isalpha(LastChar) or (LastChar == '_')) {
        TokenValue::IdentifierStr.clear();
        do {
            TokenValue::IdentifierStr += LastChar;
            LastChar = getchar();

        } while (isalnum(LastChar) or (LastChar == '_'));
        
        if (TokenValue::IdentifierStr == "fn")
            return Token::FUNCTION_DEFN;

        if (TokenValue::IdentifierStr == "ret")
            return Token::RETURN_STMT;

        if (TokenValue::IdentifierStr == "Entite")
            return Token::ENTITY_DEFN;
        
        if (TokenValue::IdentifierStr == "soit")
            return Token::DECLR_STMT;
        
        if (TokenValue::IdentifierStr == "Si")
            return Token::IF_STMT;

        if (TokenValue::IdentifierStr == "Sinon")
            return Token::ELSE_STMT;
    
        if (TokenValue::IdentifierStr == "Tantque")
            return Token::WHILE_STMT;
        
        if ((TokenValue::IdentifierStr == "vrai") or (TokenValue::IdentifierStr == "faux"))
            return Token::BOOLEAN_LITERAL;
        
        if(KronkOperators.find(TokenValue::IdentifierStr) != KronkOperators.end())
            return Token::KRONK_OPERATOR;

        return Token::IDENTIFIER;
    }


    // string literal: " * "
    if(LastChar == 34) { // ascii for double apostrophe
        TokenValue::IdentifierStr.clear();
        LastChar = getchar();
        while (LastChar != 34) {
            TokenValue::IdentifierStr += LastChar;
            LastChar = getchar();
        }

        LastChar = getchar(); // get rid of the closing apostrophe
        return Token::STRING_LITERAL;    
    }


    // Number: [0-9][0-9.e-]* 
    if (isdigit(LastChar)) {   
        std::string NumStr;
        char previousChar;
        do {
            NumStr += LastChar;
            previousChar = LastChar;
            LastChar = getchar();
            if(LastChar == '-' && previousChar != 'e') { // test determining if negative sign is part of NumStr.
                break;
            }
                
        } while (isdigit(LastChar) || LastChar == '.' || LastChar == 'e' || LastChar == '-');
        
        if (verifyNumericStr(NumStr)) {
            TokenValue::NumericLiteral = strtod(NumStr.c_str(), 0);
            return Token::NUMERIC_LITERAL;
        }

        LogTokenReadError("Error reading Number " + NumStr);
    }

    
    // Non alphanumeric operators
    auto LastCharStr = std::string(&LastChar);
    if(KronkOperators.find(LastCharStr) != KronkOperators.end()) {
        TokenValue::IdentifierStr = LastCharStr;
        LastChar = getchar();
        return Token::KRONK_OPERATOR;
    }

    // Handle comments && recursively call scanNextToken after comment body
    if (LastChar == '#') {
        do {
            LastChar = getchar();
        } while (LastChar != EOF and LastChar != '\n'); // skip whole line beginning with #

        // skip all subsequent new lines
        if (LastChar != EOF) {
            do {
                currentLexerLine++;
                LastChar = getchar();
            } while (LastChar == '\n');

            return scanNextToken();
        }
    }
    
    if (LastChar == '\n') {
        currentLexerLine++;
        LastChar = getchar();
        TokenValue::NonAlphaNumchar = LastChar;
        return Token::NEW_LINE;
    }

    if (LastChar == EOF)
        return Token::END_OF_FILE;

    // After all tests above, just return the non alphanumeric character as its ascii value.
    TokenValue::NonAlphaNumchar = LastChar;
    LastChar = getchar();
    return Token::NON_ALPHANUM_CHAR;
}


/**Validates num string format. Allows stuff like 1e15, -1e-2. 10.1e2;
 * The lexer doesn't decide whether to treat '-' as a negative number qualifier or a substraction operation.
 *  So it just returns the hyphen and it's up to the parser
 * to decide what to do with it. So this verifyNumString function only checks for one hyphen; the one after the 
 * exponent.
 **/
static bool verifyNumericStr(std::string string) {
    bool dotPresent = false;
    bool ePresent = false;
    bool hyphenPresent = false;
    
    for(auto it = string.begin(); it != string.end(); ++it) {
        // Subsequent characters must be (e) or (.) or (-) or digit. Previous loop before this verifyNumString function
        // was called already checked for that. So we just make sure the number of occurences and position of occurence
        // of each valid character is valid.
        if (*it == '.') {
            if(dotPresent or ePresent) return false; // cannot have decimal after exponent
            //if(i == string.size() - 1) return false; // for now lets allow user to have floats terminate with decimals
            dotPresent = true;
        }
        
        if (*it == '-') {
            if(hyphenPresent) return false;
            if( *(it-1) != 'e') return false;
        }
        
        if (*it == 'e') {
            if(ePresent) return false;
            if (not (isdigit(*(it-1)))) return false; // must proceed a digit;
            if((it + 1) == string.end()) return false; // number cannot terminate with exponent 
            if ( (not isdigit(*(it + 1))) and (not (*(it + 1) == '-')) ) // what proceeds e must be a digit or hyphen
                return false; 
            ePresent = true;
        }
    }
    // All test are verified. numeric string is valid
    return true; 
}