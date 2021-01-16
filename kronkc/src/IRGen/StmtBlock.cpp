#include "Nodes.h"


Value* CompoundStmt::codegen() {
    for(auto& stmt : stmtBlock) {
        if(dynamic_cast<ReturnStmt*>(stmt.get())) {
            // stop emitting ir for this block after a return statment
            stmt->codegen();
            break;
        }
        
        stmt->codegen();
    }

    return static_cast<Value*>(nullptr);
}