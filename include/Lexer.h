#ifndef _LEXER_H
#define _LEXER_H

/**
 * Each token returned by this lexer includes a token code and potentially some metadata. It returns tokens [0-255] if it is an unknown character, otherwise one
 * of these for known things.
 **/

#include <string>

enum Token {
  tok_eof = -1,

  tok_function = -2,
  tok_extern = -3,

  tok_INT = -4, // used for variable declaration type
  tok_DOUBLE = -5,

  tok_if = -6,
  tok_else = -7,
  tok_alors = -8,

  tok_identifier = -9, //for variables and function names
  tok_bareInt = -10,
  tok_bareDouble = -11,
  
  tok_whileLoop = -12,
  tok_return = -14
};


extern std::string IdentifierStr; // Filled in if tok_identifier
extern double numericValue; // Filled in if tok_bareInt or bareDouble


int scanNextToken();

#endif