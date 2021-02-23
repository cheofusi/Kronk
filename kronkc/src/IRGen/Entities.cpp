#include "Nodes.h"
#include "IRGenAide.h"
#include "Names.h"


Value* EntityDefn::codegen() {
    LogProgress("Creating entity type definition");

    if(Attr::Builder.GetInsertBlock()->getParent()->getName() != "main") {
        irGenAide::LogCodeGenError("Kronk doesn't allow nesting of entity type definitions");
    }

    StructType* structTy = StructType::create(Attr::Context, enttyTypeId);
    Attr::EntityTypes[enttyTypeId] = structTy;
    
    // now we set the member types
    std::vector<Type*> memTys;
    for(auto& memTypeId : memTypeIds) {
        auto memTy = memTypeId->typegen();
        // entities with entity members contain pointers to those entity members
        memTy = (memTy->isStructTy()) ? memTy->getPointerTo() : memTy;
        memTys.push_back(memTy);
    }

    structTy->setBody(memTys);

    return nullptr;
}


Value* AnonymousEntity::codegen() {
    // first allocate space for the entity. The parser already made sure the entyTypeId exists.
    AllocaInst* enttyPtr = Attr::Builder.CreateAlloca(names::EntityType(enttyTypeId).value());
    Attr::ScopeStack.back()->HeapAllocas.push_back(enttyPtr);
    
    // then init entity's fields. 
    auto numFields = names::EntitySignature(enttyTypeId).value().size();

    for(uint8_t fieldIndex = 0; fieldIndex < numFields; ++fieldIndex) {
        // if a value was assigned to the field, check its type against the field type before 
        // storing in the field's address
        if(enttyCons.find(fieldIndex) != enttyCons.end()) {
            auto& fieldExpr = enttyCons.at(fieldIndex);
            auto fieldV = fieldExpr->codegen(); 
            auto fieldExprTy = fieldV->getType();
            auto fieldAddr = irGenAide::getGEPAt(enttyPtr, irGenAide::getConstantInt(fieldIndex));
            auto fieldAddrTy =  fieldAddr->getType()->getPointerElementType();

            if(not types::isEqual(fieldAddrTy, fieldExprTy)) {
                auto [kmodule, symbol] = names::demangleNameForErrMsg(enttyTypeId);
                irGenAide::LogCodeGenError("Trying to assign wrong type to << " 
                                        + names::EntitySignature(enttyTypeId).value()[fieldIndex]
                                        + " >> in creation of entity of type << " 
                                        + symbol
                                        + " >> defined in the module"
                                        + kmodule);
            }

            Attr::Builder.CreateStore(fieldV, fieldAddr);

        }
    }

    return enttyPtr;
}


Value* EntityOperation::codegen() {
    LogProgress("Accessing entity field: " + fieldId);

    auto enttyPtr = entty->codegen();
    auto enttyTy = types::isEnttyPtr(enttyPtr);
    
    if(not enttyTy)
        irGenAide::LogCodeGenError("Trying to access a field of a value that is not an entity !!");

    // We are now sure that entityV indeed holds the address of an entity

    std::string enttyTypeId;
    std::vector<std::string> enttySignature;

    if (types::isListePtr(enttyPtr)) {
        // entity is a list.
        enttyTypeId = "liste";
        enttySignature = {"size"};
    }

    else {
        // get entity Type name and strip "Entity." prefix
        enttyTypeId = std::string(enttyTy->getName());
        // get signature with entity Type name and deduce index
        enttySignature = names::EntitySignature(enttyTypeId).value();
    }

    auto it = std::find(enttySignature.begin(), enttySignature.end(), fieldId);
    if(it == enttySignature.end()) {
        auto [kmodule, symbol] = names::demangleNameForErrMsg(enttyTypeId);
        irGenAide::LogCodeGenError("<< " + fieldId 
                                    + " >>" 
                                    + " is not a valid field of entity type << " 
                                    + symbol 
                                    + " >> defined in the module" 
                                    + kmodule);
    }
    
    auto fieldIdx = it - enttySignature.begin();
    // compute the addr of the field
    auto addr = irGenAide::getGEPAt(enttyPtr, irGenAide::getConstantInt(fieldIdx));
    
    if(ctx) {
        auto value = Attr::Builder.CreateLoad(addr);
      
        if(types::isListePtr(enttyPtr)) {
            // since we're loading the << size >> field as this is a liste, we cast it to double so it becomes
            // compatible with Values of type reel
            return Attr::Builder.CreateCast(Instruction::SIToFP, value, Attr::Builder.getDoubleTy());
        }

        return value;
    }
    
    if(types::isListePtr(enttyPtr))
        irGenAide::LogCodeGenError("kronk can't allow you to modify the size property of a liste");

    return addr;
}

bool EntityOperation::injectCtx(bool ctx) {
    this->ctx = ctx;
    return true;
}