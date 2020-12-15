#include "Parser.h"


const std::map<unsigned char, int> BinOpPrec = {
    std::make_pair('*', 50),
    std::make_pair('/', 50),
    std::make_pair('+', 40),
    std::make_pair('-', 40),
    std::make_pair('<', 30),
    std::make_pair('>', 30),
    std::make_pair('=', 10),
};


// simple buffer around moveToNextToken so we can hold its retain value as long as we want
Token moveToNextToken() {
    return currentToken = scanNextToken();
}


// Error Logging
std::nullptr_t LogError(std::string str) {
    std::cout << "ParserError [Line "<< currentLexerLine << "]:  " << str << std::endl;
    exit(EXIT_FAILURE);
    
    return nullptr;
}


// checks if the current token value is a non alphanumeric character and corresponds to c
bool isCurrTokenValue(char c) {
    return  ( (currentToken == Token::NON_ALPHA_NUM_CHAR) and (TokenValue::NonAlphaNumchar == c) ) ? true : false;
}


// Get the precedence of the pending binary operator token.
static int getOpPrec() {
    // Make sure it's a declared binop.
    if(BinOpPrec.find(TokenValue::NonAlphaNumchar) != BinOpPrec.end()) {
        return BinOpPrec.at(TokenValue::NonAlphaNumchar);
    }
    return -1;
}



/** Each of these functions returns an AST of one of class types Nodes.h. 
 *  
 *  General rule of thumb: If the parser can check the validity of something before the ir emitter, it should.Except
 *  it involves accessing the symbol table.
 * 
 **/


// handles type identifiers
std::unique_ptr<TypeId> ParseTypeId() {
    std::string Id = TokenValue::IdentifierStr;
    if(Id == "liste") {
        moveToNextToken();
        if(not isCurrTokenValue('('))
            return LogError("Expected '(' after 'liste' keyword");
        moveToNextToken(); // eat (

        if(currentToken != Token::IDENTIFIER)
            return LogError("Expected identifer describing the list type");
        
        auto lstty = ParseTypeId();

        if(not isCurrTokenValue(')'))
            return LogError("Expected ')'");
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

    return LogError("The type " + Id + " doesn't exist");
}




// These declarations are here so they are visible to other procedures that use them before their definition 
std::unique_ptr<Node> ParseExpr();
std::unique_ptr<Node> ParseEntityOperation(std::unique_ptr<Node>);


// handles integer literals
std::unique_ptr<IntegerNode> ParseIntLiteral(bool isNegative = false) {
    auto numericVal = (isNegative) ? (TokenValue::NumericLiteral * -1) : TokenValue::NumericLiteral;
    auto Result = std::make_unique<IntegerNode>(numericVal);
    moveToNextToken(); // consume the number
    return std::move(Result); // cast to r-value
}


// handles float literals
std::unique_ptr<FloatNode> ParseFloatLiteral(bool isNegative = false) {
    auto numericVal = (isNegative) ? (TokenValue::NumericLiteral * -1) : TokenValue::NumericLiteral;
    auto Result = std::make_unique<FloatNode>(numericVal);
    moveToNextToken(); // consume the number
    return std::move(Result); // cast to r-value
}


// handles standalone names
std::unique_ptr<Identifier> ParseIdentifier() {
    std::string Id = TokenValue::IdentifierStr;

    moveToNextToken();  // eat identifier token
    return std::make_unique<Identifier>(Id);

}


std::unique_ptr<Node> ParseDeclr() {
    moveToNextToken(); // eat soit keyword
    auto variableName = ParseIdentifier(); // parse variable name

    if (isCurrTokenValue(':')) {
        // simple declaration
        moveToNextToken(); // eat ':' 
        if (currentToken != Token::IDENTIFIER) return LogError("Expected a type identifier");
        auto variableType = ParseTypeId();
        
        return std::make_unique<Declr>(std::move(variableName->name), std::move(variableType));
    }

    if (isCurrTokenValue('=')) {
        // declaration + initialization ex: 
        // soit y = x + 8.2
        // soit lst = [1, 2, 3]
        moveToNextToken(); // eat equality sign
        auto rhs = ParseExpr();
        return std::make_unique<InitDeclr>(std::move(variableName->name), std::move(rhs));
    }

    // we got a nonalphanumric character that was neither ':' nor '='
    return LogError("Expected either ':' or '=' after start of declaration"); 

}


std::unique_ptr<AnonymousList> ParseAnonymousList() {
    // ex [1, 2, 3],  [4]
    moveToNextToken(); // eat [
    std::vector<std::unique_ptr<Node>> initList;
    if(isCurrTokenValue(']'))
        return LogError("An anonymous list must contain at least one element");

    int i = 0; // This variable is just for better error reporting
    while (true) {
        auto initElement = ParseExpr();
        initList.push_back(std::move(initElement));
        if(isCurrTokenValue(']'))
            break;
        
        if(not isCurrTokenValue(','))
            return LogError("Expected ',' after list element number " + std::to_string(i));
        moveToNextToken(); // eat ,
    }
    moveToNextToken(); // eat ]
    return std::make_unique<AnonymousList>(std::move(initList));     
    
}


std::unique_ptr<Node> ParseListOperation(std::unique_ptr<Node> list) {
    if(isCurrTokenValue(']'))
        return LogError("How are you trying to access the list ??");

    //std::unique_ptr<Node> start = nullptr, end = nullptr;
    auto [idx, sliceStart, sliceEnd] = std::array<std::unique_ptr<Node>, 3>{nullptr, nullptr, nullptr}; 

    if(isCurrTokenValue(':')) {
        // ex lst[:i]
        moveToNextToken(); // eat :
        sliceEnd = ParseExpr();
        if(not isCurrTokenValue(']'))
            return LogError("Expected ']'");
        moveToNextToken(); // eat ] 
    }

    else {
        sliceStart = ParseExpr();
    
        if(isCurrTokenValue(']')) {
            // ex lst[i]
            moveToNextToken(); // eat ]
            idx = std::move(sliceStart);
        }

        else if(isCurrTokenValue(':')) {
            moveToNextToken(); // eat :
            if(isCurrTokenValue(']')) {
                // ex: lst[i:]
                moveToNextToken(); // eat ] 
            }

            else {
                sliceEnd = ParseExpr();
                if(not isCurrTokenValue(']'))
                    return LogError("Expected ']'");
                // ex: lst[i:j]
                moveToNextToken(); // eat ] 
            }
        }

        else {
            return LogError("Malformed expression");
        }
        
    }

    auto listOp = (idx) ? std::make_unique<ListOperation>(std::move(list), std::move(idx))
                        : std::make_unique<ListOperation>(std::move(list), std::move(sliceStart), std::move(sliceEnd));
    
    // we then bottom up parse if the next token is '.' or '['. This takes care recursively of exprs 
    // like p[1][0] and p[1].x
    if(isCurrTokenValue('[')) {
        moveToNextToken(); // eat [
        return ParseListOperation(std::move(listOp));
    }

    if(isCurrTokenValue('.')) {
        moveToNextToken(); // eat .
        // niceee
        return ParseEntityOperation(std::move(listOp)); 
    }

    return listOp;
}


std::unique_ptr<AnonymousEntity> ParseAnonymousEntity(std::string entityType) {
    /// ex Point(x=1, y=0);

    if(not isCurrTokenValue('(')) return LogError("Expected '('");
    moveToNextToken(); // eat '('
    auto entitySignature = EntitySignatures[entityType]; // A list of entityType's fields

    // now we match the keyword args against the fields of the entityType.
    
    // Constructor for entityType. maps field positions to values
    std::unordered_map<unsigned int, std::unique_ptr<Node>> entityCons; 
    
    if(isCurrTokenValue(')')) {
        // default constructor
        moveToNextToken(); // eat ')'
        return std::make_unique<AnonymousEntity>(std::move(entityType), std::move(entityCons));
    }

    if(currentToken == Token::IDENTIFIER) {
        int i = 0; 
        while(i < entitySignature.size()){ // The user cannot pass more args than are fields in the entityType's signature
            auto field = ParseIdentifier();
            auto it = std::find(entitySignature.begin(), entitySignature.end(), field->name);

            if(it == entitySignature.end())
                return LogError("[ " + field->name + " ]" + " is not field of a " + entityType);
            // arg is a valid field. Now we parse value for this field
            if(not isCurrTokenValue('='))
                return LogError("Expected '=' after " + field->name);
            moveToNextToken(); // eat =
            auto fieldValue = ParseExpr();
            // now we update the constuctor
            auto fieldPos = it - entitySignature.begin();
            auto ret = entityCons.insert(std::make_pair(fieldPos, std::move(fieldValue)));
            if(ret.second == false) // Two initializations for the same field!!
                return LogError("[ " + field->name + " ]" + " already has a value");

            if(isCurrTokenValue(')'))
                break;
            else if(not isCurrTokenValue(','))
                return LogError("Expected ',' ");
            moveToNextToken(); // eat ','
            if(currentToken != Token::IDENTIFIER)
                return LogError("Expected a field after ',' ");

            i++;
            if(i == entitySignature.size()) // We're just past the # of entityType's fields and the constructor still goes on.
                return LogError("Definition is too long for entityType " + entityType);
            
        }
        moveToNextToken(); // eat ')'
        return std::make_unique<AnonymousEntity>(std::move(entityType), std::move(entityCons));
    }

    return LogError("Unexpected character after '('");
}


std::unique_ptr<Node> ParseEntityOperation(std::unique_ptr<Node> entity) {
    if(currentToken != Token::IDENTIFIER)
        return LogError("Expected alphanumeric string for a field");
    // the selected field is always an identifier
    auto entityField = ParseIdentifier();
    auto enttyOp = std::make_unique<EntityOperation>(std::move(entity), std::move(entityField->name));
    
    // we then bottom up parse if the next token is '.' or '['. This takes care recursively of exprs 
    // like s.p.x and s.p[1]
    if(isCurrTokenValue('.')) {
        moveToNextToken(); // eat .
        return ParseEntityOperation(std::move(enttyOp));
    }

    if(isCurrTokenValue('[')) {
        moveToNextToken(); // eat [
        // niceee
        return ParseListOperation(std::move(enttyOp));
    }

    return enttyOp;
}


std::unique_ptr<Node> ParseIdOp() {
    auto id = ParseIdentifier(); // eats tok_identifer

    if(EntityTypes.count(id->name) != 0) { // Anonymous entity
        return ParseAnonymousEntity(std::move(id->name));
    }

    if(isCurrTokenValue('(')) { // function call. 
        moveToNextToken(); // eat '('
        if(isCurrTokenValue(')')) { // no param function call
            moveToNextToken(); // eat ')'
            return std::make_unique<FunctionCallNode>(std::move(id));
        }
        std::vector<std::unique_ptr<Node>> Args; // args can be constants, identifiers or even functionCalls
        while (true) { 
            auto arg = ParseExpr(); // goes until ","
            Args.push_back(std::move(arg));
            if(isCurrTokenValue(')'))
                break;
            moveToNextToken(); // eat ",". 
        }
        
        moveToNextToken(); // eat ")". We don't eat ; hear cause we can have a function call without ; ex in the condition of an ifExpr.
        return std::make_unique<FunctionCallNode>(std::move(id), std::move(Args));
    }

    if(isCurrTokenValue('[')) { // list acesss
        moveToNextToken(); // eat [
        return ParseListOperation(std::move(id));
    }   
    
    if(isCurrTokenValue('.')) { // Entity operation ex. point.y;
        moveToNextToken(); // eat .
        return ParseEntityOperation(std::move(id));
    }

    // if identifier is not followed by "(", "[" or ".", return it
    return id;

}


std::unique_ptr<Node> ParseParenthesizedExpr() {
    moveToNextToken(); // eat (.
    auto V = ParseExpr();

    if (not isCurrTokenValue(')'))
        return LogError("expected ')'");
    moveToNextToken(); // eat ).
    return V;
}


std::unique_ptr<Node> ParsePrimaryExpr() {
    switch (currentToken) {
        case Token::IDENTIFIER:
            return ParseIdOp();
        
        case Token::INT_LITERAL:
            return ParseIntLiteral();
        
        case Token::FLOAT_LITERAL:
            return ParseFloatLiteral();
        
        case Token::NON_ALPHA_NUM_CHAR:
            switch (TokenValue::NonAlphaNumchar) {
                case '-':
                    moveToNextToken(); // eat -
                    if(currentToken == Token::IDENTIFIER){ // negative identifier
                        auto LHS = std::make_unique<IntegerNode>(0);
                        auto RHS = ParseIdentifier();  
                        return std::make_unique<BinaryExpr>('-', std::move(LHS), std::move(RHS));
                    }
                    // negative integer or float
                    if(currentToken == Token::INT_LITERAL)
                        return ParseIntLiteral(true);
                    if(currentToken == Token::FLOAT_LITERAL)
                        return ParseFloatLiteral(true);
                    return LogError("Invalid token after negative sign");
                
                case '(':
                    return ParseParenthesizedExpr();

                case '[':
                    return ParseAnonymousList();
            }

        default:
            return LogError("Malformed Expression");
    }
}


// binoprhs ::= (<binaryOperator> <expression>)*
// This is one of the most beautiful things I've ever seen.
std::unique_ptr<Node> ParseBinOpRHS(int ExprPrec, std::unique_ptr<Node> LHS) {
    // ExprPrec is the minimum operator precedence that this function is allowed to eat

    // This loop handles the * (zero or more) operator or kleene star
    while (true) {
        int TokPrec = getOpPrec();
        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done. This also handles unrecognized operators i.e return values of -1 by GetTokPrecedence
        if (TokPrec < ExprPrec)
            return LHS;
        // Okay, we know this is a binop.
        int binaryOperator = TokenValue::NonAlphaNumchar;
        moveToNextToken(); // eat binop

        // Parse the primary expression after the binary operator.
        auto RHS = ParsePrimaryExpr();

        // If binaryOperator binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = getOpPrec();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
        }

        // Merge LHS/RHS and call it LHS as we'll loop back and check if * regex operator is exhausted
        LHS = std::make_unique<BinaryExpr>(static_cast<char>(binaryOperator), std::move(LHS), std::move(RHS));
    }

}


// expression ::= <primaryExpr> | <primaryExpr> <binaryOperator> <expression> 
std::unique_ptr<Node> ParseExpr() {
    auto LHS = ParsePrimaryExpr();

    return ParseBinOpRHS(0, std::move(LHS));
}


// handles return statements
std::unique_ptr<ReturnExpr> ParseReturnStmt() {
    moveToNextToken(); // eat tok_return
    auto E = ParseExpr(); // value to return 
    return std::make_unique<ReturnExpr>(std::move(E));
}


std::unique_ptr<IfNode> ParseIfExpr() {
    moveToNextToken(); // eat the if
    auto Cond = ParseExpr();
    if(currentToken != Token::IDENTIFIER and TokenValue::IdentifierStr == "alors")
        return LogError("Expected 'alors' ");
    moveToNextToken(); // eat then
    if(not isCurrTokenValue('{')) return LogError("Expected a {");
    moveToNextToken(); // eat {

    std::vector<std::unique_ptr<Node>> ThenBody, ElseBody;
    while (true) {
        if(isCurrTokenValue('}'))
            break;
        
        auto stmt = ParseStmt(); 
        ThenBody.push_back(std::move(stmt));
    }

    moveToNextToken(); // eat }
    
    // There may or may not be an else part
    if(currentToken == Token::ELSE_STMT) {
        moveToNextToken(); // eat else
        // case else if
        if(currentToken == Token::IF_STMT) {
            auto subIf = ParseIfExpr();
            return std::make_unique<IfNode>(std::move(Cond), std::move(ThenBody), std::move(subIf));
        }
        // case lone else
        else {
            moveToNextToken(); // eat {
            while (true) {
                if(isCurrTokenValue('}'))
                    break;

                auto stmt = ParseStmt();
                ElseBody.push_back(std::move(stmt));
            }

            moveToNextToken(); // eat }
            return std::make_unique<IfNode>(std::move(Cond), std::move(ThenBody), std::move(ElseBody));
        }
    }
    
    // No else part
    return std::make_unique<IfNode>(std::move(Cond), std::move(ThenBody));
}


std::unique_ptr<WhileNode> ParseWhileLoop() {
    moveToNextToken(); // eat Tantque
    auto cond = ParseExpr();
    if(not isCurrTokenValue('{'))
        return LogError("Expected { after loop declaration");
    moveToNextToken(); // eat {
    
    std::vector<std::unique_ptr<Node>> whileBody;
    while (true) {
        if(isCurrTokenValue('}'))
            break;

        auto stmt = ParseStmt();
        whileBody.push_back(std::move(stmt));
    }

    moveToNextToken(); // eat }
    return std::make_unique<WhileNode>(std::move(cond), std::move(whileBody));
}


std::unique_ptr<EntityDefn> ParseEntityDefn() {
    moveToNextToken(); // eat tok_entityDefn
    auto entityTypeName = ParseIdentifier();
    if(EntityTypes.count(entityTypeName->name) != 0) // Entity Type already exists!!
        return LogError("Entity type " + entityTypeName->name + " already exists");

    if(not isCurrTokenValue('{'))
        return LogError("Expected '{' to follow " + entityTypeName->name);
    moveToNextToken(); // eat '{'
    if(isCurrTokenValue('}'))
        return LogError("kronk cannot create an entity with zero fields");

    std::vector<std::string> memberNames;    
    std::vector<std::unique_ptr<TypeId>> memberTypeIds;
    while (true) {   
        if(currentToken != Token::IDENTIFIER)
            return LogError("Expected alphanumeric string for a field");
        auto fieldName = ParseIdentifier();

        if(not isCurrTokenValue(':'))
            return LogError("Expected ':' after " + fieldName->name);
        moveToNextToken(); // eat :

        if(currentToken != Token::IDENTIFIER)    
            return LogError("Expected alpahnumeric string after ':' ");
        auto fieldTypeId = ParseTypeId();
        // now check if fieldType is a primitive type or existing entityType

        if(not isCurrTokenValue(';'))
            return LogError("Expected ';'");
        moveToNextToken(); // eat ;

        memberNames.push_back(std::move(fieldName->name));
        memberTypeIds.push_back(std::move(fieldTypeId));

        if(isCurrTokenValue('}'))
            break;
    }

    EntitySignatures[entityTypeName->name] = memberNames;
    moveToNextToken(); // eat {
    return std::make_unique<EntityDefn>(std::move(entityTypeName->name), std::move(memberTypeIds));
}


std::unique_ptr<Prototype> ParseFunctionPrototype() {
    auto funcReturnType = ParseIdentifier();
    auto funcName = ParseIdentifier();
    if(not isCurrTokenValue('(')) return LogError("Expected ( ");
    moveToNextToken(); // eat (

    std::vector<std::unique_ptr<Identifier>> argTypes;
    std::vector<std::unique_ptr<Identifier>> argNames;

    while(true) {
        auto argType = ParseIdentifier();
        auto argName = ParseIdentifier();
        argTypes.push_back(std::move(argType)); argNames.push_back(std::move(argName));

        if(isCurrTokenValue(')')) 
            break;
        moveToNextToken(); // eat ,
    }

    moveToNextToken(); // eat )
    assert(argNames.size() == argTypes.size());
    return std::make_unique<Prototype>(std::move(funcReturnType), std::move(funcName), std::move(argTypes), std::move(argNames));

}


std::unique_ptr<FunctionDefn> ParseFunctionDefn() {
    moveToNextToken();  // eat tok_fonction
    auto Proto = ParseFunctionPrototype();
    if (!Proto) return nullptr;
    if(not isCurrTokenValue('{'))
        return LogError("Expected {");
    moveToNextToken(); // eat {

    std::vector<std::unique_ptr<Node>> Body;
    while (true) {   
        if(isCurrTokenValue('}'))
            break;

        auto stmt = ParseStmt();
        Body.push_back(std::move(stmt));
    }

    moveToNextToken(); // eat {
    return std::make_unique<FunctionDefn>(std::move(Proto), std::move(Body));
}


std::unique_ptr<Node> ParseStmt() {
    std::unique_ptr<Node> stmt_ast;
    switch (currentToken) {
        case Token::FUNCTION_DEFN:
            stmt_ast = ParseFunctionDefn();
            LogProgress("Read Function Definition");
            return stmt_ast;
        
        case Token::ENTITY_DEFN:
            stmt_ast = ParseEntityDefn();
            LogProgress("Read Entity Definition");
            return stmt_ast;

        case Token::DECLR_STMT:
            stmt_ast = ParseDeclr();
            LogProgress("Read Declaration");
            if(not isCurrTokenValue(';')) 
                LogError("Expected ;");
            moveToNextToken();
            return stmt_ast;

        case Token::IF_STMT:
            stmt_ast = ParseIfExpr();
            return stmt_ast;

        case Token::WHILE_STMT:
            stmt_ast = ParseWhileLoop();
            return stmt_ast;

        case Token::RETURN_STMT:
            stmt_ast = ParseReturnStmt();
            if(not isCurrTokenValue(';')) 
                LogError("Expected ;");
            moveToNextToken();
            return stmt_ast;

        default:
            stmt_ast = ParseExpr();
            LogProgress("Read an Expression");
            if(not isCurrTokenValue(';')) 
                LogError("Expected ;");
            moveToNextToken(); // eat ;, the expression terminator.
            return stmt_ast;
    }
}