#include "ParserImpl.h"


std::unique_ptr<Node> ParserImpl::ParseDeclr() {
	moveToNextToken();  // eat soit keyword
	if (currentToken != Token::IDENTIFIER)
		LogError("Expected identifier string for the name of the variable to be declared ");

	auto variableName = ParseIdentifier();  // parse variable name

	if (isCurrTokenValue(':')) {
		// simple declaration
		moveToNextToken();  // eat ':'
		if (currentToken != Token::IDENTIFIER) LogError("Expected a type identifier");
		auto variableType = ParseTypeId();

		return std::make_unique<Declr>(std::move(variableName->name), std::move(variableType));
	}

	if (isCurrKronkOperator("=")) {
		// declaration + initialization ex:
		// soit y = x + 8.2
		// soit lst = [1, 2, 3]
		moveToNextToken();  // eat equality sign
		auto rhs = ParseExpr();
		return std::make_unique<InitDeclr>(std::move(variableName->name), std::move(rhs));
	}

	// we got a nonalphanumric character that was neither ':' nor '='
	LogError("Expected either ':' or '=' after start of declaration");
}