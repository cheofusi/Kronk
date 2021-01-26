#include "Nodes.h"
#include "irGenAide.h"


Value* WhileStmt::codegen() {
    /**
     *                   |----> loop exit block   
     * condition block --|
     *      |            |----> loop body block-->|
     *      |<------------------------------------|
     * **/

    Function* currentFunction = Attr::Builder.GetInsertBlock()->getParent();
    std::vector<Value*> BodyV; // holds codegen values for loop body
    
    BasicBlock* CondBB = BasicBlock::Create(Attr::Context, "Condition", currentFunction);
    BasicBlock* LoopBB = BasicBlock::Create(Attr::Context, "LoopBody");
    BasicBlock* ExitBB = BasicBlock::Create(Attr::Context, "exitBody");
    // emit loop condition
    Attr::Builder.CreateBr(CondBB);
    Attr::Builder.SetInsertPoint(CondBB);
    Value* CondV = Cond->codegen();
    
    Attr::Builder.CreateCondBr(CondV, LoopBB, ExitBB);

    // emit loop body
    currentFunction->getBasicBlockList().push_back(LoopBB);
    Attr::Builder.SetInsertPoint(LoopBB);

    Body->codegen();

    Attr::Builder.CreateBr(CondBB);

    // emit loop exit block
    currentFunction->getBasicBlockList().push_back(ExitBB);
    Attr::Builder.SetInsertPoint(ExitBB); // The next block of instructions (which we don't know yet) will insert an unconditional branch here to itself.

    return static_cast<Value*>(nullptr);
}
