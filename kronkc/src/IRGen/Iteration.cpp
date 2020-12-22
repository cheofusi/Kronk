#include "Nodes.h"
#include "irGenAide.h"


Value* WhileNode::codegen() {
    /**
     *                   |----> loop exit block   
     * condition block --|
     *      |            |----> loop body block-->|
     *      |<------------------------------------|
     * **/
    Function *currentFunction = builder.GetInsertBlock()->getParent();
    std::vector<Value*> BodyV; // holds codegen values for loop body
    
    BasicBlock *CondBB = BasicBlock::Create(context, "Condition", currentFunction);
    BasicBlock *LoopBB = BasicBlock::Create(context, "LoopBody");
    BasicBlock *ExitBB = BasicBlock::Create(context, "exitBody");
    // emit loop condition
    builder.CreateBr(CondBB);
    builder.SetInsertPoint(CondBB);
    Value *CondV = Cond->codegen();
    
    builder.CreateCondBr(CondV, LoopBB, ExitBB);

    // emit loop body
    currentFunction->getBasicBlockList().push_back(LoopBB);
    builder.SetInsertPoint(LoopBB);
    for(auto& s : Body)
        s->codegen();

    builder.CreateBr(CondBB);

    // emit loop exit block
    currentFunction->getBasicBlockList().push_back(ExitBB);
    builder.SetInsertPoint(ExitBB); // The next block of instructions (which we don't know yet) will insert an unconditional branch here to itself.

    return static_cast<Value*>(nullptr);
}
