#include "Types.h"
#include "IRGenAide.h"

namespace types {


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


/// Checks if a given Value* points to a kronk liste 
bool isListePtr(Type* ty) {
   if(auto vty = isEnttyPtr(ty)) {
       return vty->isLiteral();
   }

   return false;
}

bool isListePtr(Value* v) {
    return isListePtr(v->getType());
}


/// Checks if a given Value* points to a string (or kronk liste with an i8 data elements)
bool isStringPtr(Type* ty) {
    if(auto vty = isEnttyPtr(ty)) {
        if(vty->isLiteral()) {
            return vty->getElementType(1)->getPointerElementType()->isIntegerTy(8);
        }
    }
    
    return false;
}

bool isStringPtr(Value* v) {
    return isStringPtr(v->getType());
}



std::string typestr(Type* ty) {
    if(isBool(ty)) {
        return "bool";
    }

    if(isReel(ty)) {
        return "reel";
    }

    if(isStringPtr(ty)) {
        return "str";
    }

    if(isListePtr(ty)) {
        auto vty = ty->getPointerElementType();
        auto elemty = llvm::cast<StructType>(vty)->getElementType(1)->getPointerElementType();
        auto elemtypestr = typestr(elemty);

        return "liste(" + elemtypestr + ")";
    }

    // is a user-defined entity
    return llvm::cast<StructType>(ty->getPointerElementType())->getName();
}

std::string typestr(Value* v) {
    return typestr(v->getType());
}


} /// end of typeInfo namespace