#include <type_traits>

#include "ParserImpl.h"


std::unique_ptr<Parser> Parser::CreateParser(fs::path&& inputFile) {
	return std::make_unique<ParserImpl>(std::move(inputFile));
}


ParserImpl::ParserImpl(fs::path&& inputFile) { lexer = std::make_unique<Lexer>(std::move(inputFile)); }


std::unique_ptr<Node> ParserImpl::ParseStmt(bool caller_is_driver) {
	std::unique_ptr<Node> stmt_ast;
	bool parsed_include_mode_ast = false;

	switch (currentToken) {
		case Token::NEW_LINE:
			moveToNextToken(true);
			return ParseStmt(caller_is_driver);

		case Token::INCLUDE_STMT:
			stmt_ast = ParseIncludeStmt();
			LogProgress("Read Include statement");
			parsed_include_mode_ast = true;
			break;

		case Token::FUNCTION_DEFN:
			stmt_ast = ParseFunctionDefn();
			LogProgress("Read Function Definition");
			parsed_include_mode_ast = true;
			break;

		case Token::ENTITY_DEFN:
			stmt_ast = ParseEntityDefn();
			LogProgress("Read Entity Definition");
			parsed_include_mode_ast = true;
			break;

		case Token::DECLR_STMT:
			stmt_ast = ParseDeclr();
			LogProgress("Read Declaration");
			break;

		case Token::IF_STMT:
			stmt_ast = ParseIfStmt();
			LogProgress("Read if statement");
			break;

		case Token::WHILE_STMT:
			stmt_ast = ParseWhileStmt();
			LogProgress("Read while statement");
			break;

		case Token::RETURN_STMT:
			stmt_ast = ParseReturnStmt();
			LogProgress("Read return statement");
			break;

		default:
			stmt_ast = ParseExpr();
			LogProgress("Read an Expression");
			break;
	}

	if (checkStmtTerminator) {
		if ((currentToken != Token::NEW_LINE) and (not isCurrTokenValue(';')) and
		    (currentToken != Token::END_OF_FILE)) {
			LogError("A statment must end with a new line or semi-colon !!");
		}

		if (currentToken != Token::END_OF_FILE) moveToNextToken(true, true);

	}

	else {
		checkStmtTerminator = true;
	}

	if (Attr::INCLUDE_MODE and caller_is_driver) {
		// we're compiling this file in inclue mode. So only return INCLUDE_STMT,
		// FUNCTION_DEFN and ENTITY_DEFN tothe driver

		return (parsed_include_mode_ast) ? std::move(stmt_ast) : nullptr;
	}

	return stmt_ast;
}


std::unique_ptr<CompoundStmt> ParserImpl::ParseCompoundStmt() {
	moveToNextToken(true);  // eat '{' and skip all subsequent new lines
	std::vector<std::unique_ptr<Node>> stmts;

	// this parses the statement bodies for selections, iterations && functionDefns
	while (true) {
		if (isCurrTokenValue('}')) break;

		auto stmt = ParseStmt();
		stmts.push_back(std::move(stmt));
	}

	moveToNextToken();  // eat }
	return std::make_unique<CompoundStmt>(std::move(stmts));
}


// simple buffer around moveToNextToken so we can hold its retain value as long as we want
Token ParserImpl::moveToNextToken(bool ignore_subsequent_newlines, bool increment_irgen_line_offset) {
	if (ignore_subsequent_newlines) {
		do {
			currentToken = lexer->scanNextToken(increment_irgen_line_offset);
		} while (currentToken == Token::NEW_LINE);

		return currentToken;
	}

	currentToken = lexer->scanNextToken(increment_irgen_line_offset);
	if ((currentToken == Token::NON_ALPHANUM_CHAR) and (lexer->NonAlphaNumchar == '\\')) {
		if (lexer->scanNextToken(increment_irgen_line_offset) != Token::NEW_LINE)
			LogError("Expected newline after \\");

		return moveToNextToken(true);
	}
}


Token ParserImpl::currToken() { return currentToken; }
