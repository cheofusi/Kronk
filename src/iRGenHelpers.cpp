#include "iRGenHelpers.h"

namespace iRGenHelpers {

// Error Logging
std::nullptr_t LogCodeGenError(std::string str) {
    std::cout <<"Code Generation Error [Line " << currentLexerLine << "]:  " << str << std::endl;
    exit(EXIT_FAILURE);
    return nullptr;
}


Value* buildRuntimeErrStr(std::string customMsg) {
    auto errMsg = "Line " + std::to_string(currentLexerLine) +  ",  " + customMsg;
    return builder.CreateGlobalStringPtr(errMsg);
}


bool isEqual(const Type *left, const Type *right) {
    /*  Inspiration taken from https://gist.github.com/quantumsheep/bab733c49b7475d5cee1c45faa816a50

        Kronk's type system only maps to llvm's primitive types ( i1, i64, double) and the identified struct
        derived type. So we concern ourselves with those
    */
//    if(auto left_struct = dyn_cast<StructType>(left)){
//        if(auto right_struct = dyn_cast<StructType>(right)){
//            left_struct->isLiteral();
//        }
//    }
    
    return (left == right) ? true : false;
}


/// Checks if a given Type* points to a kronk entity. If it does it returns the pointerelementype
StructType* isEnttyPtr(const Type* type) {
    if(type->isPointerTy()) {
        auto vty = type->getPointerElementType();
        return dyn_cast<StructType>(vty);
    }
    return nullptr;
}


/// Checks if a given Value* points to a kronk entity. If it does it returns the pointerelementype
StructType* isEnttyPtr(const Value* v) {
    return isEnttyPtr(v->getType());
}


/* Checks whether a given Value* points to a kronk liste */
bool isListePtr(const Value* v) {
   if(auto vty = isEnttyPtr(v)) {
       return vty->isLiteral();
   }
   return false;
}


/* Construct a 64 bit signed int*/
Value* getLLVMConstantInt(int value) {
    return ConstantInt::get(builder.getInt64Ty(), value, true);
}


/* Emits a memcpy instruction */
void emitMemcpy(Value* dst, Value* src, Value* numElemstoCopy) {
    auto dataTypeSize = module->getDataLayout().getTypeAllocSize(src->getType()->getPointerElementType()).getFixedSize();
    
    auto numBytestoCopyV = builder.CreateMul(numElemstoCopy, getLLVMConstantInt(dataTypeSize));

    builder.CreateMemCpy(dst, dst->getPointerAlignment(module->getDataLayout()), src,
                                 src->getPointerAlignment(module->getDataLayout()), numBytestoCopyV);
}


/// Helper function for emiting GEP instructions
Value* getGEPAt(Value* alloc, Value* idxV, bool isDataPtr) {
    if (isDataPtr) {
        return builder.CreateGEP(alloc, std::vector<Value*>{idxV});
    }
    auto idxV32 = builder.CreateCast(Instruction::Trunc, idxV, builder.getInt32Ty());
    return builder.CreateGEP(alloc, std::vector<Value*>{ getLLVMConstantInt(0), idxV32 }); 
}


/// Helper function for filling the members of a list entity. Also fills the data block
/// if the values are supplied 
void fillUpListEntty(Value* listPtr, std::vector<Value*> members, std::vector<Value*> dataValues) {
    // fill entity members
    for(int i = 0; i < 2; ++i) {
        auto fieldAddr = getGEPAt(listPtr, getLLVMConstantInt(i));
        builder.CreateStore(members[i], fieldAddr);
    }
    
    // fill data block if data values are passed
    if(dataValues.size()) {
        for(int i = 0; i < dataValues.size(); ++i) {
            builder.CreateStore( dataValues[i], getGEPAt(members[1], getLLVMConstantInt(i), true) );
        }
    }
}


void copyEntty(Value* dst, Value* src) {
    emitMemcpy(dst, src, getLLVMConstantInt(1)); // there's just one element of this type

    // also copy data values if src is a liste
    if(isListePtr(src)) {
        auto dstDataPtr = builder.CreateLoad(getGEPAt(dst, getLLVMConstantInt(1)));
        auto srcDataPtr = builder.CreateLoad(getGEPAt(src, getLLVMConstantInt(1)));
        auto srcDataSize = builder.CreateLoad(getGEPAt(src, getLLVMConstantInt(0)));
        emitMemcpy(dstDataPtr, srcDataPtr, srcDataSize);
    }
}


Value* concatLists(Value* L, Value* R) {

    // now the magic begins
    auto Lsize = builder.CreateLoad(getGEPAt(L, getLLVMConstantInt(0)));
    auto LdataPtr = builder.CreateLoad(getGEPAt(L, getLLVMConstantInt(1)));
    auto Rsize = builder.CreateLoad(getGEPAt(R, getLLVMConstantInt(0)));
    auto RdataPtr = builder.CreateLoad(getGEPAt(R, getLLVMConstantInt(1)));

    // check whether the data types are equal
    if(not isEqual(LdataPtr->getType(), RdataPtr->getType()))
        return LogCodeGenError("Trying to concatenate lists with unequal types");
    
    auto newListSizeV = builder.CreateAdd(Lsize, Rsize);
    // The starting address of of the underlying memory block
    AllocaInst* newDataPtr = builder.CreateAlloca(LdataPtr->getType()->getPointerElementType(), newListSizeV);

    // we now get all data from both listes in order
    emitMemcpy(newDataPtr, LdataPtr, Lsize);
    emitMemcpy(getGEPAt(newDataPtr, Lsize, true), RdataPtr, Rsize);

    AllocaInst* newlistPtr = builder.CreateAlloca(StructType::get(builder.getInt64Ty(), newDataPtr->getType()));

    fillUpListEntty(newlistPtr, std::vector<Value*>{newListSizeV, newDataPtr});

    return newlistPtr;
}


}