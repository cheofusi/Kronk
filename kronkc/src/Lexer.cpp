#include "Lexer.h"


Lexer::Lexer(std::string& inputFile) {
    ifile = std::ifstream(inputFile);
    Attr::CurrentLexerLine = 1;
}


Token Lexer::scanNextToken() {
    // Handle whitespace
    while (LastChar == ' ') {
        LastChar = ReadNextChar();
    }

    // identifier: [a-zA-Z_][a-zA-Z0-9_]* 
    if (isalpha(LastChar) or (LastChar == '_')) {
        IdentifierStr.clear();
        do {
            IdentifierStr += LastChar;
            LastChar = ReadNextChar();

        } while (isalnum(LastChar) or (LastChar == '_'));
        
        // TODO: change all this if statements to an unordered map.
        if (IdentifierStr == "fn")
            return Token::FUNCTION_DEFN;

        if (IdentifierStr == "ret")
            return Token::RETURN_STMT;

        if (IdentifierStr == "Entite")
            return Token::ENTITY_DEFN;
        
        if (IdentifierStr == "soit")
            return Token::DECLR_STMT;
        
        if (IdentifierStr == "Si")
            return Token::IF_STMT;

        if (IdentifierStr == "Sinon")
            return Token::ELSE_STMT;
    
        if (IdentifierStr == "Tantque")
            return Token::WHILE_STMT;
        
        if ((IdentifierStr == "vrai") or (IdentifierStr == "faux"))
            return Token::BOOLEAN_LITERAL;
        
        if(Attr::KronkOperators.find(IdentifierStr) != Attr::KronkOperators.end())
            return Token::KRONK_OPERATOR;
        
        return Token::IDENTIFIER;
    }




    // string literal: ["]*["]
    if(LastChar == 34) { // ascii for double apostrophe
        IdentifierStr.clear();
        LastChar = ReadNextChar();
        while (LastChar != 34) {
            if(LastChar == EOF)
                LogTokenReadError("Incomplete string !!");

            IdentifierStr += LastChar;
            LastChar = ReadNextChar();
        }

        LastChar = ReadNextChar(); // get rid of the closing apostrophe
        return Token::STRING_LITERAL;    
    }

    

    // Number: [0-9][0-9.e-]* 
    if (isdigit(LastChar)) {   
        std::string NumStr;
        char previousChar;
        do {
            NumStr += LastChar;
            previousChar = LastChar;
            LastChar = ReadNextChar();
            if(LastChar == '-' && previousChar != 'e') { // test determining if negative sign is part of NumStr.
                break;
            }
                
        } while (isdigit(LastChar) || LastChar == '.' || LastChar == 'e' || LastChar == '-');
        
        if (verifyNumericStr(NumStr)) {
            NumericLiteral = strtod(NumStr.c_str(), 0);
            return Token::NUMERIC_LITERAL;
        }

        LogTokenReadError("Error reading Number " + NumStr);
    }

    // Non alphanumeric operators
    LastCharStr.clear();
    LastCharStr = static_cast<char>(LastChar);
    if(Attr::KronkOperators.find(LastCharStr) != Attr::KronkOperators.end()) {
        IdentifierStr = LastCharStr;
        LastChar = ReadNextChar();
        if( (LastChar == '*') or (LastChar == '=') or ((LastChar == '<') or ((LastChar == '>')))) {
            // all 2-nonalphanum-character operators are either followed by a *, = , < or > 
            IdentifierStr += static_cast<char>(LastChar);
            LastChar = ReadNextChar();
        }

        return Token::KRONK_OPERATOR;
    }

    // Handle comments && recursively call scanNextToken after comment body
    if (LastChar == '#') {
        do {
            LastChar = ReadNextChar();
        } while (LastChar != EOF and LastChar != '\n'); // skip whole line beginning with #

        // skip all subsequent new lines
        if (LastChar != EOF) {
            do {
                Attr::CurrentLexerLine++;
                LastChar = ReadNextChar();
            } while (LastChar == '\n');

            return scanNextToken();
        }
    }
    
    if (LastChar == '\n') {
        Attr::CurrentLexerLine++;
        LastChar = ReadNextChar();
        NonAlphaNumchar = LastChar;
        return Token::NEW_LINE;
    }

    if (LastChar == EOF) {
        ifile.close();
        return Token::END_OF_FILE;
    }

    // After all tests above, just return the non alphanumeric character as its ascii value.
    NonAlphaNumchar = LastChar;
    LastChar = ReadNextChar();
    return Token::NON_ALPHANUM_CHAR;
}


void Lexer::LogTokenReadError(std::string str) {
    std::cout << "TokenReadError [ Line " << Attr::CurrentLexerLine <<" ]:  " << str << std::endl;
    exit(EXIT_FAILURE);
}


int Lexer::ReadNextChar() {
    // emulates getchar() function
    return ifile.get();
} 



// Validates num string format. Allows stuff like 1e15, -1e-2. 10.1e2;
// The lexer doesn't decide whether to treat '-' as a negative number qualifier or a substraction operation.
// So it just returns the hyphen and it's up to the parser
// to decide what to do with it. So this verifyNumString function only checks for one hyphen; the one after the 
// exponent.
bool Lexer::verifyNumericStr(std::string string) {
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