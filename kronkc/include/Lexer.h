#ifndef _LEXER_H
#define _LEXER_H

#include "Attributes.h"
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
  BOOLEAN_LITERAL,
  NUMERIC_LITERAL,
  STRING_LITERAL,

  KRONK_OPERATOR,

  NEW_LINE,

  NON_ALPHANUM_CHAR // shows that currentToken holds a non alphanumeric character
};


namespace TokenValue {
    extern std::string IdentifierStr;
    extern double NumericLiteral;
    extern unsigned char NonAlphaNumchar;
}


Token scanNextToken();

#endif