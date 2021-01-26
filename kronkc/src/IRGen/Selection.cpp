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

    Function* currentFunction = Attr::Builder.GetInsertBlock()->getParent();
    // Create blocks for the then and else cases. Insert the 'then' block at the
    // end of the function.
    BasicBlock* ThenBB = BasicBlock::Create(Attr::Context, "if.then", currentFunction);
    BasicBlock* ElseBB = BasicBlock::Create(Attr::Context, "if.else");
    BasicBlock* MergeBB = BasicBlock::Create(Attr::Context, "if.cont");
    
    Attr::ScopeStack.back()->fnExitBB = BasicBlock::Create(Attr::Context, "fnExit");

    Attr::Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // emit then block
    Attr::Builder.SetInsertPoint(ThenBB);
    ThenBody->codegen();
    
    if(not ThenBB->getTerminator()) {
        // do not emit a br Inst, if this block already has a terminator Inst.
        Attr::Builder.CreateBr(MergeBB);
    } 

    // emit else block
    currentFunction->getBasicBlockList().push_back(ElseBB);
    Attr::Builder.SetInsertPoint(ElseBB);

    if(ElseBody) {
        ElseBody->codegen();
    }
    
    if(not ElseBB->getTerminator()) {
        // do not emit a br Inst, if this block already has a terminator Inst.
        Attr::Builder.CreateBr(MergeBB);
    } 
   
    // Emit merge block.
    currentFunction->getBasicBlockList().push_back(MergeBB);
    Attr::Builder.SetInsertPoint(MergeBB);

    return static_cast<Value*>(nullptr); // need to change this so we can efficiently catch a codegen error
}