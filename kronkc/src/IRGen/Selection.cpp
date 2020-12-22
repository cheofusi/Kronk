#include "Nodes.h"
#include "irGenAide.h"


Value* IfNode::codegen() {
     /**
     *                   |----> then block---->|   
     * condition---------|                     |---->cont block
     *                   |----> else block---->|
     *    
     * **/

    Value *CondV = Cond->codegen();

    Function *currentFunction = builder.GetInsertBlock()->getParent();
    // Create blocks for the then and else cases. Insert the 'then' block at the
    // end of the function.
    BasicBlock *ThenBB = BasicBlock::Create(context, "if.then", currentFunction);
    BasicBlock *ElseBB = BasicBlock::Create(context, "if.else");
    BasicBlock *MergeBB = BasicBlock::Create(context, "if.cont");
  
    builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // emit then block
    builder.SetInsertPoint(ThenBB);
    for(auto& s : Then)
        s->codegen();
    builder.CreateBr(MergeBB);

    // emit else block
    currentFunction->getBasicBlockList().push_back(ElseBB);// We do this so this block doesn't enter the function before the nested blocks of ThenBB
    builder.SetInsertPoint(ElseBB);
    // else if
    if(ElseIf){
        ElseIf->codegen();
    }
    // just else
    else if(!LoneElse.empty()){
        for(auto& s : LoneElse)
            s->codegen();
    }
    builder.CreateBr(MergeBB);
    // Emit merge block.
    currentFunction->getBasicBlockList().push_back(MergeBB);
    builder.SetInsertPoint(MergeBB);

    return static_cast<Value*>(nullptr); // need to change this so we can efficiently catch a codegen error
}