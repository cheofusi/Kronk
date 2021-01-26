#ifndef _IRGen_H
#define _IRGen_H

#include "Attributes.h"

class Scope;
extern std::vector<std::unique_ptr<Scope>> ScopeStack; 


class Scope {
    public:
        // symbol table for variables of this scope
        std::unordered_map<std::string, Value*> SymbolTable; 
        
        // record of all heap allocations for this scope
        std::vector<Value*> HeapAllocas;

        // last block in a function definition.
        BasicBlock* fnExitBB;
        // holds return value for function. Helps to handle multiple return values in function definition
        Value* returnValue = nullptr; 
        
};


class Node {
    
    public:
        // ctx is 1 for load (default context), 0 for store. This has nothing to do with llvm's context
        bool ctx = 1; 

        // Some Node classes have no context e.x Declr, FunctionDefn, CompoundStmt. While some can only allow a load
        // context e.x ListSlice, FunctionCallExpr.
        // Node classes that can have a load or store ctx (i.e Nodes allowed on the left hand side of an assigment) 
        // override this method.
        virtual bool injectCtx(bool ctx) {
            return false;
        }

        virtual ~Node() {}
        virtual Value *codegen() = 0;
};


#endif