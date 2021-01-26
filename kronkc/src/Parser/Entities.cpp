#include "ParserImpl.h"


std::unique_ptr<EntityDefn> ParserImpl::ParseEntityDefn() {
    moveToNextToken(); // eat tok_entityDefn
    if(currentToken != Token::IDENTIFIER)
        LogError("Expected identifier string for the name of the entity type");

    auto entityTypeName = ParseIdentifier();
    if(Attr::EntityTypes.count(entityTypeName->name) != 0) // Entity Type already exists!!
        LogError("Entity type << " + entityTypeName->name + " >> already exists");

    if(not isCurrTokenValue('{'))
        LogError("Expected '{' to follow << " + entityTypeName->name + " >>");

    moveToNextToken(true); // eat '{' and skip all subsequent new lines

    if(isCurrTokenValue('}'))
        LogError("kronk cannot create an entity with zero fields");

    std::vector<std::string> memberNames;    
    std::vector<std::unique_ptr<TypeId>> memberTypeIds;
    while (true) {   
        if(currentToken != Token::IDENTIFIER)
            LogError("Expected an identifer string for a field");
        auto fieldName = ParseIdentifier();

        if(not isCurrTokenValue(':'))
            LogError("Expected ':' after << " + fieldName->name + " >>");
        moveToNextToken(); // eat :

        if(currentToken != Token::IDENTIFIER)    
            LogError("Expected an identifier string for a type ");
        auto fieldTypeId = ParseTypeId();

        // the declarations in an entity type definition are actually pseudo-statements
        if((currentToken != Token::NEW_LINE) and (not isCurrTokenValue(';'))) {
            LogError("A statment must end with a new line or semi-colon !!");
        }

        moveToNextToken(true); // eat terminator

        memberNames.push_back(std::move(fieldName->name));
        memberTypeIds.push_back(std::move(fieldTypeId));

        if(isCurrTokenValue('}'))
            break;
    }

    Attr::EntitySignatures[entityTypeName->name] = memberNames;
    moveToNextToken();
    return std::make_unique<EntityDefn>(std::move(entityTypeName->name), std::move(memberTypeIds));
}


std::unique_ptr<AnonymousEntity> ParserImpl::ParseAnonymousEntity(std::string entityType) {
    /// ex Point(x=1, y=0);

    if(not isCurrTokenValue('(')) LogError("Expected '('");
    moveToNextToken(); // eat '('
    auto entitySignature = Attr::EntitySignatures[entityType]; // A list of entityType's fields

    // now we match the keyword args against the fields of the entityType.
    
    // Constructor for entityType. maps field positions to values
    std::unordered_map<uint8_t, std::unique_ptr<Node>> entityCons; 
    
    if(isCurrTokenValue(')')) {
        // default constructor
        moveToNextToken(); // eat ')'
        return std::make_unique<AnonymousEntity>(std::move(entityType), std::move(entityCons));
    }

    if(currentToken == Token::IDENTIFIER) {
        int i = 0; 
        // The user cannot pass more args than are fields in the entityType's signature
        while(i < entitySignature.size()) { 
            auto field = ParseIdentifier();
            auto it = std::find(entitySignature.begin(), entitySignature.end(), field->name);

            if(it == entitySignature.end())
                LogError("<< " + field->name + " >>" + " is not a valid field of << " + entityType + " >>");

            // arg is a valid field. Now we parse value for this field
            if(not isCurrKronkOperator("="))
                LogError("Expected '=' after << " + field->name + " >>");
            moveToNextToken(); // eat =
            
            auto fieldValue = ParseExpr();
            // now we update the constuctor
            auto fieldPos = it - entitySignature.begin();
            auto ret = entityCons.insert(std::make_pair(fieldPos, std::move(fieldValue)));
            if(ret.second == false) // Two initializations for the same field!!
                LogError("<< " + field->name + " >>" + " already has a value");

            if(isCurrTokenValue(')'))
                break;

            else if(not isCurrTokenValue(','))
                LogError("Expected ',' ");

            moveToNextToken(); // eat ','
            if(currentToken != Token::IDENTIFIER)
                LogError("Expected a field after ',' ");

            i++;

            // We're just past the # of entityType's fields and the constructor still goes on.
            if(i == entitySignature.size()) 
                LogError("Definition is too long for entityType << " + entityType + " >>");
            
        }

        moveToNextToken(); // eat ')'
        return std::make_unique<AnonymousEntity>(std::move(entityType), std::move(entityCons));
    }

    LogError("Unexpected character after '('");
}


std::unique_ptr<EntityOperation> ParserImpl::ParseEntityOperation(std::unique_ptr<Node> entity) {
    if(currentToken != Token::IDENTIFIER)
        LogError("Expected alphanumeric string for a field");

    // the selected field is always an identifier
    auto entityField = ParseIdentifier();
    return std::make_unique<EntityOperation>(std::move(entity), std::move(entityField->name));
}