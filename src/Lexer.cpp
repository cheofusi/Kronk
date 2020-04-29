#include "Lexer.h"
#include <iostream>
std::string IdentifierStr;
double numericValue;

int verifyNumString(std::string string); // validates num string format. Allows stuff like 1e15, -1e-2. 10.1e2;
bool expectingMinus = false;

auto LogTokenError(std::string str) {
    std::cout << "TokenError " << str << std::endl;
    exit(EXIT_FAILURE);
    return 0;
}

int scanNextToken(){
    static char LastChar = ' ';

    if(expectingMinus){
       expectingMinus = false;
       int returnVal = LastChar;
       LastChar = getchar();
       return returnVal;
    }

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

    /** Number: [0-9][0-9.]* Still to fix so decimal point appears only once && not at end 
     *  Write external function to validate number string && then call after loop
    **/
    if (isdigit(LastChar) || LastChar == '.' || LastChar == '-') {   
        std::string NumStr;
        char previousChar;
        do {
            NumStr += LastChar;
            previousChar = LastChar;
            LastChar = getchar();
            if(LastChar == '-' && previousChar != 'e'){
                expectingMinus = true;
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


int verifyNumString(std::string string){
    bool dotPresent = false;
    bool ePresent = false;
    int hyphenCount = 0; //at most 2 hyphens && only appears at the beginning of string &&/or immediately after e e.g -1e-1
    
    // first character must be either digit or (-) or (.)
    if (string[0] != '-' && string[0] != '.' && !(string[0] >= '0' && string[0] <= '9'))
        return false;
    dotPresent = (string[0] == '.') ? true : false;
    if (string[0] == '-')
        hyphenCount++;

    
    for(int i = 1; i < string.size(); ++i){
        // Subsequent characters must be (e) or (.) or (-) or digit
        if (string[i] != '.' && string[i] != '-' && string[i] != 'e' && !(string[i] >= '0' &&  string[i] <= '9'))
            return false;
        
        if (string[i] == '.'){
            if(dotPresent or ePresent) return false;
            if(i == string.size() - 1) return false; 
            dotPresent = true;
        }
        
        if (string[i] == '-'){
            if(hyphenCount > 2) return false;
            if(string[i-1] != 'e') return false;
            hyphenCount++;
        }
        
        if (string[i] == 'e'){
            if(ePresent) return false;
            if (!(string[i-1] >= '0' && string[i-1] <= '9')) return false;
            if(i == string.size() - 1) return false; 
            if  (!(string[i+1] >= '0' && string[i+1] <= '9') && !(string[i+1] == '-')) return false;
            ePresent = true;
        }
    }
    // All test are verified. Now convert to C++ double
    numericValue = strtod(string.c_str(), 0);
    return (dotPresent || ePresent) ? tok_bareDouble : tok_bareInt; 
}