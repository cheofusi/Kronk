#include "Nodes.h"
#include "irGenAide.h"


Value* EntityDefn::codegen() {
    LogProgress("Creating entity type definition");

    if(Attr::Builder.GetInsertBlock()->getParent()->getName() != "main") {
        irGenAide::LogCodeGenError("Kronk doesn't allow nesting of entity type definitions");
    }

    StructType* structTy = StructType::create(Attr::Context, "Entity." + entityName);
    Attr::EntityTypes[entityName] = structTy;
    
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
    AllocaInst* enttyPtr = Attr::Builder.CreateAlloca(Attr::EntityTypes[enttypeStr]);
    Attr::ScopeStack.back()->HeapAllocas.push_back(enttyPtr);
    // Now we init entity's fields
    auto numFields = Attr::EntitySignatures[enttypeStr].size();
    for(unsigned int fieldIndex = 0; fieldIndex < numFields; ++fieldIndex) {
        // if a value was assigned to the field, check its type against the field type before 
        // storing in the field's address
        if(entityCons.find(fieldIndex) != entityCons.end()) {
            auto& fieldExpr = entityCons.at(fieldIndex);
            auto fieldV = fieldExpr->codegen(); 
            auto fieldExprTy = fieldV->getType();
            auto fieldAddr = irGenAide::getGEPAt(enttyPtr, irGenAide::getConstantInt(fieldIndex));
            auto fieldAddrTy =  fieldAddr->getType()->getPointerElementType();

            if(not typeInfo::isEqual(fieldAddrTy, fieldExprTy)) {
                irGenAide::LogCodeGenError("Trying to assign wrong type to << " 
                                        + Attr::EntitySignatures[enttypeStr][fieldIndex]
                                        + " >> in creation of entity of type << " + enttypeStr + " >>");
            }

            Attr::Builder.CreateStore(fieldV, fieldAddr);

        }
    }

    return enttyPtr;
}


Value* EntityOperation::codegen() {
    LogProgress("Accessing entity field: " + selectedField);

    auto enttyPtr = entity->codegen();
    auto enttypeTy = typeInfo::isEnttyPtr(enttyPtr);
    
    if(not enttypeTy)
        irGenAide::LogCodeGenError("Trying to access a field of a value that is not an entity !!");

    // We are now sure that entityV indeed holds the address of an entity

    std::string enttypeStr;
    std::vector<std::string> enttypeSignature;
    if (typeInfo::isListePtr(enttyPtr)) {
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
        enttypeSignature = Attr::EntitySignatures[enttypeStr];
    }

    auto it = std::find(enttypeSignature.begin(), enttypeSignature.end(), selectedField);
    if(it == enttypeSignature.end())
        irGenAide::LogCodeGenError("<< " + selectedField + " >>" + " is not a valid field of << " 
                                                + enttypeStr + " >> entity");
    
    auto fieldIdx = it - enttypeSignature.begin();
    // compute the addr of the field
    auto addr = irGenAide::getGEPAt(enttyPtr, irGenAide::getConstantInt(fieldIdx));
    
    if(ctx) {
        auto value = Attr::Builder.CreateLoad(addr);
      
        if(typeInfo::isListePtr(enttyPtr)) {
            // since we're loading the << size >> field as this is a liste, we cast it to double so it becomes
            // compatible with Values of type nbre
            return Attr::Builder.CreateCast(Instruction::SIToFP, value, Attr::Builder.getDoubleTy());
        }

        return value;
    }
    
    if(typeInfo::isListePtr(enttyPtr))
        irGenAide::LogCodeGenError("kronk can't allow you to modify the size property of a liste");

    return addr;
}

bool EntityOperation::injectCtx(bool ctx) {
    this->ctx = ctx;
    return true;
}