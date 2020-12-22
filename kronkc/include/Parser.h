#ifndef _PARSER_H_
#define _PARSER_H_

#include "Lexer.h"
#include "Nodes.h"


Token moveToNextToken();

std::unique_ptr<Node> ParseStmt();

#endif