#ifndef _LEXER_H
#define _LEXER_H

#include <fstream>

#include "Attributes.h"


// enum for Token types
enum class Token {
	END_OF_FILE,

	INCLUDE_STMT,

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

	NON_ALPHANUM_CHAR  // shows that currentToken holds a non alphanumeric character
};


class Lexer {
	std::ifstream ifile;
	int LastChar = ' ';

	int ReadNextChar();
	void LogTokenReadError(std::string str);
	bool verifyNumericStr(const std::string& string);

public:
	// Filled in when an identifier or string literal or kronk operator is seen
	std::string IdentifierStr;
	// Filled in when an integer or double is seen
	double NumericLiteral;
	// Filled in otherwise
	unsigned char NonAlphaNumchar;

	Lexer(fs::path&& inputFile);
	Token scanNextToken(bool increment_irgen_line_offset);
};


#endif