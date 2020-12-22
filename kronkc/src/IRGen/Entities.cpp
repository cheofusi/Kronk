#include "Nodes.h"
#include "irGenAide.h"


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
            auto fieldExprTy = fieldV->getType();
            auto fieldAddr = irGenAide::getGEPAt(enttyPtr, irGenAide::getConstantInt(fieldIndex));
            auto fieldAddrTy =  fieldAddr->getType()->getPointerElementType();

            if(fieldAddrTy->isDoubleTy() and fieldExprTy->isIntegerTy()) {
                fieldV = builder.CreateCast(Instruction::SIToFP, fieldV, builder.getDoubleTy());
            }

            else if(fieldAddrTy->isIntegerTy() and fieldExprTy->isDoubleTy()) {
                fieldV = builder.CreateCast(Instruction::FPToSI, fieldV, builder.getInt64Ty());
            }

            if(not irGenAide::isEqual(fieldAddrTy, fieldExprTy))
                return irGenAide::LogCodeGenError("Trying to assign wrong type to " 
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
    auto enttypeTy = irGenAide::isEnttyPtr(enttyPtr);
    
    if(not enttypeTy)
        return irGenAide::LogCodeGenError("Trying to access a field of a value that is not an entity !!");

    // We are now sure that entityV indeed holds the address of an entity

    std::string enttypeStr;
    std::vector<std::string> enttypeSignature;
    if (irGenAide::isListePtr(enttyPtr)) {
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
        return irGenAide::LogCodeGenError("[ " + selectedField + " ]" + " is not a valid field of a " 
                                                + enttypeStr + " entity");
    
    auto fieldIdx = it - enttypeSignature.begin();
    // compute the addr of the field
    auto addr = irGenAide::getGEPAt(enttyPtr, irGenAide::getConstantInt(fieldIdx));
    
    if(ctx) {
        // If ctx is a Load Op then check init status for this field before the load Op
        return builder.CreateLoad(addr);
    }
    // do not load. So the operator that injected this ctx will surely but something in this field
    // so we update the init status
    if(irGenAide::isListePtr(enttyPtr))
        return irGenAide::LogCodeGenError("kronk can't allow you to modify the size property of a liste");

    return addr;
}