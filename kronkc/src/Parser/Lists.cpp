#include "ParserImpl.h"


std::unique_ptr<AnonymousList> ParserImpl::ParseAnonymousList() {
    // ex [1, 2, 3],  [4]
    moveToNextToken(); // eat [
    std::vector<std::unique_ptr<Node>> initList;
    if(isCurrTokenValue(']'))
        LogError("An anonymous list must contain at least one element");

    int i = 0; // This variable is just for better error reporting
    while (true) {
        auto initElement = ParseExpr();
        initList.push_back(std::move(initElement));
        if(isCurrTokenValue(']'))
            break;
        
        if(not isCurrTokenValue(','))
            LogError("Expected ',' after list element number << " + std::to_string(i) + " >>");
        moveToNextToken(); // eat ,
    }
    moveToNextToken(); // eat ]
    return std::make_unique<AnonymousList>(std::move(initList));     
    
}


 std::unique_ptr<AnonymousString> ParserImpl::ParseAnonymousString() {
    auto str = lexer->IdentifierStr;
    moveToNextToken();
    return std::make_unique<AnonymousString>(std::move(str));
 }


std::unique_ptr<Node> ParserImpl::ParseListOperation(std::unique_ptr<Node> list) {
    if(isCurrTokenValue(']'))
        LogError("How are you trying to access the list ??");

    //std::unique_ptr<Node> start = nullptr, end = nullptr;
    auto [idx, sliceStart, sliceEnd] = std::array<std::unique_ptr<Node>, 3>{nullptr, nullptr, nullptr}; 

    if(isCurrTokenValue(':')) {
        // ex lst[:i]
        moveToNextToken(); // eat :
        sliceEnd = ParseExpr();
        if(not isCurrTokenValue(']'))
            LogError("Expected ']' ");
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
                    LogError("Expected ']' ");
                // ex: lst[i:j]
                moveToNextToken(); // eat ] 
            }
        }

        else {
            LogError("Malformed expression");
        }
        
    }

    if(idx) {
        return std::make_unique<ListIdxRef>(std::move(list), std::move(idx));
    }

    return std::make_unique<ListSlice>(std::move(list), std::move(sliceStart), std::move(sliceEnd));
    
}