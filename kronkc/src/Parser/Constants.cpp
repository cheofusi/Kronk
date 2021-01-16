#include "ParserImpl.h"



std::unique_ptr<BooleanLiteral> ParserImpl::ParseBooleanLiteral() {
    auto boolVal = TokenValue::IdentifierStr;

    moveToNextToken(); // consume boolean literal
    return std::make_unique<BooleanLiteral>(std::move(boolVal));
}


std::unique_ptr<NumericLiteral> ParserImpl::ParseNumericLiteral() {
    auto numVal = TokenValue::NumericLiteral;

    moveToNextToken(); // consume the number
    return std::make_unique<NumericLiteral>(numVal);
}
