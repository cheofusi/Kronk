#include "Names.h"
#include "ParserImpl.h"


std::unique_ptr<Prototype> ParserImpl::ParseFunctionPrototype() {
	auto fnName = ParseIdentifier();
	auto mangledFnName = names::mangleNameAsLocalFunction(fnName->name);

	if (not isCurrTokenValue('(')) LogError("Expected '(' after function name");

	moveToNextToken();  // eat (

	std::vector<std::string> paramNames;
	std::vector<std::unique_ptr<TypeId>> paramTypeIds;

	if (isCurrTokenValue(')')) {
		// no parameter function call
		moveToNextToken();  // eat )
	}

	else {
		while (true) {
			if (currentToken != Token::IDENTIFIER)
				LogError("Expected an identifier string for a parameter");
			auto paramName = ParseIdentifier();

			if (not isCurrTokenValue(':'))
				LogError("Expected ':' after parameter << " + paramName->name + " >>");
			moveToNextToken();  // eat :

			if (currentToken != Token::IDENTIFIER)
				LogError("Expected an identifier string for a parameter type");
			auto paramTypeId = ParseTypeId();

			paramNames.push_back(std::move(paramName->name));
			paramTypeIds.push_back(std::move(paramTypeId));

			if (isCurrTokenValue(')')) break;

			if (not isCurrTokenValue(',')) LogError("Expected ',' or ')' after parameter type");
			moveToNextToken();  // eat ,
		}

		moveToNextToken();  // eat )
	}


	// parse return type
	if (currentToken != Token::IDENTIFIER)
		LogError("Expected identifier string for function return type");
	auto fnTypeId = ParseTypeId();

	if (paramNames.size()) {
		return std::make_unique<Prototype>(std::move(mangledFnName), std::move(fnTypeId),
		                                   std::move(paramNames), std::move(paramTypeIds));
	}

	return std::make_unique<Prototype>(std::move(mangledFnName), std::move(fnTypeId));
}


std::unique_ptr<FunctionDefn> ParserImpl::ParseFunctionDefn() {
	moveToNextToken();  // eat fn keyword
	if (currentToken != Token::IDENTIFIER) LogError("Expected an identifier for the function name");

	auto Proto = ParseFunctionPrototype();

	if (not isCurrTokenValue('{')) LogError("Expected '{' ");

	auto fnBody = ParseCompoundStmt();
	return std::make_unique<FunctionDefn>(std::move(Proto), std::move(fnBody));
}


std::unique_ptr<ReturnStmt> ParserImpl::ParseReturnStmt() {
	moveToNextToken();     // eat tok_return
	auto E = ParseExpr();  // value to return
	return std::make_unique<ReturnStmt>(std::move(E));
}


std::unique_ptr<FunctionCallExpr> ParserImpl::ParseFunctionCallExpr(std::string callee) {
	// callee is already mangled

	moveToNextToken();            // eat '('
	if (isCurrTokenValue(')')) {  // no param function call
		moveToNextToken();        // eat ')'
		return std::make_unique<FunctionCallExpr>(std::move(callee));
	}

	std::vector<std::unique_ptr<Node>> Args;
	while (true) {
		auto arg = ParseExpr();
		Args.push_back(std::move(arg));
		if (isCurrTokenValue(')')) break;

		if (not isCurrTokenValue(',')) {
			LogError("Expected ','");
		}

		moveToNextToken();  // eat ",".
	}

	moveToNextToken();  // eat ")
	return std::make_unique<FunctionCallExpr>(std::move(callee), std::move(Args));
}