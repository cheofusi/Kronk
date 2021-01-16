#include "ParserImpl.h"


std::unique_ptr<IfStmt> ParserImpl::ParseIfStmt() {
    moveToNextToken(); // eat the if
    auto Cond = ParseExpr();
    if(currentToken != Token::IDENTIFIER and TokenValue::IdentifierStr == "alors")
        LogError("Expected 'alors' ");
    moveToNextToken(); // eat then
    if(not isCurrTokenValue('{')) 
        LogError("Expected '{' ");

    auto ThenBody = ParseCompoundStmt();
    if( (currentToken == Token::NEW_LINE) or (isCurrTokenValue(';'))) {
        // if a terminator follows }, eat it and skip all subsequent new lines
        moveToNextToken(true);
    }

    // There may or may not be an else stmt
    if(currentToken == Token::ELSE_STMT) {
        moveToNextToken(); // eat else
        // case else if
        if(currentToken == Token::IF_STMT) {
            auto subIf = ParseIfStmt();
            return std::make_unique<IfStmt>(std::move(Cond), std::move(ThenBody), std::move(subIf));
        }
        // case lone else
        else {            
            auto ElseBody = ParseCompoundStmt();
            return std::make_unique<IfStmt>(std::move(Cond), std::move(ThenBody), std::move(ElseBody));
        }
    }
    
    // lone if
    return std::make_unique<IfStmt>(std::move(Cond), std::move(ThenBody));
}