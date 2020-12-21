#ifndef _LEXER_H
#define _LEXER_H

#include <string>

// enum for Token types
enum class Token {
  END_OF_FILE,

  FUNCTION_DEFN,
  ENTITY_DEFN,

  DECLR_STMT,

  IF_STMT,
  ELSE_STMT,
  
  WHILE_STMT,
  FOR_STMT,

  RETURN_STMT,

  IDENTIFIER,
  INT_LITERAL,
  FLOAT_LITERAL,

  NON_ALPHANUM_CHAR // shows that currentToken holds a non alphanumeric character
};


namespace TokenValue {
    extern std::string IdentifierStr;
    extern double NumericLiteral;
    extern unsigned char NonAlphaNumchar;
}

extern unsigned int currentLexerLine; // keeps track of current line being parsed in source file

Token scanNextToken();

#endif