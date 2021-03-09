#include "Names.h"
#include "ParserImpl.h"

// Error Logging
LLVM_ATTRIBUTE_NORETURN
void ParserImpl::LogError(std::string errMsg) {
	outs() << "Parse Error in " << names::getModuleFile().filename() << '\n'
	       << "[Line " << Attr::CurrentLexerLine << "]:  " << errMsg << '\n';

	exit(EXIT_FAILURE);
}


// checks if the current token value is a non alphanumeric character (that is not a kronk operator)
// corresponding to c
bool ParserImpl::isCurrTokenValue(char c) {
	return ((currentToken == Token::NON_ALPHANUM_CHAR) and (lexer->NonAlphaNumchar == c)) ? true : false;
}


// checks if the current token value is a kronk operator corresponding to op
bool ParserImpl::isCurrKronkOperator(const std::string& op) {
	return ((currentToken == Token::KRONK_OPERATOR) and (lexer->IdentifierStr == op)) ? true : false;
}


// Get the precedence of the pending binary operator token.
int ParserImpl::getOpPrec() {
	int prec = -1;
	if (currentToken == Token::KRONK_OPERATOR) {
		prec = Attr::KronkOperators.at(lexer->IdentifierStr);
	}

	return prec;
}


bool ParserImpl::currTokenIsAccessOp() { return isCurrTokenValue('.') or isCurrTokenValue('['); }


bool ParserImpl::isRightAssociativeOp(const std::string& Op) {
	return Attr::RightAssociativeOps.find(Op) != Attr::RightAssociativeOps.end();
}


bool ParserImpl::isRelationalOp(const std::string& Op) {
	return Attr::RelationalOps.find(Op) != Attr::RelationalOps.end();
}