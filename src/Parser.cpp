#include "Parser.h"

// simple buffer around moveToNextToken so we can hold its retain value as long as we want
int moveToNextToken() {
    return currentToken = scanNextToken();
}

// Helper functions

// GetTokPrecedence - Get the precedence of the pending binary operator token.
int GetTokenPrecedence() {
    if (!isascii(currentToken))
        return -1;

    // Make sure it's a declared binop.
    int tokenPrecedence = BinaryOperatorPrecedence[currentToken];
    if (tokenPrecedence <= 0) return -1;
        return tokenPrecedence;
}

// Error Logging
auto LogError(const char *str) {
    std::cout << "LogError: " << str << std::endl;
    exit(EXIT_FAILURE);
    return nullptr;
}



/** Now the parser functions which actually build the final AST by recursively rebuilding sub ASTs.
 *  Each function returns an AST of one of class types node.h
 * 
 * <Statement> ::= <VariableDeclaration> '=' <expression> ';'  |  <Identifier> '=' <expression> ';'
 * <VariableDeclaration> ::= <type> <identifer> 
 * <expression> ::= <expression> <binaryOperator> <expression>
 *              ::= <bareInteger> | <bareFloat> | <Identifier>
 *              ::= <functionCall>
 * 
 * <binaryOperator> ::= '+' | '-' | '*' | '/'
 * <bareInteger> ::= [0-9]+
 * <bareFloat> ::= [0-9][0-9.]+
 * 
 * 
 **/


/* Called for bare integer */
std::unique_ptr<IntegerNode> ParseBareInteger(bool isNegative = false) {
    auto numericVal = (isNegative) ? (numericValue * -1) : numericValue;
    auto Result = std::make_unique<IntegerNode>(numericVal);
    moveToNextToken(); // consume the number
    return std::move(Result); // cast to r-value
}


/* Called for bare double */
std::unique_ptr<FloatNode> ParseBareFloat(bool isNegative = false) {
    auto numericVal = (isNegative) ? (numericValue * -1) : numericValue;
    auto Result = std::make_unique<FloatNode>(numericVal);
    moveToNextToken(); // consume the number
    return std::move(Result); // cast to r-value
}


/* Called for when an identifer ex a variable reference is seen*/
std::unique_ptr<Identifier> ParseIdentifier() {
    std::string IdName = IdentifierStr;
    moveToNextToken();  // eat identifier token
    return std::make_unique<Identifier>(IdName);

}

// Called when return is seen
std::unique_ptr<ReturnExpr> ParseReturnStmt() {
    moveToNextToken(); // eat tok_return
    auto E = ParseExpression(); // value to return 
    return std::make_unique<ReturnExpr>(std::move(E));
}

std::unique_ptr<ArrayOperation> ParseArrayLoadOrStore(std::unique_ptr<Identifier> arrName){
    auto index = ParseExpression(); // index must not only be identifier, can be bare int or double or even a function call
    if(currentToken == '='){ // store operation i.e tab @ i = 4;
        moveToNextToken(); // eat =
        auto rvalue = ParseExpression();
        return std::make_unique<ArrayOperation>(std::move(arrName), std::move(index), std::move(rvalue));
    }

    return std::make_unique<ArrayOperation>(std::move(arrName), std::move(index));
}


/* Called when '(' is seen
 i.e ::= '(' expression* ')'
*/
std::unique_ptr<Node> ParseParenthesizedExpr() {
    moveToNextToken(); // eat (.
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (currentToken != ')')
        return LogError("expected ')'");
    moveToNextToken(); // eat ).
    return V;
}

/* Called when token_INT OR token_DOUBLE is seen */
std::unique_ptr<Node> ParseVariableDeclaration(){
    auto variableType = ParseIdentifier(); //first parse type stored in i
    auto variableName = ParseIdentifier(); // then parse variable name
    if(currentToken == ';') // variable declaration e.g int x;
        return std::make_unique<VariableDeclaration>(std::move(variableType), std::move(variableName));
    
    if(currentToken == '='){ // variable declaration + initialization e.g int x = 0;
        moveToNextToken(); // eat equality sign
        auto rhs = ParseExpression();// goes until it meets unknown operator
        return std::make_unique<VariableDeclaration>(std::move(variableType), std::move(variableName), std::move(rhs));
    }

    if(currentToken == '('){ // array declaration
        moveToNextToken(); // eat (
        if(currentToken == '['){ // list initializer for array ex entier arr([1, 3, 4]);
            std::vector<std::unique_ptr<Node>> initList;
            moveToNextToken(); // eat [
            if(currentToken == ']')
                return LogError("Initializer list must contain at least one element");
            while (true){
                if(auto initElement = ParseExpression())
                    initList.push_back(std::move(initElement));
                if(currentToken == ']')
                    break;
                moveToNextToken(); // eat ,
            }
            moveToNextToken(); // eat ]
            moveToNextToken(); // eat )
            return std::make_unique<ArrayDeclaration>(std::move(variableType), std::move(variableName), initList.size(), std::move(initList));
            
        }

        else{ // simple array declaration ex entier arr(10); The size must be an integer. So just read numericValue and cast.
            if(currentToken != tok_bareInt)
                return LogError("Array size must be an integer");
            moveToNextToken(); // eat tok_bareInt
            moveToNextToken(); // eat )
            return std::make_unique<ArrayDeclaration>(std::move(variableType), std::move(variableName), static_cast<uint64_t>(numericValue));
        }
    }

    return LogError("Expected ';' after declaration"); 

}


// primary ::= identifierexpr | numberexpr | parenexpr
std::unique_ptr<Node> ParsePrimary() {
    switch (currentToken) {
    default:
        return LogError("unknown token when expecting an expression");
    case tok_identifier:
        return ParseIdentifierOrCall();
    case tok_bareInt:
        return ParseBareInteger();
    case tok_bareDouble:
        return ParseBareFloat();
    case '-':
        moveToNextToken(); // eat -
        if(currentToken == tok_identifier){ // negative identifier
            auto LHS = std::make_unique<IntegerNode>(0);
            auto RHS = ParseIdentifier();  
            return std::make_unique<BinaryExpr>('-', std::move(LHS), std::move(RHS));
        }
        // negative integer or float
        if(currentToken == tok_bareInt)
            return ParseBareInteger(true);
        if(currentToken == tok_bareDouble)
            return ParseBareFloat(true);
        return LogError("unkown token after negative sign");
    case '(':
        return ParseParenthesizedExpr();
    case tok_if:
        return ParseIfExpr();
    case tok_return:
        return ParseReturnStmt();
    case tok_whileLoop:
        return ParseWhileLoop();
    }
}


// binoprhs ::= (<binaryOperator> <expression>)*
// This is one of the most beautiful things I've ever seen.
std::unique_ptr<Node> ParseBinOpRHS(int ExprPrec, std::unique_ptr<Node> LHS) {
    // ExprPrec is the minimum operator precedence that this function is allowed to eat

    // This loop handles the * (zero or more) operator
    while (true) {
        int TokPrec = GetTokenPrecedence();
        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done. This also handles unrecognized operators i.e return values of -1 by GetTokPrecedence
        if (TokPrec < ExprPrec)
            return LHS;
        // Okay, we know this is a binop.
        int binaryOperator = currentToken;
        moveToNextToken(); // eat binop

        // Parse the primary expression after the binary operator.
        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        // If binaryOperator binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokenPrecedence();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        // Merge LHS/RHS and call it LHS as we'll loop back and check if * regex operator is exhausted
        LHS = std::make_unique<BinaryExpr>(static_cast<char>(binaryOperator), std::move(LHS), std::move(RHS));
    }

}


// expression ::= <primaryExpr> | <primaryExpr> <binaryOperator> <expression> 
std::unique_ptr<Node> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

/// <IdentifierOrFunctionCall> ::= <identifier> // A statement can simply be an identifier.
///                           ::= <identifer> '='  <expression> //An assigment
///                           ::= <expression> //an expression can be a function call
std::unique_ptr<Node> ParseIdentifierOrCall(){
    auto id = ParseIdentifier(); // eats tok_identifer

    if(currentToken == '='){ // assigment statement
        moveToNextToken(); // eat equality sign
        auto rhs  = ParseExpression();
        if(currentToken != ';') return LogError("Expected ';' after assigment statement");
        // We don't eat ';' here because we can have an assigment without ; ex in the condition of an ifExpr
        return std::make_unique<Assignment>(std::move(id), std::move(rhs));
    }

    if(currentToken == '('){ // function call
        moveToNextToken(); // eat '('
        std::vector<std::unique_ptr<Node>> Args; // args can be constants, identifiers or even functionCalls
        while (true){
            auto arg = ParseExpression(); // goes until ","
            Args.push_back(std::move(arg));
            if(currentToken == ')')
                break;
            moveToNextToken(); // eat ",". 
        }
        
        moveToNextToken(); // eat ")". We don't eat ; hear cause we can have a function call without ; ex in the condition of an ifExpr.
        if(Args.empty()) 
            return std::make_unique<FunctionCallNode>(std::move(id));
        return std::make_unique<FunctionCallNode>(std::move(id), std::move(Args));
    }

    if(currentToken == '@'){ // array operation ex. tab @ i = 4;
        moveToNextToken(); // eat @
        return ParseArrayLoadOrStore(std::move(id));
    }

    // if identifier is not followed by a "=" or a "(", then return it
    return id;

}

// called when currentToken = tok_if
std::unique_ptr<IfNode> ParseIfExpr(){
    moveToNextToken(); // eat the if
    auto Cond = ParseExpression();
    if(currentToken != tok_alors) return LogError("Expected a then");
    moveToNextToken(); // eat then
    if(currentToken != '{') return LogError("Expected a {");
    moveToNextToken(); // eat {

    std::vector<std::unique_ptr<Node>> ThenBody, ElseBody;
    while (true){
        if(currentToken == '}')
            break;
        if(currentToken == tok_INT || currentToken == tok_DOUBLE){
            auto statement = ParseVariableDeclaration();
            ThenBody.push_back(std::move(statement));
        }
        // could be id, function call, assigment, nested if else etc.
        else{
            auto Ex = ParseExpression(); // check error.
            ThenBody.push_back(std::move(Ex));
        }
        moveToNextToken(); // eat ;
    }
    moveToNextToken(); // eat }
    
    // There may or may not be an else part
    if(currentToken == tok_else){
        moveToNextToken(); // eat else
        // case else if
        if(currentToken == tok_if){
            auto subIf = ParseExpression();
            return std::make_unique<IfNode>(std::move(Cond), std::move(ThenBody), std::move(subIf));
        }
        // case lone else
        else {
            moveToNextToken(); // eat {
            while (true){
                if(currentToken == '}')
                    break;
                if(currentToken == tok_INT || currentToken == tok_DOUBLE){
                    auto statement = ParseVariableDeclaration();
                    ElseBody.push_back(std::move(statement));
                }
                else{
                    auto Ex = ParseExpression(); // Subroutines of ParseExpression will report errors
                    ElseBody.push_back(std::move(Ex));
                }
                if(currentToken == ';') // There can be no semi colons after an expression ex after a conditional statement
                    moveToNextToken();
            }

            moveToNextToken(); // eat }
            return std::make_unique<IfNode>(std::move(Cond), std::move(ThenBody), std::move(ElseBody));
        }
    }
    
    else { // No else part
        return std::make_unique<IfNode>(std::move(Cond), std::move(ThenBody));
    }
}



std::unique_ptr<WhileNode> ParseWhileLoop(){
    moveToNextToken(); // eat Tantque
    auto cond = ParseExpression();
    if(currentToken != '{')
        return LogError("Expected { after loop declaration");
    moveToNextToken(); // eat {
    
    std::vector<std::unique_ptr<Node>> body;
    while (true){
        if(currentToken == '}')
            break;
        if(currentToken == tok_INT || currentToken == tok_DOUBLE){
            auto statement = ParseVariableDeclaration();
            body.push_back(std::move(statement));
        }
        // could be id, function call, assigment, nested if else, nested while loop etc.
        else{
            auto Ex = ParseExpression(); // check error.
            body.push_back(std::move(Ex));
        }
        if(currentToken == ';') // There can be no semi colons after an expression ex after a conditional statement
            moveToNextToken(); // eat ;
    }
    moveToNextToken(); // eat }
    return std::make_unique<WhileNode>(std::move(cond), std::move(body));
}


std::unique_ptr<Prototype> ParseFunctionPrototype(){
    auto funcReturnType = ParseIdentifier();
    auto funcName = ParseIdentifier();
    if(currentToken != '(') return LogError("Expected ( ");
    moveToNextToken(); // eat (
    std::vector<std::unique_ptr<Identifier>> argTypes;
    std::vector<std::unique_ptr<Identifier>> argNames;
    while(true){
        auto argType = ParseIdentifier();
        auto argName = ParseIdentifier();
        argTypes.push_back(std::move(argType)); argNames.push_back(std::move(argName));
        if(currentToken == ')') break;
        moveToNextToken(); // eat ,
    }
    moveToNextToken(); // eat )
    assert(argNames.size() == argTypes.size());
    return std::make_unique<Prototype>(std::move(funcReturnType), std::move(funcName), std::move(argTypes), std::move(argNames));

}


/// definition ::= 'fonction' <prototype> <expression*>
//  No multiple return statements for now. 
std::unique_ptr<FunctionDefinition> ParseFunctionDefinition() {
    moveToNextToken();  // eat tok_fonction
    auto Proto = ParseFunctionPrototype();
    if (!Proto) return nullptr;
    if(currentToken != '{') return LogError("Expected {");
    moveToNextToken(); // eat {
    std::vector<std::unique_ptr<Node>> Body;
    while (true)
    {   
        if(currentToken == '}')
            break;
        if(currentToken == tok_INT || currentToken == tok_DOUBLE){
            auto statement = ParseVariableDeclaration();
            Body.push_back(std::move(statement));
        }
        else{
            auto Ex = ParseExpression(); // check error.
            Body.push_back(std::move(Ex));
        }
        if(currentToken == ';') // There can be no semi colons after an expression ex after a conditional statement
            moveToNextToken(); // eat ;
    }
    moveToNextToken(); // eat {
    return std::make_unique<FunctionDefinition>(std::move(Proto), std::move(Body));
}