#include "ParserImpl.h"


std::unique_ptr<Identifier> ParserImpl::ParseIdentifier() {
    std::string Id = TokenValue::IdentifierStr;

    moveToNextToken();  // eat identifier token
    return std::make_unique<Identifier>(Id);

}


// parses type identifiers
std::unique_ptr<TypeId> ParserImpl::ParseTypeId() {
    std::string Id = TokenValue::IdentifierStr;
    if(Id == "liste") {
        moveToNextToken();
        if(not isCurrTokenValue('('))
            LogError("Expected '(' after liste keyword");
        moveToNextToken(); // eat (

        if(currentToken != Token::IDENTIFIER)
            LogError("Expected identifer describing the list type");
        
        auto lstty = ParseTypeId();

        if(not isCurrTokenValue(')'))
            LogError("Expected ')'");
        moveToNextToken(); // eat )

        return std::make_unique<ListTyId>(std::move(lstty));
    }

    if(std::find(PrimitiveTypes.begin(), PrimitiveTypes.end(), Id) != PrimitiveTypes.end()){
        moveToNextToken();
        return std::make_unique<PrimitiveTyId>(std::move(Id));
    }

    if(EntityTypes.count(Id) > 0)  {
        moveToNextToken();
        return std::make_unique<EntityTyId>(std::move(Id));
    }

    LogError("The type << " + Id + " >> doesn't exist !!");
}


std::unique_ptr<Node> ParserImpl::ParseIdOp() {
    auto id = ParseIdentifier(); // eats tok_identifer

    if(EntityTypes.count(id->name) != 0) { // Anonymous entity
        return ParseAnonymousEntity(std::move(id->name));
    }

    if(isCurrTokenValue('(')) { // function call. 
       return ParseFunctionCallExpr(std::move(id->name));
    } 

    // if identifier is not and entity type nor is followed by a "(", just return it
    return id;

}