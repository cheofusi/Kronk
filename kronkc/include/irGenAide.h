#ifndef _IRGENAIDE_H_
#define _IRGENAIDE_H_

#include "Attributes.h"


namespace irGenAide { // start of namespace iRGenAide

std::nullptr_t LogCodeGenError(std::string str);

Value* buildRuntimeErrStr(std::string customMsg);

bool isEqual(const Type *left, const Type *right);

StructType* isEnttyPtr(const Type* type);

StructType* isEnttyPtr(const Value* v);

bool isListePtr(const Value* v);

Value* getConstantInt(int value);

void emitMemcpy(Value* dst, Value* src, Value* numElemstoCopy);

Value* getGEPAt(Value* alloc, Value* idxV, bool isDataPtr = false);

void copyEntty(Value* dst, Value* src);

void fillUpListEntty(Value* listPtr, std::vector<Value*> members, std::vector<Value*> dataValues = {});

} // end of namespace iRGenAide

#endif