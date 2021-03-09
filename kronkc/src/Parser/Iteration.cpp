#include "ParserImpl.h"


std::unique_ptr<WhileStmt> ParserImpl::ParseWhileStmt() {
	moveToNextToken();  // eat Tantque
	auto cond = ParseExpr();
	if (not isCurrTokenValue('{')) LogError("Expected '{' after loop declaration");

	auto whileBody = ParseCompoundStmt();

	return std::make_unique<WhileStmt>(std::move(cond), std::move(whileBody));
}