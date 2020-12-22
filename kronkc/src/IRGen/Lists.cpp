#include "Nodes.h"
#include "irGenAide.h"


Value* AnonymousList::codegen() {
    LogProgress("Creating anonymous list");

    std::vector<Value*> initializerListV;
    Value* prevInitElementV;
    unsigned int idx = 0;
    for(auto& initElement : initializerList) {
        // first check if element is of the same type as the previous element. This is to ensure the validity 
        // of all elements before allocation memory and filling the symbol table.
        auto initElementV = initElement->codegen();
        if (idx >= 1) {
            if(not irGenAide::isEqual(initElementV->getType(), prevInitElementV->getType())) {
                return irGenAide::LogCodeGenError("Element number " + std::to_string(idx+1) + " in " + 
                                        "initializer list different from previous elements");
            }
        }
        initializerListV.push_back(std::move(initElementV));
        prevInitElementV = initElementV;
        idx++; 
    }

    auto sizeV = irGenAide::getConstantInt(initializerListV.size());
    auto listElementType = initializerListV[0]->getType();

    AllocaInst* dataPtr = builder.CreateAlloca(listElementType, sizeV); 
    AllocaInst* lstPtr = builder.CreateAlloca(StructType::get(builder.getInt64Ty(), dataPtr->getType()));
    
    irGenAide::fillUpListEntty(lstPtr, { sizeV, dataPtr }, initializerListV);

    ScopeStack.back()->HeapAllocas.push_back(lstPtr);

    return lstPtr;
}


Value* ListConcatenation::codegen() {
    // BinaryExpr already checked if list1 & list2 are both listPtrs. So we don't need to
    // now the magic begins
    auto Lsize = builder.CreateLoad(irGenAide::getGEPAt(list1, irGenAide::getConstantInt(0)));
    auto LdataPtr = builder.CreateLoad(irGenAide::getGEPAt(list1, irGenAide::getConstantInt(1)));
    auto Rsize = builder.CreateLoad(irGenAide::getGEPAt(list2, irGenAide::getConstantInt(0)));
    auto RdataPtr = builder.CreateLoad(irGenAide::getGEPAt(list2, irGenAide::getConstantInt(1)));

    // check whether the data types are equal
    if(not irGenAide::isEqual(LdataPtr->getType(), RdataPtr->getType()))
        return irGenAide::LogCodeGenError("Trying to concatenate lists with unequal types");
    
    auto newListSizeV = builder.CreateAdd(Lsize, Rsize);
    // The starting address of of the underlying memory block
    AllocaInst* newDataPtr = builder.CreateAlloca(LdataPtr->getType()->getPointerElementType(), newListSizeV);

    // we now get all data from both listes in order
    irGenAide::emitMemcpy(newDataPtr, LdataPtr, Lsize);
    irGenAide::emitMemcpy(irGenAide::getGEPAt(newDataPtr, Lsize, true), RdataPtr, Rsize);

    AllocaInst* newlistPtr = builder.CreateAlloca(StructType::get(builder.getInt64Ty(), newDataPtr->getType()));

    irGenAide::fillUpListEntty(newlistPtr, std::vector<Value*>{newListSizeV, newDataPtr});

    return newlistPtr;
}


Value* ListSlice::codegen() {
    auto lstPtr = list->codegen();
    auto listSizeV = builder.CreateLoad(irGenAide::getGEPAt(lstPtr, irGenAide::getConstantInt(0)));
    
    // first make sure start and end are valid indices given the list size 
    auto startV = start ? start->codegen() : irGenAide::getConstantInt(0);
    auto endV = end ? end->codegen() : listSizeV;
    
    if( !irGenAide::isEqual(startV->getType(), builder.getInt64Ty()) 
        or !irGenAide::isEqual(endV->getType(), builder.getInt64Ty()))
        return irGenAide::LogCodeGenError("index value into list must be an integer!!");
    
    // we insert a call for checking if the index is in the list bounds
    std::vector<Value*> args {startV, endV, listSizeV};
    auto Cond = builder.CreateCall(module->getFunction("_kronk_list_splice_check"), args);
    
    auto currentFunc = builder.GetInsertBlock()->getParent();
    BasicBlock* FalseBB = BasicBlock::Create(context, "splice.bad", currentFunc);
    BasicBlock* TrueBB = BasicBlock::Create(context, "splice.good", currentFunc);

    builder.CreateCondBr(Cond, TrueBB, FalseBB);

    // emit ir for bad splice
    builder.SetInsertPoint(FalseBB);
    auto errMsgV = irGenAide::buildRuntimeErrStr("SpliceError: while splicing liste");
    auto failure = builder.CreateCall(module->getFunction("_kronk_runtime_error"), std::vector<Value*>{errMsgV});
    builder.CreateUnreachable();

    // emit ir for good splice
    builder.SetInsertPoint(TrueBB);
    // taking care of negative idx's
    startV = builder.CreateCall(module->getFunction("_kronk_list_fix_idx"), std::vector<Value*>{startV, listSizeV});
    endV = builder.CreateCall(module->getFunction("_kronk_list_fix_idx"), std::vector<Value*>{endV, listSizeV});

    auto spliceSizeV = builder.CreateSub(endV, startV);
    auto oldDataPtr = builder.CreateLoad(irGenAide::getGEPAt(lstPtr, irGenAide::getConstantInt(1)));
    AllocaInst* newDataPtr = builder.CreateAlloca(oldDataPtr->getType()->getPointerElementType(), spliceSizeV);

    // transfer elements from original list to the splice
    irGenAide::emitMemcpy(newDataPtr, oldDataPtr, spliceSizeV);
    
    // build the list entity for the splice
    AllocaInst* newlstPtr = builder.CreateAlloca(StructType::get(builder.getInt64Ty(), newDataPtr->getType()));
    irGenAide::fillUpListEntty(newlstPtr, std::vector<Value*>{spliceSizeV, newDataPtr});

    return newlstPtr;
}


Value* ListOperation::codegen() {
    auto lstPtr = list->codegen();
    if(not irGenAide::isListePtr(lstPtr))
        return irGenAide::LogCodeGenError("Trying to perform a list operation on a value that is not a liste !!");
    auto listSizeV = builder.CreateLoad(irGenAide::getGEPAt(lstPtr, irGenAide::getConstantInt(0)));
    
    if(idx) {
        // list reference like lst[i]
        auto idxV = idx->codegen();
        if(not irGenAide::isEqual(idxV->getType(), builder.getInt64Ty()))
            return irGenAide::LogCodeGenError("index value into list must be an integer!!");
        std::vector<Value*> args {idxV, listSizeV};
        auto Cond = builder.CreateCall(module->getFunction("_kronk_list_index_check"), args);

        auto currentFunc = builder.GetInsertBlock()->getParent();
        BasicBlock* FalseBB = BasicBlock::Create(context, "index.bad", currentFunc);
        BasicBlock* TrueBB = BasicBlock::Create(context, "index.good", currentFunc);
    
        builder.CreateCondBr(Cond, TrueBB, FalseBB);
        
        // emit ir for bad index
        builder.SetInsertPoint(FalseBB);
        auto errMsgV = irGenAide::buildRuntimeErrStr("IndexError: while accessing liste");
        auto failure = builder.CreateCall(module->getFunction("_kronk_runtime_error"), std::vector<Value*>{errMsgV});
        builder.CreateUnreachable();

        // emit ir for good index, which is loading the list value at index
        builder.SetInsertPoint(TrueBB);
        // take care of negative indices
        idxV = builder.CreateCall(module->getFunction("_kronk_list_fix_idx"), std::vector<Value*>{idxV, listSizeV});
        auto listDataPtr = builder.CreateLoad(irGenAide::getGEPAt(lstPtr, irGenAide::getConstantInt(1)));

        auto dataPtr = irGenAide::getGEPAt(listDataPtr, idxV, true);

        return (ctx) ? builder.CreateLoad(dataPtr) : dataPtr;
    }
    
    return slice->codegen();
}