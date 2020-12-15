#ifndef _LEXER_H
#define _LEXER_H

/**
 * Each token returned by this lexer includes a token code and potentially some metadata. It returns tokens [0-255] if it is an unknown character, otherwise one
 * of these for known things.
 **/

#include <string>

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

  NON_ALPHA_NUM_CHAR // shows that currentToken holds a non alphanumeric character
};

namespace TokenValue {
    extern std::string IdentifierStr;
    extern double NumericLiteral;
    extern unsigned char NonAlphaNumchar;
}

extern unsigned int currentLexerLine; // keeps track of current line being parsed in source file

Token scanNextToken();

#endif