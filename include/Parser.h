#ifndef _PARSER_H_
#define _PARSER_H_

#include "Lexer.h"
#include "node.h"

#include <map>

extern int currentToken;
int moveToNextToken();

extern std::map<char, int> BinaryOperatorPrecedence;
// GetTokPrecedence - Get the precedence of the pending binary operator token.
int GetTokenPrecedence();

auto LogError(const char *Str);

std::unique_ptr<IntegerNode> ParseBareInteger(bool);
std::unique_ptr<FloatNode> ParseBareFloat(bool); 
std::unique_ptr<Identifier> ParseIdentifier();
std::unique_ptr<Node> ParseParenthesizedExpr();

std::unique_ptr<Node> ParseVariableDeclaration();
std::unique_ptr<Node> ParseExpression();
std::unique_ptr<Node> ParseBinOpRHS(int ExprPrec, std::unique_ptr<Node> LHS);
std::unique_ptr<Node> ParsePrimary();

std::unique_ptr<Node> ParseIdentifierOrCall();
std::unique_ptr<IfNode> ParseIfExpr();
std::unique_ptr<WhileNode> ParseWhileLoop();
std::unique_ptr<FunctionDefinition> ParseFunctionDefinition();

#endif