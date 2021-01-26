#include "ParserImpl.h"



std::unique_ptr<Node> ParserImpl::ParseExpr() {
    // expression ::= <primaryExpr> | <primaryExpr> <binaryOperator> <expression> 
    auto LHS = ParsePrimaryExpr();

    return ParseBinOpRHS(0, std::move(LHS));
}


std::unique_ptr<Node> ParserImpl::ParseBinOpRHS(int ExprPrec, std::unique_ptr<Node> LHS) {
    // binoprhs ::= (<binaryOperator> <expression>)*
    // This is one of the most beautiful things I've ever seen.
    
    // TODO: find aN example showing why its a bad idea to recursively call 
    // ParseBinOpRHS(TokPrec, std::move(RHS)) instead of ParseBinOpRHS(TokPrec + 1, std::move(RHS))

    // ExprPrec is the minimum operator precedence that this function is allowed to eat

    // This loop handles the * (zero or more) operator or kleene star
    while (true) {

        // list or entity access. I didn't want to handle this during codegen as a binop.
        if(isCurrTokenValue('.') or isCurrTokenValue('[')) {
            LHS = ParseAccessOp(std::move(LHS));
        }

        int TokPrec = getOpPrec();
        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done. This also handles unrecognized operators 
        // i.e return values of -1 by GetTokPrecedence
        if (TokPrec < ExprPrec)
            return LHS;
        // Okay, we know this is a binop.
        std::string binaryOperator = lexer->IdentifierStr;
        moveToNextToken(); // eat binop

        // Parse the primary expression after the binary operator.
        auto RHS = ParsePrimaryExpr();

        // list or entity access. I didn't want to handle this during codegen as a binop.
        if(isCurrTokenValue('.') or isCurrTokenValue('[')) {
            RHS = ParseAccessOp(std::move(RHS));    
        }

        // If binaryOperator binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = getOpPrec();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
        }

        // Merge LHS/RHS and call it LHS as we'll loop back and check if * regex operator is exhausted
        LHS = std::make_unique<BinaryExpr>(std::move(binaryOperator), std::move(LHS), std::move(RHS));
    }

}


std::unique_ptr<Node> ParserImpl::ParseUnaryOp() {
    auto op = lexer->IdentifierStr;
    if(op == "-") {
        // a hyphen at the start of an expression negates the terminal it precedes
        moveToNextToken(); // eat -
        return std::make_unique<BinaryExpr>("-", std::make_unique<NumericLiteral>(0), std::move(ParsePrimaryExpr()));
    }

    LogError("Unkown unary operator << " + op + " >>");
}


std::unique_ptr<Node> ParserImpl::ParseAccessOp(std::unique_ptr<Node> listOrEntity) {
    std::unique_ptr<Node> listOrEntityOp;

    if(isCurrTokenValue('[')) { 
        // listOp e.x lst[1]
        moveToNextToken(); // [
        listOrEntityOp = ParseListOperation(std::move(listOrEntity));
    }

    else {
        // entityOp like p.x
        moveToNextToken(); // eat '.'
        listOrEntityOp = ParseEntityOperation(std::move(listOrEntity));
    }

    // recurse if the next token is '.' or '['. This takes care of exprs 
    // like s.p.x and s.p[1]
    if(isCurrTokenValue('.') or isCurrTokenValue('[')) {
        return ParseAccessOp(std::move(listOrEntityOp));
    }

    return listOrEntityOp;
}


std::unique_ptr<Node> ParserImpl::ParsePrimaryExpr() {
    switch (currentToken) {
        case Token::IDENTIFIER:
            return ParseIdOp();
        
        case Token::BOOLEAN_LITERAL:
            return ParseBooleanLiteral();

        case Token::NUMERIC_LITERAL:
            return ParseNumericLiteral();
        
        case Token::STRING_LITERAL:
            return ParseAnonymousString();
        
        case Token::KRONK_OPERATOR:
            return ParseUnaryOp();

        case Token::NON_ALPHANUM_CHAR:
            switch (lexer->NonAlphaNumchar) {
                case '(':
                    return ParseParenthesizedExpr();

                case '[':
                    return ParseAnonymousList();
            }

        default:
            LogError("Malformed Expression");
    }
}


std::unique_ptr<Node> ParserImpl::ParseParenthesizedExpr() {

    moveToNextToken(); // eat (.
    auto V = ParseExpr();

    if (not isCurrTokenValue(')'))
        LogError("expected ')' ");
    moveToNextToken(); // eat ).
    return V;
}


