#ifndef _IRGENAIDE_H_
#define _IRGENAIDE_H_

#include "Attributes.h"
#include "Types.h"


namespace irGenAide {  // start of namespace iRGenAide


void LogCodeGenError(std::string errMsg);

Value* DoubletoIntCast(Value* v);

Value* InttoDoubleCast(Value* v);

Value* getConstantInt(int value);

void emitMemcpy(Value* dst, Value* src, Value* numElemstoCopy);

Value* getGEPAt(Value* alloc, Value* idxV, bool isDataPtr = false);

void copyEntty(Value* dst, Value* src);

void fillUpListEntty(Value* listPtr, std::vector<Value*> members, std::vector<Value*> dataValues = {});

Function* getRtModuleFn(std::string name);

void emitRtCheck(std::string name, std::vector<Value*> Args);

Value* emitRtCompilerUtilCall(std::string name, std::vector<Value*> Args);

}  // namespace irGenAide

#endif