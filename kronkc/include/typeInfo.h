#ifndef _TYPEINFO_H_
#define _TYPEINFO_H_

#include "Attributes.h"

namespace typeInfo {


bool isBool(Type* ty);

bool isBool(Value* v);

bool isEqual(Type *left, Type *right);

bool isReel(Type* ty);

bool isReel(Value* v);

StructType* isEnttyPtr(Type* type);

StructType* isEnttyPtr(Value* v);

bool isListePtr(Value* v);

bool isStringPtr(Value* v);

}

#endif