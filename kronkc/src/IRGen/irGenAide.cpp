#include "irGenAide.h"


namespace irGenAide {

// Error Logging
std::nullptr_t LogCodeGenError(std::string str) {
    std::cout <<"Code Generation Error [Around Line " << (currentLexerLine - 1)  << "]:  " << str << std::endl;
    exit(EXIT_FAILURE);
    return nullptr;
}


Value* buildRuntimeErrStr(std::string errMsg) {
    auto errMsgAug = "Line " + std::to_string(currentLexerLine) +  ",  " + errMsg;
    return builder.CreateGlobalStringPtr(errMsgAug);
}


// Tries the int<--->double implicit cast on rhsV
Value* doIntCast(Value* v) {
    return builder.CreateCast(Instruction::FPToSI, v, builder.getInt64Ty());
}


/* Construct a 64 bit signed int*/
Value* getConstantInt(int value) {
    return ConstantInt::get(builder.getInt64Ty(), value, true);
}


/// Helper function for emiting GEP instructions
Value* getGEPAt(Value* alloc, Value* idxV, bool isDataPtr) {
    if (isDataPtr) {
        return builder.CreateGEP(alloc, std::vector<Value*>{idxV});
    }
    auto idxV32 = builder.CreateCast(Instruction::Trunc, idxV, builder.getInt32Ty());
    return builder.CreateGEP(alloc, std::vector<Value*>{ getConstantInt(0), idxV32 }); 
}


/* Emits a memcpy instruction */
void emitMemcpy(Value* dst, Value* src, Value* numElemstoCopy) {
    auto dataTypeSize = module->getDataLayout().getTypeAllocSize(src->getType()->getPointerElementType()).getFixedSize();
    
    auto numBytestoCopyV = builder.CreateMul(numElemstoCopy, getConstantInt(dataTypeSize));

    builder.CreateMemCpy(dst, dst->getPointerAlignment(module->getDataLayout()), src,
                                 src->getPointerAlignment(module->getDataLayout()), numBytestoCopyV);
}


void deepCopy(Value* enttyPtr) {
    auto enttypeTy = cast<StructType>(enttyPtr->getType()->getPointerElementType());

    // recursively copy all members of this entity that are also entities, since these members are actuallly pointers
    for(uint32_t idx = 0; idx < enttypeTy->getNumElements(); ++idx) {
        if(auto fieldTy = typeInfo::isEnttyPtr(enttypeTy->getElementType(idx))) {
            auto fieldAddr = getGEPAt(enttyPtr, getConstantInt(idx));
            auto fieldPtr = builder.CreateLoad(fieldAddr);
            AllocaInst* alloc = builder.CreateAlloca(fieldTy);

            copyEntty(alloc, fieldPtr);
            builder.CreateStore(alloc, fieldAddr);
        }
    }
}


void copyEntty(Value* dst, Value* src) {
    emitMemcpy(dst, src, getConstantInt(1)); // there's just one element of this type

    // also copy data values if src is a liste
    if(typeInfo::isListePtr(src)) {
        auto dstDataPtr = builder.CreateLoad(getGEPAt(dst, getConstantInt(1)));
        auto srcDataPtr = builder.CreateLoad(getGEPAt(src, getConstantInt(1)));
        auto srcDataSize = builder.CreateLoad(getGEPAt(src, getConstantInt(0)));
        emitMemcpy(dstDataPtr, srcDataPtr, srcDataSize);

        return; // kronk doesn't deep copy list elements
    }

    deepCopy(dst);
}


/// Helper function for filling the members of a list entity. Also fills the data block
/// if the values are supplied 
void fillUpListEntty(Value* listPtr, std::vector<Value*> members, std::vector<Value*> dataValues) {
    // fill entity members
    for(int i = 0; i < 2; ++i) {
        auto fieldAddr = irGenAide::getGEPAt(listPtr, irGenAide::getConstantInt(i));
        builder.CreateStore(members[i], fieldAddr);
    }
    
    // fill data block if data values are passed
    if(dataValues.size()) {
        for(int i = 0; i < dataValues.size(); ++i) {
            builder.CreateStore( dataValues[i], irGenAide::getGEPAt(members[1], irGenAide::getConstantInt(i), true) );
        }
    }
}


// checks if a symbol is defined in the current module or in the rt. If it is in the rt the symbol's declaration is
// inserted into the current module. 
std::optional<Function*> getFunction(std::string name) {
    // first check the current module f
    if(auto fn = module->getFunction(name)) {
        return fn;
    }

    if(auto fn = runtimeLibM->getFunction(name)) {
        // insert runtime function declaration if it doesn't exist yet.
        outs() << "Found function in runtime library" << '\n';
        module->getOrInsertFunction(fn->getName(), fn->getFunctionType());
        return fn;
    }

    return std::nullopt;
}


} // end of irGenAide namespace