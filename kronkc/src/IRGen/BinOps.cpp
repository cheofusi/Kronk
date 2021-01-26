#include "Nodes.h"
#include "irGenAide.h"


Value* Assignment::codegen() {
    LogProgress("Creating assignment");

    // first try to inject a store context into the left side be codegen 'ing it.
    if(not lhs->injectCtx(0)) {
        irGenAide::LogCodeGenError("Invalid expression on the left hand side of assigment");
    }

    auto lhsV = lhs->codegen(); // always returns a pointer type ex i1*, double*, { double }**, { double }*
    auto rhsV = rhs->codegen(); // only returns a pointer in case there is an entity.

    auto lhsTy = lhsV->getType()->getPointerElementType();
    auto rhsTy = rhsV->getType();
    

    if(lhsTy->isStructTy() or typeInfo::isEnttyPtr(lhsTy)) {
        // if lhsV points to an entity, then rhsV must also point to an entity of the same type as lhsV

        // this takes care of the special case where rhs is an Identifier Node that codegens an entity ptr
        // which ignores the ctx, giving types like { double }* instead of { double }**. Any call to lhsTy->isStrucTy() 
        // inside this if block is dealing with this exception
        auto lvalueTy = (lhsTy->isStructTy()) ? lhsV->getType() : lhsTy;

        if(not typeInfo::isEnttyPtr(rhsV)) 
            irGenAide::LogCodeGenError("Trying to assign a primitive to an entity");

        if(not typeInfo::isEqual(lvalueTy->getPointerElementType(), rhsTy->getPointerElementType()))
            irGenAide::LogCodeGenError("Assignment operand types do not match");

        if(lhsTy->isStructTy()) {
            irGenAide::copyEntty(lhsV, rhsV);
            return lhsV;
        }
    
        AllocaInst* alloc = Attr::Builder.CreateAlloca(lvalueTy->getPointerElementType());
        irGenAide::copyEntty(alloc, rhsV);
        Attr::Builder.CreateStore(alloc, lhsV);
        return alloc;
    }

    if(lhsTy->isIntegerTy(8)) {
        // modifying a character in a string. 
        if(not typeInfo::isStringPtr(rhsV))
            irGenAide::LogCodeGenError("Trying to replace a character of a string with a non-string value");
        
        // TODO: insert runtime check to see if rhsV is a 1 character string. 
        auto character = Attr::Builder.CreateLoad(irGenAide::getGEPAt(rhsV, irGenAide::getConstantInt(1)));
        Attr::Builder.CreateStore(character, lhsV);
    }
    
    // lhsV & rhsV are both primitives 

    if(not typeInfo::isEqual(lhsTy, rhsTy)) {
        irGenAide::LogCodeGenError("Operand types of assignment do not match");
    }

    Attr::Builder.CreateStore(rhsV, lhsV);
    return rhsV;
}


Value* BinaryExpr::codegen() {
    // 

    if(Op == "=") {
        auto assn = std::make_unique<Assignment>(std::move(lhs), std::move(rhs));
        return assn->codegen();
    }
    
    Value* lhsV = lhs->codegen();
    Value* rhsV = rhs->codegen();

    if(typeInfo::isEnttyPtr(lhsV) or typeInfo::isEnttyPtr(rhsV)) {
        if(typeInfo::isListePtr(lhsV) and typeInfo::isListePtr(rhsV)) {
            if(Op != "+")
                irGenAide::LogCodeGenError(
                        "The only binary operation allowed between two entities is list concatenation");
            
            auto listConcat = std::make_unique<ListConcatenation>(lhsV, rhsV);
            return listConcat->codegen();
        }

        irGenAide::LogCodeGenError(
            "Your trying to perform a binary operation on two entities that are not both listes !!");
    }

    if(typeInfo::isBool(lhsV) and typeInfo::isBool(rhsV)) {
        if(Op == "et")
            return Attr::Builder.CreateAnd(lhsV, rhsV);
        
        if(Op == "ou")
            return Attr::Builder.CreateOr(lhsV, rhsV);
        
        if(Op == "==")
            return Attr::Builder.CreateICmpEQ(lhsV, rhsV);
        
        if(Op == "!=")
            return Attr::Builder.CreateICmpNE(lhsV, rhsV);

        irGenAide::LogCodeGenError("Unkown Binary Operator << " + Op + " >> between two booleans");
    }
    
    if(typeInfo::isReel(lhsV) and typeInfo::isReel(rhsV)) {
            if(Op == "**") {
                // call runtime exponent function
            }

            if(Op == "*")
                return Attr::Builder.CreateFMul(lhsV, rhsV);
                
            if(Op == "/")
                // TODO check division by zero
                return Attr::Builder.CreateFDiv(lhsV, rhsV);

            if(Op == "mod") {
                // call runtime modulo function

            }

            if(Op == "+")
                return Attr::Builder.CreateFAdd(lhsV, rhsV);

            if(Op == "-")
                return Attr::Builder.CreateFSub(lhsV, rhsV);

            if(Op == "<<") {
            
            }

            if(Op == ">>") {

            }

            if(Op == "<")
                return Attr::Builder.CreateFCmpOLT(lhsV, rhsV);
            
            if(Op == ">")
                return Attr::Builder.CreateFCmpOGT(lhsV, rhsV);

            if(Op == "<=")
                return Attr::Builder.CreateFCmpOLE(lhsV, rhsV);

            if(Op == ">=")
                return Attr::Builder.CreateFCmpOGE(lhsV, rhsV);

            irGenAide::LogCodeGenError("Unkown Binary Operator << " + Op + " >> between two reels");
    }
}