#include "Nodes.h"
#include "irGenAide.h"



Value* BooleanLiteral::codegen() {
    if(value == "vrai") {
        return builder.getTrue();
    }

    return builder.getFalse();
}


Value* NumericLiteral::codegen() {
    return ConstantFP::get(builder.getDoubleTy(), APFloat(value));
}


Value* Identifier::codegen() {
    LogProgress("Accessing value of identifier: " + name);

    if(ScopeStack.back()->SymbolTable.count(name) == 0)
        irGenAide::LogCodeGenError("Unknown Identifier << " + name + " >>");
    
    Value* ptr = ScopeStack.back()->SymbolTable[name];

    if(typeInfo::isEnttyPtr(ptr)) {
        // this is the one exception to the ctx rule. it is so because the symbol table stores pointers to entities
        // different than entities containing entities do. So it doesn't make sense to 'load' from the symbol table
        return ptr;
    }
    return (ctx) ? builder.CreateLoad(ptr) : ptr; 
}

bool Identifier::injectCtx(bool ctx) {
    this->ctx = ctx;
    return true;
}