#include "Nodes.h"
#include "irGenAide.h"

Value* Declr::codegen() {
    auto typeTy = type->typegen();
    AllocaInst* alloc;

    if(typeTy->isStructTy()) {
        auto enttypeTy = llvm::cast<StructType>(typeTy);
        alloc = Attr::Builder.CreateAlloca(enttypeTy);

        if(enttypeTy->isLiteral()) {
            // We're declaring a liste with zero elements
            
            auto listSizeV = irGenAide::getConstantInt(0);
            auto dataPtr = Attr::Builder.CreateAlloca(enttypeTy->getTypeAtIndex(1)->getPointerElementType(), listSizeV);
            irGenAide::fillUpListEntty(alloc, { listSizeV, dataPtr });
        }

        Attr::ScopeStack.back()->HeapAllocas.push_back(alloc);
    }

    else {
        // We're declaring either an i1 or double on the stack
        alloc = Attr::Builder.CreateAlloca(typeTy);
    }

    Attr::ScopeStack.back()->SymbolTable[name] = alloc;
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
                                Attr::ScopeStack.back()->SymbolTable.begin(),
                                Attr::ScopeStack.back()->SymbolTable.end(),
                                [rvalue](const auto& symTblEntry) {return symTblEntry.second == rvalue;}
                                ); 
       
        if(it == Attr::ScopeStack.back()->SymbolTable.end()) {
            Attr::ScopeStack.back()->SymbolTable[name] = rvalue;
        }

        else {
            AllocaInst* alloc = Attr::Builder.CreateAlloca(rvalueTy);
            irGenAide::copyEntty(alloc, rvalue);

            Attr::ScopeStack.back()->SymbolTable[name] = alloc;
            Attr::ScopeStack.back()->HeapAllocas.push_back(alloc);
        }        
    }

    else {
        // rvalue either holds the address of a primitive type, or is the result of some operation that returns
        /// a primitve type
        auto rvalueType = rvalue->getType();
        AllocaInst* alloc =  Attr::Builder.CreateAlloca(rvalueType);
        Attr::ScopeStack.back()->SymbolTable[name] = alloc;
        // Now store rvalue in lvalue
        Attr::Builder.CreateStore(rvalue, alloc);
    }

    return static_cast<Value*>(nullptr);
}