#include "ParserImpl.h"
#include "Names.h"



std::unique_ptr<Identifier> ParserImpl::ParseIdentifier() {
    std::string Id = std::move(lexer->IdentifierStr);

    moveToNextToken();  // eat identifier token
    return std::make_unique<Identifier>(std::move(Id));

}


std::unique_ptr<Identifier> ParserImpl::ParseForeignSymbol() {
    // ex in the expression ` km::Point `, the caller of this method discovers there is a ':' after 'km'.
    // And since the caller admits foreign symbols in whatever context it is parsing, this method is called
    // to pass the rest i.e '::Point'.

    moveToNextToken(); // eat first :
    if(not isCurrTokenValue(':'))
        LogError("Expected two semi colons, got one"); 
        
    moveToNextToken(); // eat second :
    if(currentToken != Token::IDENTIFIER)
        LogError("Expected an identifier referring to a function or entity type");
    
    auto symbolId = ParseIdentifier();
    return std::move(symbolId); 
}


// parses type identifiers
std::unique_ptr<TypeId> ParserImpl::ParseTypeId() {
    auto id = ParseIdentifier();

    if(id->name == "liste") {
        if(not isCurrTokenValue('('))
            LogError("Expected '(' after liste keyword");
        moveToNextToken(); // eat (

        if(currentToken != Token::IDENTIFIER)
            LogError("Expected identifer for the list type");
        
        auto lstty = ParseTypeId();

        if(not isCurrTokenValue(')'))
            LogError("Expected ')'");
        moveToNextToken(); // eat )

        return std::make_unique<ListTyId>(std::move(lstty));
    }

    if(Attr::BuiltinTypes.find(id->name) != Attr::BuiltinTypes.end()) {
        return std::make_unique<BuiltinTyId>(std::move(id->name));
    }

    // this part is tricky. consider the statement ` soit lst: liste(km::Point) `. In this case id=km at this point
    // when it rather refers to a module. so we have to look after it.

    if(isCurrTokenValue(':')) {
        // foreign symbol
        auto id2 = ParseForeignSymbol();
        auto nameAsForeignType = names::mangleNameAsForeignType(id->name, id2->name);
        if(names::EntityType(nameAsForeignType)) {
            return std::make_unique<EntityTyId>(std::move(nameAsForeignType));
        }

        LogError("<< " + id2->name 
                + " >> is not an entity type in the module << " 
                + names::demangleNameForErrMsg(nameAsForeignType).first 
                + " >>");
    }

    auto nameAsLocalType = names::mangleNameAsLocalType(id->name);

    if(names::EntityType(nameAsLocalType)) {
        return std::make_unique<EntityTyId>(std::move(nameAsLocalType));
    }

    LogError("The type << " 
            + id->name 
            + " >> doesn't exist in the module << " 
            + names::demangleNameForErrMsg(nameAsLocalType).first);
}


std::unique_ptr<Node> ParserImpl::ParseIdOp() {
    auto id = ParseIdentifier(); // eats tok_identifer

    if(isCurrTokenValue(':')) { 
        // foreign symbool    
        
        auto id2 = ParseForeignSymbol();
        
        // so we have something like 'id::id2' (actually id->name :: id2->name). And we don't know if id2 
        // is a type or a function in the foreign module. So we try parsing it as type and if it fails,
        // try parsing it as a function.

        // first check if id->name is a dep of the currrent module
        auto it = std::find(Attr::Dependencies.begin(), Attr::Dependencies.end(), id->name);
        if(it == Attr::Dependencies.end()) {
            LogError("<< " 
                    + id->name 
                    + " does not identify any included module ");
        }    
        
        auto nameAsForeignType = names::mangleNameAsForeignType(id->name, id2->name);
        if(names::EntityType(nameAsForeignType)) {
            // Anonymous foreign entity type
            return ParseAnonymousEntity(std::move(nameAsForeignType));
        }

        // foreign function call.
        auto nameAsForeignFunction = names::mangleNameAsForeignFunction(id->name, id2->name);
        if(names::Function(nameAsForeignFunction)) {
            return ParseFunctionCallExpr(std::move(nameAsForeignFunction));
        }

        LogError("<< " + id2->name 
                + " >> is neither an entity type nor a function in the module << " 
                + names::demangleNameForErrMsg(nameAsForeignType).first 
                + " >>");
    }


    else if(isCurrTokenValue('(')) {
        // id->name supposedly refers to a local symbol that's either an anonymous entity constructor 
        // or function call. Since local Entity types and function names are also mangled, we mangle
        // it with the Attr::ThisModule 's name in both cases. We try parsing it as a local type and
        // on failure leave it to irgen to check if it is a local function

        auto nameAsLocalType = names::mangleNameAsLocalType(id->name);
        if(names::EntityType(nameAsLocalType)) { 
            // Anonymous entity
            return ParseAnonymousEntity(std::move(nameAsLocalType));
        }

        auto nameAsLocalFunction = names::mangleNameAsLocalFunction(id->name); 
        return ParseFunctionCallExpr(std::move(nameAsLocalFunction));
    } 

    // if identifier is not followed by a "(", just return it
    return id;

}