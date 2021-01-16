#include "typeInfo.h"


namespace typeInfo {


bool isEqual(Type *lhsTy, Type *rhsTy) {
    // Inspiration taken from https://gist.github.com/quantumsheep/bab733c49b7475d5cee1c45faa816a50

    
    return (lhsTy == rhsTy) ? true : false;
}


bool isBool(Type* ty) {
    return ty->isIntegerTy(1);
}


bool isBool(Value* v) {
    return isBool(v->getType());
} 


bool isReel(Type* ty) {
    return ty->isDoubleTy();
}


bool isReel(Value* v) {
    return isReel(v->getType());
}


/// Checks if a given Type* points to a kronk entity. If it does it returns the pointerelementype
StructType* isEnttyPtr(Type* type) {
    if(type->isPointerTy()) {
        auto vty = type->getPointerElementType();
        return dyn_cast<StructType>(vty);
    }

    return nullptr;
}


/// Checks if a given Value* points to a kronk entity. If it does it returns the pointerelementype
StructType* isEnttyPtr(Value* v) {
    return isEnttyPtr(v->getType());
}


/// Checks whether a given Value* points to a kronk liste 
bool isListePtr(Value* v) {
   if(auto vty = isEnttyPtr(v)) {
       return vty->isLiteral();
   }

   return false;
}


} /// end of typeInfo namespace