#ifndef _PARSER_IMPL_H
#define _PARSER_IMPL_H

#include "Nodes.h"
#include "Parser.h"

class ParserImpl : public Parser {
private:
	std::unique_ptr<Lexer> lexer;
	Token currentToken;

	bool checkStmtTerminator = true;

	void LogError(std::string str);
	bool isCurrTokenValue(char c);
	bool isCurrKronkOperator(const std::string& Op);
	int getOpPrec();
	bool currTokenIsAccessOp();
	bool isRightAssociativeOp(const std::string& Op);
	bool isRelationalOp(const std::string& Op);


	std::unique_ptr<Identifier> ParseIdentifier();
	std::unique_ptr<Identifier> ParseForeignSymbol();
	std::unique_ptr<TypeId> ParseTypeId();
	std::unique_ptr<Node> ParseIdOp();

	std::unique_ptr<BooleanLiteral> ParseBooleanLiteral();
	std::unique_ptr<NumericLiteral> ParseNumericLiteral();

	std::unique_ptr<Node> ParseDeclr();

	std::unique_ptr<AnonymousList> ParseAnonymousList();
	std::unique_ptr<AnonymousString> ParseAnonymousString();
	std::unique_ptr<Node> ParseListOperation(std::unique_ptr<Node> list);

	std::unique_ptr<EntityDefn> ParseEntityDefn();
	std::unique_ptr<AnonymousEntity> ParseAnonymousEntity(std::string entityType);
	std::unique_ptr<EntityOperation> ParseEntityOperation(std::unique_ptr<Node> entity);


	std::unique_ptr<Node> ParseExpr();
	std::unique_ptr<Node> ParseBinOpRHS(int ExprPrec, std::unique_ptr<Node> LHS);
	std::unique_ptr<Node> ParseUnaryOp();
	std::unique_ptr<Node> ParseAccessOp(std::unique_ptr<Node> listOrEntity);
	std::unique_ptr<Node> ParseAtomicExpr();
	std::unique_ptr<Node> ParseParenthesizedExpr();

	std::unique_ptr<CompoundStmt> ParseCompoundStmt();

	std::unique_ptr<IfStmt> ParseIfStmt();
	std::unique_ptr<WhileStmt> ParseWhileStmt();

	std::unique_ptr<Prototype> ParseFunctionPrototype();
	std::unique_ptr<FunctionDefn> ParseFunctionDefn();
	std::unique_ptr<ReturnStmt> ParseReturnStmt();
	std::unique_ptr<FunctionCallExpr> ParseFunctionCallExpr(std::string callee);

	std::unique_ptr<IncludeStmt> ParseIncludeStmt();

public:
	ParserImpl(fs::path&& inputFile);
	Token moveToNextToken(bool ignore_subsequent_newlines = false,
	                      bool increment_irgen_line_offset = false) override;
	Token currToken() override;
	std::unique_ptr<Node> ParseStmt(bool caller_is_driver = false) override;
};


#endif