#include "IRGenAide.h"
#include "Names.h"


namespace irGenAide {

// Error Logging
LLVM_ATTRIBUTE_NORETURN
void LogCodeGenError(std::string errMsg) {
    outs()  << "Code Generation Error in "
            << names::getModuleFile().filename() << '\n'
            << "[Line "
            << (Attr::CurrentLexerLine - Attr::IRGenLineOffset - 1)
            << "]: " 
            << errMsg 
            << '\n';

    exit(EXIT_FAILURE);
}


// Tries the int<--->double implicit cast on rhsV
Value* DoubletoIntCast(Value* v) {
    return Attr::Builder.CreateCast(Instruction::FPToSI, v, Attr::Builder.getInt64Ty());
}

Value* InttoDoubleCast(Value* v) {
    return Attr::Builder.CreateCast(Instruction::SIToFP, v, Attr::Builder.getDoubleTy());
}


// Construct a 64 bit signed int //
Value* getConstantInt(int value) {
    return ConstantInt::get(Attr::Builder.getInt64Ty(), value, true);
}


/// Helper function for emiting GEP instructions
Value* getGEPAt(Value* alloc, Value* idxV, bool isDataPtr) {
    if (isDataPtr) {
        return Attr::Builder.CreateGEP(alloc, std::vector<Value*>{idxV});
    }
    auto idxV32 = Attr::Builder.CreateCast(Instruction::Trunc, idxV, Attr::Builder.getInt32Ty());
    return Attr::Builder.CreateGEP(alloc, std::vector<Value*>{ getConstantInt(0), idxV32 }); 
}


// Emits a memcpy instruction //
void emitMemcpy(Value* dst, Value* src, Value* numElemstoCopy) {
    auto dataTypeSize = Attr::ThisModule->getDataLayout().getTypeAllocSize(src->getType()->getPointerElementType())
                                        .getFixedSize();
    
    auto numBytestoCopyV = Attr::Builder.CreateMul(numElemstoCopy, getConstantInt(dataTypeSize));

    Attr::Builder.CreateMemCpy(dst, dst->getPointerAlignment(Attr::ThisModule->getDataLayout()), src,
                                 src->getPointerAlignment(Attr::ThisModule->getDataLayout()), numBytestoCopyV);
}


void deepCopy(Value* enttyPtr) {
    auto enttypeTy = cast<StructType>(enttyPtr->getType()->getPointerElementType());

    // recursively copy all members of this entity that are also entities, since these members are actuallly pointers
    for(uint32_t idx = 0; idx < enttypeTy->getNumElements(); ++idx) {
        if(auto fieldTy = types::isEnttyPtr(enttypeTy->getElementType(idx))) {
            auto fieldAddr = getGEPAt(enttyPtr, getConstantInt(idx));
            auto fieldPtr = Attr::Builder.CreateLoad(fieldAddr);
            AllocaInst* alloc = Attr::Builder.CreateAlloca(fieldTy);

            copyEntty(alloc, fieldPtr);
            Attr::Builder.CreateStore(alloc, fieldAddr);
        }
    }
}


void copyEntty(Value* dst, Value* src) {
    emitMemcpy(dst, src, getConstantInt(1)); // there's just one element of this type

    // also copy data values if src is a liste
    if(types::isListePtr(src)) {
        auto dstDataPtr = Attr::Builder.CreateLoad(getGEPAt(dst, getConstantInt(1)));
        auto srcDataPtr = Attr::Builder.CreateLoad(getGEPAt(src, getConstantInt(1)));
        auto srcDataSize = Attr::Builder.CreateLoad(getGEPAt(src, getConstantInt(0)));
        emitMemcpy(dstDataPtr, srcDataPtr, srcDataSize);

        return; // kronk doesn't deep copy list elements
    }

    deepCopy(dst);
}


// Helper function for filling the members of a list entity. Also fills the data block
// if the values are supplied 
void fillUpListEntty(Value* listPtr, std::vector<Value*> members, std::vector<Value*> dataValues) {
    // fill entity members
    for(int i = 0; i < 2; ++i) {
        auto fieldAddr = irGenAide::getGEPAt(listPtr, irGenAide::getConstantInt(i));
        Attr::Builder.CreateStore(members[i], fieldAddr);
    }
    
    // fill data block if data values are passed
    if(dataValues.size()) {
        for(int i = 0; i < dataValues.size(); ++i) {
            Attr::Builder.CreateStore( dataValues[i], irGenAide::getGEPAt(members[1], irGenAide::getConstantInt(i), true) );
        }
    }
}


// Used to get functions in kronkrt modules
Function* getRtModuleFn(std::string name) {
    auto fn = Attr::Kronkrt->getFunction(name);
    Attr::ThisModule->getOrInsertFunction(fn->getName(), fn->getFunctionType());
    
    return fn;
}


// Used to emit runtime checks ex for list indices
void emitRtCheck(std::string name, std::vector<Value*> Args) {
    auto fn = Attr::Kronkrt->getFunction(name);
    Attr::ThisModule->getOrInsertFunction(fn->getName(), fn->getFunctionType());

    Args.push_back(Attr::Builder.CreateGlobalStringPtr(names::getModuleFile().filename().string()));
    Args.push_back(getConstantInt(Attr::CurrentLexerLine - Attr::IRGenLineOffset - 1));

    Attr::Builder.CreateCall(fn, Args);
}


// Used to emit calls to runtime compiler util functions that are not checks
Value* emitRtCompilerUtilCall(std::string name, std::vector<Value*> Args) {
    auto fn = Attr::Kronkrt->getFunction(name);
    Attr::ThisModule->getOrInsertFunction(fn->getName(), fn->getFunctionType());

    return Attr::Builder.CreateCall(fn, Args);
}

} // end of irGenAide namespace