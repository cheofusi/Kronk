#include "ParserImpl.h"


std::unique_ptr<IncludeStmt> ParserImpl::ParseIncludeStmt() {
    moveToNextToken(); // eat include token

    std::string rtModule;
    // a vector is used because userModule could be a path   
    std::vector<std::string> userModule; 

    if(isCurrTokenValue(':')) {
        // we're including a std library module
        
        moveToNextToken(); // eat :
        if(currentToken != Token::IDENTIFIER)
            LogError("Expected identifier for standard library module to be included");
        
        auto id = ParseIdentifier();
        rtModule = id->name;
    }

    else {
        // we're including a user module
        
        if(currentToken != Token::IDENTIFIER)
            LogError("Expected identifier for module to be included");
        
        while (true) {
            auto id = ParseIdentifier();
            userModule.push_back(id->name);

            if(not isCurrTokenValue('.'))
                break;
            
            moveToNextToken(); // eat :
            if(currentToken != Token::IDENTIFIER)
                LogError("Expected identifier after the semi colon");
        }
    }
    
    // what follows is either an identifier for the alias keyword or a statement terminator.

    std::string alias; 
    if(currentToken == Token::IDENTIFIER) {
        auto alias_keyword = ParseIdentifier();
        if(alias_keyword->name != "as") 
            LogError("Expected alias keyword 'as' ");

        if(currentToken != Token::IDENTIFIER) 
            LogError("Expected an identifier aliasing the module << " + userModule.front() + " >>");

        auto id = ParseIdentifier();
        alias = id->name;
    }

    if(rtModule.empty()) {
        return std::make_unique<IncludeStmt>(std::move(userModule), std::move(alias));
    }
    
    return std::make_unique<IncludeStmt>(std::move(rtModule), std::move(alias)); 
}