#include "Lexer.h"
#include <iostream>

namespace TokenValue {
    std::string IdentifierStr; // Filled in when an identifier is seen
    double NumericLiteral; // Filled in when an integer or double is seen
    unsigned char NonAlphaNumchar; // Filled in when a non alpha numeric character (except # for comments) is seen 
}


unsigned int currentLexerLine = 1;

static bool verifyNumericStr(std::string string); 

void LogTokenReadError(std::string str) {
    std::cout << "TokenReadError [ Line " << currentLexerLine <<" ]:  " << str << std::endl;
    exit(EXIT_FAILURE);
}

Token scanNextToken() {
    static char LastChar = ' ';

    // Handle whitespace
    while (isspace(LastChar)){
        if(LastChar == '\n') currentLexerLine++;
        LastChar = getchar();
    }
    // identifier: [a-zA-Z][a-zA-Z0-9]*[.]* 
    if (isalpha(LastChar)) { 
        TokenValue::IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            TokenValue::IdentifierStr += LastChar;
        
        if (TokenValue::IdentifierStr == "fonction")
            return Token::FUNCTION_DEFN;

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
        
        if (TokenValue::IdentifierStr == "retourner")
            return Token::RETURN_STMT;

        // /* If not any of the above keywords, then it's either an identifier for a function call or a variable reference
        // *  If a dot immediately follows (with no space) the identifier, then it's gotta be a an entity field selector  
        // */
        // if(LastChar == '.'){
        //     TokenValue::IdentifierStr += LastChar;
        //     LastChar = getchar();
        // }
            
        return Token::IDENTIFIER;
    }

    /** Number: [0-9][0-9.e-]* 
    **/
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
            return ( NumStr.find('.') == std::string::npos ) ? Token::INT_LITERAL : Token::FLOAT_LITERAL;
        }

        LogTokenReadError("Error reading Number " + NumStr);
    }

    // Handle comments && recursively call scanNextToken after comment body
    if (LastChar == '#') {
        do {
            LastChar = getchar();
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r'); // skip whole line beginning with #

        if (LastChar != EOF)
            return scanNextToken();
    }
    
    // End of file
    if (LastChar == EOF)
        return Token::END_OF_FILE;

    // After all tests above, just return the non alphanumeric character as its ascii value.
    TokenValue::NonAlphaNumchar = LastChar;
    LastChar = getchar();
    return Token::NON_ALPHANUM_CHAR;
}

/**Validates num string format. Allows stuff like 1e15, -1e-2. 10.1e2;
 * The lexer doesn't decide whether to treat '-' as a negative number qualifier or a substraction operation. So it just returns the hyphen and it's up to the parser
 * to decide what to do with it. So this verifyNumString function only checks for one hyphen; the one after the exponent.
 **/
static bool verifyNumericStr(std::string string){
    bool dotPresent = false;
    bool ePresent = false;
    bool hyphenPresent = false;
    
    for(auto it = string.begin(); it != string.end(); ++it) {
        // Subsequent characters must be (e) or (.) or (-) or digit. Previous loop before this verifyNumString function was called already checked for that. So
        // we just make sure the number of occurences and position of occurence of each valid character is valid.
        if (*it == '.') {
            if(dotPresent || ePresent) return false; // cannot have decimal after exponent
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