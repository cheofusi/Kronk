#include "Nodes.h"
#include "iRGenHelpers.h"


SMDiagnostic error;
// module hold all kronk rutime functions
std::unique_ptr<Module> runtimeLibM = getLazyIRFileModule("bin/libkronkrt.bc", error, context);

// Each scope is a shared pointer so load and store operations can acess these scopes without owning the scope
std::vector<std::unique_ptr<Scope>> ScopeStack; 

// primitive types 
std::vector<std::string> PrimitiveTypes = {"entier", "reel"}; 
// maps entity types(user defined types) to their signature names
std::unordered_map<std::string, std::vector<std::string>> EntitySignatures;
// maps entity types to their signature types.
std::unordered_map<std::string, StructType*> EntityTypes; 

extern void LogProgress(std::string str);

llvm::LLVMContext context;
std::shared_ptr<llvm::Module> module = std::make_shared<llvm::Module>("top", context);
IRBuilder<> builder(context);




/**********************************************************************************************************************/
Type* PrimitiveTyId::typegen() {
    if (prityId == "entier") {
        return builder.getInt64Ty();
    }
    else if (prityId == "reel") {
        return builder.getDoubleTy();
    }

    // This will never happen as the parser cannot miss this.
    return iRGenHelpers::LogCodeGenError("Primitive Type not recognized");
}


Type* EntityTyId::typegen() {
    return EntityTypes[enttyId];
}


Type* ListTyId::typegen() {
    auto lstElemTy = lsttyId->typegen();
    // The first element of a liste entity is the its size. The second is a ptr to its element type

    if(lstElemTy->isStructTy()) {
        // this is because lists of entities hold pointers to those entities, the entities themselves
        // ex. liste(liste(entier)) -> { i64, { i64, i64* }** }, liste(Point) -> { i64, Point** }
        return StructType::get(context, { builder.getInt64Ty(), lstElemTy->getPointerTo()->getPointerTo() });
    }
    // ex. liste(entier) -> { i64, i64* }
    return StructType::get(context, { builder.getInt64Ty(), lstElemTy->getPointerTo() });
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




Value* IntegerNode::codegen() {
    return iRGenHelpers::getLLVMConstantInt(value);
}


Value* FloatNode::codegen() {
    return ConstantFP::get(builder.getDoubleTy(), APFloat(value));
}


Value* Identifier::codegen() {
    LogProgress("Accessing value of identifier: " + name);
    if(ScopeStack.back()->SymbolTable.count(name) == 0)
        return iRGenHelpers::LogCodeGenError("Unknown Identifier " + name);
    
    Value* ptr = ScopeStack.back()->SymbolTable[name];

    if(iRGenHelpers::isEnttyPtr(ptr)) {
        // this is the one exception to the ctx rule. it is so because the symbol table stores pointers to entities
        // different than entities containing entities do. So it doesn't make sense to 'load' from the symbol table
        return ptr;
    }
    return (ctx) ? builder.CreateLoad(ptr) : ptr; 
}


Value* Declr::codegen() {
    auto typeTy = type->typegen();
    AllocaInst* alloc;

    if(typeTy->isStructTy()) {
        auto enttypeTy = llvm::cast<StructType>(typeTy);
        alloc = builder.CreateAlloca(enttypeTy);

        if(enttypeTy->isLiteral()) {
            // We're declaring a liste with zero elements
            
            auto listSizeV = iRGenHelpers::getLLVMConstantInt(0);
            auto dataPtr = builder.CreateAlloca(enttypeTy->getTypeAtIndex(1)->getPointerElementType(), listSizeV);
            iRGenHelpers::fillUpListEntty(alloc, { listSizeV, dataPtr });

        }

        ScopeStack.back()->HeapAllocas.push_back(alloc);
    }

    else {
        // We're declaring either an i1, i64 or double on the stack
        alloc = builder.CreateAlloca(typeTy);
    }

    ScopeStack.back()->SymbolTable[name] = alloc;
    return static_cast<Value*>(nullptr);
}


Value* InitDeclr::codegen() {
    auto rvalue = rhs->codegen();
   
    // An Expression node on emitting ir always returns one of two types: a value holding an i1, i64, or double;
    // or a pointer holding the address of some entity, since the default ctx for all expressions is Load
    if(auto rvalueTy = iRGenHelpers::isEnttyPtr(rvalue)) { 
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
            iRGenHelpers::copyEntty(alloc, rvalue);

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
            if(not iRGenHelpers::isEqual(initElementV->getType(), prevInitElementV->getType())) {
                return iRGenHelpers::LogCodeGenError("Element number " + std::to_string(idx+1) + " in " + 
                                        "initializer list different from previous elements");
            }
        }
        initializerListV.push_back(std::move(initElementV));
        prevInitElementV = initElementV;
        idx++; 
    }

    auto sizeV = iRGenHelpers::getLLVMConstantInt(initializerListV.size());
    auto listElementType = initializerListV[0]->getType();

    AllocaInst* dataPtr = builder.CreateAlloca(listElementType, sizeV); 
    AllocaInst* lstPtr = builder.CreateAlloca(StructType::get(builder.getInt64Ty(), dataPtr->getType()));
    
    iRGenHelpers::fillUpListEntty(lstPtr, { sizeV, dataPtr }, initializerListV);

    ScopeStack.back()->HeapAllocas.push_back(lstPtr);

    return lstPtr;
}


Value* ListSlice::codegen() {
    auto lstPtr = list->codegen();
    auto listSizeV = builder.CreateLoad(iRGenHelpers::getGEPAt(lstPtr, iRGenHelpers::getLLVMConstantInt(0)));
    
    // first make sure start and end are valid indices given the list size 
    auto startV = start ? start->codegen() : iRGenHelpers::getLLVMConstantInt(0);
    auto endV = end ? end->codegen() : listSizeV;
    
    if( !iRGenHelpers::isEqual(startV->getType(), builder.getInt64Ty()) 
        or !iRGenHelpers::isEqual(endV->getType(), builder.getInt64Ty()))
        return iRGenHelpers::LogCodeGenError("index value into list must be an integer!!");
    
    // we insert a call for checking if the index is in the list bounds
    std::vector<Value*> args {startV, endV, listSizeV};
    auto Cond = builder.CreateCall(module->getFunction("_kronk_list_splice_check"), args);
    
    auto currentFunc = builder.GetInsertBlock()->getParent();
    BasicBlock* FalseBB = BasicBlock::Create(context, "splice.bad", currentFunc);
    BasicBlock* TrueBB = BasicBlock::Create(context, "splice.good", currentFunc);

    builder.CreateCondBr(Cond, TrueBB, FalseBB);

    // emit ir for bad splice
    builder.SetInsertPoint(FalseBB);
    auto errMsgV = iRGenHelpers::buildRuntimeErrStr("SpliceError: while splicing liste");
    auto failure = builder.CreateCall(module->getFunction("_kronk_runtime_error"), std::vector<Value*>{errMsgV});
    builder.CreateUnreachable();

    // emit ir for good splice
    builder.SetInsertPoint(TrueBB);
    // taking care of negative idx's
    startV = builder.CreateCall(module->getFunction("_kronk_list_fix_idx"), std::vector<Value*>{startV, listSizeV});
    endV = builder.CreateCall(module->getFunction("_kronk_list_fix_idx"), std::vector<Value*>{endV, listSizeV});

    auto spliceSizeV = builder.CreateSub(endV, startV);
    auto oldDataPtr = builder.CreateLoad(iRGenHelpers::getGEPAt(lstPtr, iRGenHelpers::getLLVMConstantInt(1)));
    AllocaInst* newDataPtr = builder.CreateAlloca(oldDataPtr->getType()->getPointerElementType(), spliceSizeV);

    // transfer elements from original list to the splice
    iRGenHelpers::emitMemcpy(newDataPtr, oldDataPtr, spliceSizeV);
    
    // build the list entity for the splice
    AllocaInst* newlstPtr = builder.CreateAlloca(StructType::get(builder.getInt64Ty(), newDataPtr->getType()));
    iRGenHelpers::fillUpListEntty(newlstPtr, std::vector<Value*>{spliceSizeV, newDataPtr});

    return newlstPtr;
}


Value* ListOperation::codegen() {
    auto lstPtr = list->codegen();
    if(not iRGenHelpers::isListePtr(lstPtr))
        return iRGenHelpers::LogCodeGenError("Trying to perform a list operation on a value that is not a liste !!");
    auto listSizeV = builder.CreateLoad(iRGenHelpers::getGEPAt(lstPtr, iRGenHelpers::getLLVMConstantInt(0)));
    
    if(idx) {
        // list reference like lst[i]
        auto idxV = idx->codegen();
        if(not iRGenHelpers::isEqual(idxV->getType(), builder.getInt64Ty()))
            return iRGenHelpers::LogCodeGenError("index value into list must be an integer!!");
        std::vector<Value*> args {idxV, listSizeV};
        auto Cond = builder.CreateCall(module->getFunction("_kronk_list_index_check"), args);

        auto currentFunc = builder.GetInsertBlock()->getParent();
        BasicBlock* FalseBB = BasicBlock::Create(context, "index.bad", currentFunc);
        BasicBlock* TrueBB = BasicBlock::Create(context, "index.good", currentFunc);
    
        builder.CreateCondBr(Cond, TrueBB, FalseBB);
        
        // emit ir for bad index
        builder.SetInsertPoint(FalseBB);
        auto errMsgV = iRGenHelpers::buildRuntimeErrStr("IndexError: while accessing liste");
        auto failure = builder.CreateCall(module->getFunction("_kronk_runtime_error"), std::vector<Value*>{errMsgV});
        builder.CreateUnreachable();

        // emit ir for good index, which is loading the list value at index
        builder.SetInsertPoint(TrueBB);
        // take care of negative indices
        idxV = builder.CreateCall(module->getFunction("_kronk_list_fix_idx"), std::vector<Value*>{idxV, listSizeV});
        auto listDataPtr = builder.CreateLoad(iRGenHelpers::getGEPAt(lstPtr, iRGenHelpers::getLLVMConstantInt(1)));

        auto dataPtr = iRGenHelpers::getGEPAt(listDataPtr, idxV, true);

        return (ctx) ? builder.CreateLoad(dataPtr) : dataPtr;
    }
    
    return slice->codegen();
}


Value* AnonymousEntity::codegen() {
    // We first allocate space for the entity
    AllocaInst* enttyPtr = builder.CreateAlloca(EntityTypes[enttypeStr]);
    ScopeStack.back()->HeapAllocas.push_back(enttyPtr);
    // Now we init entity's fields
    auto numFields = EntitySignatures[enttypeStr].size();
    for(unsigned int fieldIndex = 0; fieldIndex < numFields; ++fieldIndex){
        // if a value was assigned to the field, check its type against the field type before storing in the field's address
        if(auto& fieldExpr = entityCons[fieldIndex]){
            auto fieldV = fieldExpr->codegen(); 
            auto fieldExprT = fieldV->getType();
            auto fieldAddr = iRGenHelpers::getGEPAt(enttyPtr, iRGenHelpers::getLLVMConstantInt(fieldIndex));
            auto fieldAddrT =  fieldAddr->getType()->getPointerElementType();

            if(fieldAddrT->isDoubleTy() and fieldExprT->isIntegerTy()) {
                fieldV = builder.CreateCast(Instruction::SIToFP, fieldV, builder.getDoubleTy());
            }

            else if(fieldAddrT->isIntegerTy() and fieldExprT->isDoubleTy()) {
                fieldV = builder.CreateCast(Instruction::FPToSI, fieldV, builder.getInt64Ty());
            }

            if(not iRGenHelpers::isEqual(fieldAddrT, fieldExprT))
                return iRGenHelpers::LogCodeGenError("Trying to assign wrong type to " 
                                        + EntitySignatures[enttypeStr][fieldIndex]
                                        + " in construction of entity of type " + enttypeStr);
            builder.CreateStore(fieldV, fieldAddr);
        }
    }
    return enttyPtr;
}


Value* EntityOperation::codegen() {
    LogProgress("Accessing entity field: " + selectedField);

    auto enttyPtr = entity->codegen();
    auto enttypeTy = iRGenHelpers::isEnttyPtr(enttyPtr);
    
    if(not enttypeTy)
        return iRGenHelpers::LogCodeGenError("Trying to access a field of a value that is not an entity !!");

    // We are now sure that entityV indeed holds the address of an entity

    std::string enttypeStr;
    std::vector<std::string> enttypeSignature;
    if (iRGenHelpers::isListePtr(enttyPtr)) {
        // entity is a list.
        enttypeStr = "liste";
        enttypeSignature = {"size"};
    }

    else {
        // get entity Type name and strip "Entity." prefix
        enttypeStr = std::string(enttypeTy->getName());
        auto [start, end] = std::make_tuple(enttypeStr.find("."), enttypeStr.size());
        enttypeStr = enttypeStr.substr(start+1, end - start);
        // get signature with entity Type name and deduce index
        enttypeSignature = EntitySignatures[enttypeStr];
    }

    auto it = std::find(enttypeSignature.begin(), enttypeSignature.end(), selectedField);
    if(it == enttypeSignature.end())
        return iRGenHelpers::LogCodeGenError("[ " + selectedField + " ]" + " is not a valid field of a " 
                                                + enttypeStr + " entity");
    
    auto fieldIdx = it - enttypeSignature.begin();
    // compute the addr of the field
    auto addr = iRGenHelpers::getGEPAt(enttyPtr, iRGenHelpers::getLLVMConstantInt(fieldIdx));
    
    if(ctx) {
        // If ctx is a Load Op then check init status for this field before the load Op
        return builder.CreateLoad(addr);
    }
    // do not load. So the operator that injected this ctx will surely but something in this field
    // so we update the init status
    if(iRGenHelpers::isListePtr(enttyPtr))
        return iRGenHelpers::LogCodeGenError("kronk can't allow you to modify the size property of a liste");

    return addr;
}


Value* Assignment::codegen() {
    LogProgress("Creating assignment");

    // first inject the respective contexts to the left and right hand side be codegen 'ing them.
    lhs->injectCtx("Store");
    rhs->injectCtx("Load"); // this isn't necessary as this is the default context

    auto lhsV = lhs->codegen(); // always returns a pointer type like i1*, i64*, double*, { i64 }**, { i64 }*
    auto rhsV = rhs->codegen(); // only returns a pointer in case there is an entity.

    auto lhsTy = lhsV->getType()->getPointerElementType();
    auto rhsTy = rhsV->getType();
    

    if(lhsTy->isStructTy() or iRGenHelpers::isEnttyPtr(lhsTy)) {
        // if lhsV points to an entity, then rhsV must also point to an entity of the same type as lhsV

        // this takes care of the special case where rhs is an Identifier Node that codegens an entity ptr
        // which ignores the ctx, giving types like { i64 }* instead of { i64 }**. Any call to lhsTy->isStrucTy() inside
        // this if block is dealing with this exception
        auto lvalueTy = (lhsTy->isStructTy()) ? lhsV->getType() : lhsTy;

        if(not iRGenHelpers::isEnttyPtr(rhsV)) 
            return iRGenHelpers::LogCodeGenError("Trying to assign a primitive to an entity");
        if(not iRGenHelpers::isEqual(lvalueTy->getPointerElementType(), rhsTy->getPointerElementType()))
            return iRGenHelpers::LogCodeGenError("Assignment operand types do not match");

        if(lhsTy->isStructTy()) {
            iRGenHelpers::copyEntty(lhsV, rhsV);
            return lhsV;
        }
    
        AllocaInst* alloc = builder.CreateAlloca(lvalueTy->getPointerElementType());
        iRGenHelpers::copyEntty(alloc, rhsV);
        builder.CreateStore(alloc, lhsV);
        return alloc;
    }
    
    // lhsV is a primitive type
    if(iRGenHelpers::isEnttyPtr(rhsV))
        return iRGenHelpers::LogCodeGenError("Trying to assign an entity to a primitive");

    if(lhsTy->isDoubleTy() and rhsTy->isIntegerTy()) {
        rhsV = builder.CreateCast(Instruction::SIToFP, rhsV, builder.getDoubleTy());
    }

    else if(lhsTy->isIntegerTy() and rhsTy->isDoubleTy()) {
        rhsV = builder.CreateCast(Instruction::FPToSI, rhsV, builder.getInt64Ty());
    }

    else if(not iRGenHelpers::isEqual(lhsTy, rhsTy)) {
        return iRGenHelpers::LogCodeGenError("Assignment operand types do not match");
    }

    builder.CreateStore(rhsV, lhsV);
    return rhsV;
}


Value* BinaryExpr::codegen() {
    /**
     * For +,-,/,* , if one of operands is a float then we cast all other non floats to floats and create a float BinOp.
     * For comparison operators, cast operands must be the same.
     * */

    if(Op == '=') {
        auto assn = std::make_unique<Assignment>(std::move(LHS), std::move(RHS));
        return assn->codegen();
    }
    
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();

    if (iRGenHelpers::isEnttyPtr(L) or iRGenHelpers::isEnttyPtr(R)) {
        if(iRGenHelpers::isListePtr(L) and iRGenHelpers::isListePtr(R)) {
            if(Op != '+')
                return iRGenHelpers::LogCodeGenError(
                                "The only operation allowed between two entities is list concatenation");
            return iRGenHelpers::concatLists(L, R);
        }
        return iRGenHelpers::LogCodeGenError(
                        "Your trying to perform a binary operation on two entities that are not both listes !!");
    }
    
    if(L->getType()->isDoubleTy() || R->getType()->isDoubleTy()){
        L = builder.CreateCast(Instruction::SIToFP, L, Type::getDoubleTy(context));
        R = builder.CreateCast(Instruction::SIToFP, R, Type::getDoubleTy(context));
    }
    
    switch (Op) {
        case '+':
            if(L->getType()->isDoubleTy())
                return builder.CreateFAdd(L, R);
            return builder.CreateAdd(L, R);
        case '-':
            if(L->getType()->isDoubleTy())
                return builder.CreateFSub(L, R);
            return builder.CreateNSWSub(L, R);
        case '*':
            if(L->getType()->isDoubleTy())
                return builder.CreateFMul(L, R);
            return builder.CreateMul(L, R);
        case '/':
            // check division by zero
            if(L->getType()->isDoubleTy())
                return builder.CreateFDiv(L, R);
            return builder.CreateSDiv(L, R, "", true);
        case '<':
            if(L->getType()->isDoubleTy())
                return builder.CreateFCmpOLT(L, R);
            
            return builder.CreateICmpSLT(L, R);
        case '>':
            if(L->getType()->isDoubleTy())
                return builder.CreateFCmpOGT(L, R);
            return builder.CreateICmpSGT(L, R);
        default:
            return iRGenHelpers::LogCodeGenError("Unkown Binary Operator " + std::string(&Op));
    }
}


Value* ReturnExpr::codegen() {
    LogProgress("Generating Return Stmt");
    Value* rvalue = returnExpr->codegen();
    Value* lvalue = ScopeStack.back()->returnValue;

    if(rvalue->getType()->getPointerTo() !=  lvalue->getType()){ // if lvalue type isneq to rvalue type, then try to cast rvalue type to lvalue type
        if(lvalue->getType()->getPointerElementType()->isDoubleTy() && rvalue->getType()->isIntegerTy()) // cast int to double  
            rvalue = builder.CreateCast(Instruction::SIToFP, rvalue, Type::getDoubleTy(context));
        else if(lvalue->getType()->getPointerElementType()->isIntegerTy() && rvalue->getType()->isDoubleTy())
            rvalue = builder.CreateCast(Instruction::FPToSI, rvalue, Type::getInt32Ty(context));
        else
            return iRGenHelpers::LogCodeGenError("Return value does not correspond to function return type");
        
    }
    return builder.CreateStore(rvalue, lvalue);
}


Value* FunctionCallNode::codegen() {
    LogProgress("Creating function call to " + Callee->name);
    std::vector<Value*> ArgsV; // holds codegen IR for each argument
    std::string StringFormat; // holds string formatting for all arguments
    // we need to verify the type of the args
    if(Callee->name == "afficher") {
        for(auto& arg : Args){ // add a genHelper that does this work
            if(auto v = arg->codegen()){ 
                if(v->getType()->isDoubleTy())
                    StringFormat += "%.4f "; // we treat lire and afficher differently because of this
                else if(v->getType()->isIntegerTy())
                    StringFormat += "%d ";
                ArgsV.push_back(v);
            }
        }
        StringFormat += "\n";
        ArgsV.insert(ArgsV.begin(), builder.CreateGlobalStringPtr(StringFormat));

        auto printfF = module->getOrInsertFunction("printf", FunctionType::get(builder.getInt32Ty(), true));
        return builder.CreateCall(printfF, ArgsV, "printfCall");
    }
    
    if(Callee->name == "lire"){
        for(auto& arg : Args){ // ar should always be an identifier.
            Identifier* id = dynamic_cast<Identifier*>(arg.get());
            if(auto v = arg->codegen()){ 
                if(v->getType()->isDoubleTy())
                    StringFormat += "%lf "; // 
                else if(v->getType()->isIntegerTy())
                    StringFormat += " %d";
                ArgsV.push_back(ScopeStack.back()->SymbolTable[id->name]);
            }else return nullptr;
        }
        ArgsV.insert(ArgsV.begin(), builder.CreateGlobalStringPtr(StringFormat));
        return builder.CreateCall(module->getFunction("scanf"), ArgsV, "scanfCall");
    }
    
    for(auto& arg : Args){
        if(auto v = arg->codegen())
            ArgsV.push_back(v);
        else return nullptr;
    }
    return builder.CreateCall(module->getFunction(Callee->name), ArgsV, "f_call");
}


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


Value* EntityDefn::codegen() {
    StructType* structTy = StructType::create(context, "Entity." + entityName);
    EntityTypes[entityName] = structTy;
    
    // now we set the member types
    std::vector<Type*> memTys;
    for(auto& memtypeId : memtypeIds) {
        auto memTy = memtypeId->typegen();
        // entities with entity members contain pointers to those entity members
        memTy = (memTy->isStructTy()) ? memTy->getPointerTo() : memTy;
        memTys.push_back(memTy);
    }

    structTy->setBody(memTys);

    return static_cast<Value*>(nullptr);
}


Value* Prototype::codegen() {
    LogProgress("Creating Prototype for " + funcName->name);
    std::vector<Type*> ArgTypes;
    for(auto& type: argTypes){
        Type* t = builder.getInt64Ty();//typeOf(type->name);
        ArgTypes.push_back(t);
    }
    llvm::FunctionType *FnType = llvm::FunctionType::get(builder.getInt64Ty(), ArgTypes, false); //typeOf(returnType->name)
    llvm::Function* fn = llvm::Function::Create(FnType, llvm::GlobalValue::InternalLinkage, funcName->name, module.get());

    // Set names for all arguments so ir code is readable
    unsigned Idx = 0;
    for (auto &Arg : fn->args())
        Arg.setName(argNames[Idx++]->name);

    return fn;
}


Value* FunctionDefn::codegen(){
    BasicBlock* PreFuncBlock = builder.GetInsertBlock();
    LogProgress("Creating Function definition");
    if(!prototype->codegen())
        return nullptr;

    Function* f = module->getFunction(prototype->funcName->name);
    BasicBlock *bb = BasicBlock::Create(context, "entry", f);
    ScopeStack.push_back(std::make_unique<Scope>());
    
    builder.SetInsertPoint(bb);

    ScopeStack.back()->returnValue = builder.CreateAlloca(f->getReturnType(), nullptr, "returnValue"); 

    for(auto &arg : f->args()){ // set the name->address correspondence in the local symboltable for each argument
        llvm::AllocaInst *alloca = builder.CreateAlloca(arg.getType(), 0, ""); // allocate memory on the stack to hold the arguments
        builder.CreateStore(static_cast<llvm::Value*>(&arg), alloca); // store each argument in each allocated memory space
        ScopeStack.back()->SymbolTable[std::string(arg.getName())] = alloca; // we use the names set in the protottype declaration.
    }
    for(auto& s : Body){
        s->codegen();
    }
    builder.CreateRet(builder.CreateLoad(ScopeStack.back()->returnValue));

    if(!llvm::verifyFunction(*f))
        return iRGenHelpers::LogCodeGenError("There's a problem with your function definition kronk can't figure out");
    ScopeStack.pop_back();
    // We should instead just return to main.
    builder.SetInsertPoint(PreFuncBlock); // WHY ARE WE DOING THIS?? Because Kronk has no main function. We can 
                                          // continue writing expressions after a function definition.
    return f;

}