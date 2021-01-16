#include "Nodes.h"
#include "irGenAide.h"

Value* Declr::codegen() {
    auto typeTy = type->typegen();
    AllocaInst* alloc;

    if(typeTy->isStructTy()) {
        auto enttypeTy = llvm::cast<StructType>(typeTy);
        alloc = builder.CreateAlloca(enttypeTy);

        if(enttypeTy->isLiteral()) {
            // We're declaring a liste with zero elements
            
            auto listSizeV = irGenAide::getConstantInt(0);
            auto dataPtr = builder.CreateAlloca(enttypeTy->getTypeAtIndex(1)->getPointerElementType(), listSizeV);
            irGenAide::fillUpListEntty(alloc, { listSizeV, dataPtr });
        }

        ScopeStack.back()->HeapAllocas.push_back(alloc);
    }

    else {
        // We're declaring either an i1 or double on the stack
        alloc = builder.CreateAlloca(typeTy);
    }

    ScopeStack.back()->SymbolTable[name] = alloc;
    return static_cast<Value*>(nullptr);
}


Value* InitDeclr::codegen() {
    auto rvalue = rhs->codegen();
   
    // An Expression node on emitting ir always returns one of two types: a value holding an i1, i64, or double;
    // or a pointer holding the address of some entity, since the default ctx for all expressions is Load
    if(auto rvalueTy = typeInfo::isEnttyPtr(rvalue)) { 
        // rvalue is a pointer to an entity (i.e it is an AllocaInst holding the entity's start address)
        // if it was just constructed, say be a list splice, anonymous list or entity constructor then it will be 
        // in the HeapAllocas record but not in the SymbolTable. We only need check the latter condition and if true
        // point name to it. If not then we have to copy rvalue
       
        auto it = std::find_if(
                                ScopeStack.back()->SymbolTable.begin(),
                                ScopeStack.back()->SymbolTable.end(),
                                [rvalue](const auto& symTblEntry) {return symTblEntry.second == rvalue;}
                                ); 
       
        if(it == ScopeStack.back()->SymbolTable.end()) {
            ScopeStack.back()->SymbolTable[name] = rvalue;
        }

        else {
            AllocaInst* alloc = builder.CreateAlloca(rvalueTy);
            irGenAide::copyEntty(alloc, rvalue);

            ScopeStack.back()->SymbolTable[name] = alloc;
            ScopeStack.back()->HeapAllocas.push_back(alloc);
        }        
    }

    else {
        // rvalue either holds the address of a primitive type, or is the result of some operation that returns
        /// a primitve type
        auto rvalueType = rvalue->getType();
        AllocaInst* alloc =  builder.CreateAlloca(rvalueType);
        ScopeStack.back()->SymbolTable[name] = alloc;
        // Now store rvalue in lvalue
        builder.CreateStore(rvalue, alloc);
    }

    return static_cast<Value*>(nullptr);
}