#ifndef _PARSER_H_
#define _PARSER_H_

#include "Lexer.h"
#include "Nodes.h"

extern Token currentToken;
extern void LogProgress(std::string str);

Token moveToNextToken();

std::unique_ptr<Node> ParseStmt();

#endif