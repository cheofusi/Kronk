#include "ParserImpl.h"



std::unique_ptr<Parser> Parser::CreateParser(std::string& inputFile) {
    return std::make_unique<ParserImpl>(inputFile);
}


ParserImpl::ParserImpl(std::string& inputFile) {
    lexer = std::make_unique<Lexer>(inputFile);
}


std::unique_ptr<Node> ParserImpl::ParseStmt() {
    std::unique_ptr<Node> stmt_ast;
    switch (currentToken) {
        case Token::NEW_LINE:
            moveToNextToken(true);
            return ParseStmt();

        case Token::FUNCTION_DEFN:
            stmt_ast = ParseFunctionDefn();
            LogProgress("Read Function Definition");
            break;
        
        case Token::ENTITY_DEFN:
            stmt_ast = ParseEntityDefn();
            LogProgress("Read Entity Definition");
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
   
    if((currentToken != Token::NEW_LINE) and (not isCurrTokenValue(';'))) {
        if(currentToken == Token::END_OF_FILE)
            return stmt_ast;
        LogError("A statment must end with a new line or semi-colon !!");
    }

    moveToNextToken(true);
    return stmt_ast;
}


std::unique_ptr<CompoundStmt> ParserImpl::ParseCompoundStmt() {
    moveToNextToken(true); // eat '{' and skip all subsequent new lines
    std::vector<std::unique_ptr<Node>> stmts;

    // this parses the statement bodies for selections, iterations && functionDefns
    while (true) {
        if(isCurrTokenValue('}'))
            break;
        
        auto stmt = ParseStmt();
        stmts.push_back(std::move(stmt));
    }

    moveToNextToken(); // eat }
    return std::make_unique<CompoundStmt>(std::move(stmts));
}


// simple buffer around moveToNextToken so we can hold its retain value as long as we want
Token ParserImpl::moveToNextToken(bool ignore_subsequent_newlines) {
    if(ignore_subsequent_newlines) {
        do {
            currentToken = lexer->scanNextToken();
        } while (currentToken == Token::NEW_LINE);

        return currentToken;
    }
    
    currentToken = lexer->scanNextToken();
    if( (currentToken == Token::NON_ALPHANUM_CHAR) and (lexer->NonAlphaNumchar == '\\') ) {
        if(lexer->scanNextToken() != Token::NEW_LINE)
            LogError("Expected newline after \\"); 

        return moveToNextToken(true);
    }
}


Token ParserImpl::currToken() {
    return currentToken;
}
