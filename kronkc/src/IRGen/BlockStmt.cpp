#include "Nodes.h"


Value* CompoundStmt::codegen() {
    for(auto& stmt : block_stmt) {
        if(dynamic_cast<ReturnStmt*>(stmt.get())) {
            // stop emitting ir for this block after a return statment
            stmt->codegen();
            break;
        }
        
        stmt->codegen();
    }

    return nullptr;
}