#include "Lexer.h"
#include <iostream>
std::string IdentifierStr;
double numericValue;

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
        * if its a variable reference then '(' is not supposed to be the next token (since variable references only precede binary operators, '=' and ';')
        */
        
        return tok_identifier;
    }

    /** Number: [0-9][0-9.]* Still to fix so decimal point appears only once and not at end 
     * For now all bare numbers are considered double
    **/
    if (isdigit(LastChar) || LastChar == '.') {   
        std::string NumStr;
        int numDecimalPoints = 0;
        do {
            NumStr += LastChar;
            numDecimalPoints += (LastChar == '.') ? 1 : 0;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        numericValue = strtod(NumStr.c_str(), 0);
        return (numDecimalPoints > 0) ? tok_bareDouble : tok_bareInt  ;
    }

    // Handle comments and recursively call scanNextToken after comment body
    if (LastChar == '#') {
        do {
            LastChar = getchar();
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r'); // skip whole line beginning with #

        if (LastChar != EOF)
            return scanNextToken();
    }

    // End of file
    if (LastChar == EOF)
        return tok_eof;

    // After all tests above, just return the character as its ascii value. (For now this takes care of operators and parentheses)
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}