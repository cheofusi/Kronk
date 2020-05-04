#include "Lexer.h"
#include <iostream>
std::string IdentifierStr;
double numericValue;

int verifyNumString(std::string string); // validates num string format. Allows stuff like 1e15, -1e-2. 10.1e2;

auto LogTokenError(std::string str) {
    std::cout << "TokenError " << str << std::endl;
    exit(EXIT_FAILURE);
    return 0;
}

int scanNextToken(){
    static char LastChar = ' ';

    // Handle whitespace
    while (isspace(LastChar))
        LastChar = getchar();

    // identifier: [a-zA-Z][a-zA-Z0-9]*
    if (isalpha(LastChar)) { 
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;
        
        if (IdentifierStr == "fonction")
            return tok_function;
        if (IdentifierStr == "entier"){
            IdentifierStr = "i32";
            return tok_INT;
        }
        if (IdentifierStr == "reel"){
            IdentifierStr = "double";
            return tok_DOUBLE;
        }
        if (IdentifierStr == "Si")
            return tok_if;
        
        if (IdentifierStr == "alors")
            return tok_alors;

        if (IdentifierStr == "Sinon")
            return tok_else;
        if (IdentifierStr == "retourner")
            return tok_return;
        if (IdentifierStr == "Tantque")
            return tok_whileLoop;

        /* If not any of the above keywords, then it's either a function call or a variable reference/identifer
        * if its a variable reference then '(' is not supposed to be the next token (since variable references only precede binary operators, '=' && ';')
        */
        
        return tok_identifier;
    }

    /** Number: [0-9][0-9.]* 
    **/
    if (isdigit(LastChar) || LastChar == '.') {   
        std::string NumStr;
        char previousChar;
        do {
            NumStr += LastChar;
            previousChar = LastChar;
            LastChar = getchar();
            if(LastChar == '-' && previousChar != 'e'){
                break;
            }
                
        } while (isdigit(LastChar) || LastChar == '.' || LastChar == 'e' || LastChar == '-');
        
        int verifResult = verifyNumString(NumStr);
        return (verifResult) ? verifResult : LogTokenError("Error reading Number " + NumStr);
    }

    // Handle comments && recursively call scanNextToken after comment body
    if (LastChar == '#') {
        do {
            LastChar = getchar(  );
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r'); // skip whole line beginning with #

        if (LastChar != EOF)
            return scanNextToken();
    }

    // End of file
    if (LastChar == EOF)
        return tok_eof;

    // After all tests above, just return the character as its ascii value. (For now this takes care of operators && parentheses)
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

/**
 * The lexer doesn't decide whether to treat '-' as a negative number qualifier or a substraction operation. So it just returns the hyphen and it's up to the parser
 * to decide what to do with it. So this verifyNumString function only checks for one hyphen; the one after the exponent.
 * For now we don't support nested exponents ex 1e10e10e10
 **/
int verifyNumString(std::string string){
    bool dotPresent = false;
    bool ePresent = false;
    bool hyphenPresent = false;
    
    dotPresent = (string[0] == '.') ? true : false;
    
    for(int i = 1; i < string.size(); ++i){
        // Subsequent characters must be (e) or (.) or (-) or digit. Previous loop before this verifyNumString function was called already checked for that. So
        // we just make sure the number and position of occurence of each valid character is valid.
        if (string[i] == '.'){
            if(dotPresent || ePresent) return false; // cannot have decimal after exponent
            //if(i == string.size() - 1) return false; // for now lets allow user to have floats terminate with decimals
            dotPresent = true;
        }
        
        if (string[i] == '-'){
            if(hyphenPresent) return false;
            if(string[i-1] != 'e') return false;
        }
        
        if (string[i] == 'e'){
            if(ePresent) return false;
            if (!isdigit(string[i-1])) return false; // must proceed a digit;
            if(i == string.size() - 1) return false; // number cannot terminate with exponent 
            if (!isdigit(string[i+1]) && !(string[i+1] == '-')) return false; // what proceeds e must be a digit or hyphen
            ePresent = true;
        }
    }
    // All test are verified. Now convert to C++ double
    numericValue = strtod(string.c_str(), 0);
    return (dotPresent || ePresent) ? tok_bareDouble : tok_bareInt; 
}