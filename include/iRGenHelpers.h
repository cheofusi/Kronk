#ifndef _IRGENHELPERS_H_
#define _IRGENHELPERS_H_

#include "Nodes.h"

extern unsigned int currentLexerLine;
extern std::shared_ptr<llvm::Module> module;

namespace iRGenHelpers { // start of namespace iRGenHelpers

std::nullptr_t LogCodeGenError(std::string str);

Value* buildRuntimeErrStr(std::string customMsg);

bool isEqual(const Type *left, const Type *right);

StructType* isEnttyPtr(const Type* type);

StructType* isEnttyPtr(const Value* v);

bool isListePtr(const Value* v);

Value* getLLVMConstantInt(int value);

void emitMemcpy(Value* dst, Value* src, Value* numElemstoCopy);

Value* getGEPAt(Value* alloc, Value* idxV, bool isDataPtr = false);

void fillUpListEntty(Value* listPtr, std::vector<Value*> members, std::vector<Value*> dataValues = {});

void copyEntty(Value* dst, Value* src);

Value* concatLists(Value* L, Value* R);

} // end of namespace iRGenHelpers

#endif