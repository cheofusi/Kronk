#include "Nodes.h"
#include "IRGenAide.h"


Value* IfStmt::codegen() {
     /**
     *                   |----> then block---->|   
     * condition---------|                     |---->cont block
     *                   |----> else block---->|
     *    
     * **/

    Value* CondV = Cond->codegen();

    auto currFunction = Attr::Builder.GetInsertBlock()->getParent();
    // Create blocks for the then and else cases. Insert the 'then' block at the
    // end of the function.
    auto ThenBB = BasicBlock::Create(Attr::Context, "if.then");
    auto ElseBB = BasicBlock::Create(Attr::Context, "if.else");
    auto MergeBB = BasicBlock::Create(Attr::Context, "if.cont");

    Attr::Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // emit then block

    currFunction->getBasicBlockList().push_back(ThenBB);
    Attr::Builder.SetInsertPoint(ThenBB);

    if(ThenBody) {
        ThenBody->codegen();
    }

    if(not Attr::Builder.GetInsertBlock()->getTerminator()) {
        // do not emit a br Inst, if this block already has a terminator Inst.
        Attr::Builder.CreateBr(MergeBB);
    } 

    // emit else block
    
    currFunction->getBasicBlockList().push_back(ElseBB);
    Attr::Builder.SetInsertPoint(ElseBB);

    if(ElseBody) {
        ElseBody->codegen();
    }
    
    if(not Attr::Builder.GetInsertBlock()->getTerminator()) {
        // do not emit a br Inst, if this block already has a terminator Inst.
        Attr::Builder.CreateBr(MergeBB);
    } 

    // Emit merge block.
    currFunction->getBasicBlockList().push_back(MergeBB);
    Attr::Builder.SetInsertPoint(MergeBB);

    return nullptr; 
}