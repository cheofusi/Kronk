#ifndef _TYPES_H_
#define _TYPES_H_

#include "Attributes.h"

namespace types {


bool isBool(Type* ty);
bool isBool(Value* v);

bool isEqual(Type* left, Type* right);

bool isReel(Type* ty);
bool isReel(Value* v);

StructType* isEnttyPtr(Type* type);
StructType* isEnttyPtr(Value* v);

bool isListePtr(Value* v);
bool isStringPtr(Value* v);

std::string typestr(Value* v);

}  // namespace types

#endif