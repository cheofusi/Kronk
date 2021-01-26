#include "ParserImpl.h"


// Error Logging
std::nullptr_t ParserImpl::LogError(std::string str) {
    std::cout << "ParserError [Line "<< currentLexerLine << "]:  " << str << std::endl;
    exit(EXIT_FAILURE);
    
    return nullptr;
}


// checks if the current token value is a non alphanumeric character (that is not a kronk operator)
// corresponding to c
bool ParserImpl::isCurrTokenValue(char c) {
    return  ( (currentToken == Token::NON_ALPHANUM_CHAR) and (lexer->NonAlphaNumchar == c) ) ? true : false;
}


// checks if the current token value is a kronk operator corresponding to op
bool ParserImpl::isCurrKronkOperator(std::string op) {
    return ( (currentToken == Token::KRONK_OPERATOR) and (lexer->IdentifierStr == op) ) ? true : false;
}


// Get the precedence of the pending binary operator token.
int ParserImpl::getOpPrec() {
    int prec = -1;
    if(currentToken == Token::KRONK_OPERATOR) {
        prec = KronkOperators.at(lexer->IdentifierStr);
    }

    return prec;
}
