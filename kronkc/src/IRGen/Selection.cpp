#include "Nodes.h"
#include "irGenAide.h"


Value* IfStmt::codegen() {
     /**
     *                   |----> then block---->|   
     * condition---------|                     |---->cont block
     *                   |----> else block---->|
     *    
     * **/

    Value* CondV = Cond->codegen();

    Function* currentFunction = builder.GetInsertBlock()->getParent();
    // Create blocks for the then and else cases. Insert the 'then' block at the
    // end of the function.
    BasicBlock* ThenBB = BasicBlock::Create(context, "if.then", currentFunction);
    BasicBlock* ElseBB = BasicBlock::Create(context, "if.else");
    BasicBlock* MergeBB = BasicBlock::Create(context, "if.cont");
    
    ScopeStack.back()->fnExitBB = BasicBlock::Create(context, "fnExit");

    builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // emit then block
    builder.SetInsertPoint(ThenBB);
    ThenBody->codegen();
    
    if(not ThenBB->getTerminator()) {
        // do not emit a br Inst, if this block already has a terminator Inst.
        builder.CreateBr(MergeBB);
    } 

    // emit else block
    currentFunction->getBasicBlockList().push_back(ElseBB);
    builder.SetInsertPoint(ElseBB);

    if(ElseBody) {
        ElseBody->codegen();
    }
    
    if(not ElseBB->getTerminator()) {
        // do not emit a br Inst, if this block already has a terminator Inst.
        builder.CreateBr(MergeBB);
    } 
   
    // Emit merge block.
    currentFunction->getBasicBlockList().push_back(MergeBB);
    builder.SetInsertPoint(MergeBB);

    return static_cast<Value*>(nullptr); // need to change this so we can efficiently catch a codegen error
}