#include "Nodes.h"
#include "irGenAide.h"


Value* Assignment::codegen() {
    LogProgress("Creating assignment");

    // first try to inject a store context into the left side be codegen 'ing it.
    if(not lhs->injectCtx(0)) {
        irGenAide::LogCodeGenError("Invalid expression on the left hand side of assigment");
    }

    auto lhsV = lhs->codegen(); // always returns a pointer type like i1*, double*, { double }**, { double }*
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
    
        AllocaInst* alloc = builder.CreateAlloca(lvalueTy->getPointerElementType());
        irGenAide::copyEntty(alloc, rhsV);
        builder.CreateStore(alloc, lhsV);
        return alloc;
    }
    
    // lhsV & rhsV are both primitives 

    if(not typeInfo::isEqual(lhsTy, rhsTy)) {
        irGenAide::LogCodeGenError("Operand types of assignment do not match");
    }

    builder.CreateStore(rhsV, lhsV);
    return rhsV;
}


Value* BinaryExpr::codegen() {
    /**
     * For +,-,/,* , if one of operands is a float then we cast all other non floats to floats and create a float BinOp.
     * For comparison operators, cast operands must be the same.
     * */

    if(Op == "=") {
        auto assn = std::make_unique<Assignment>(std::move(lhs), std::move(rhs));
        return assn->codegen();
    }
    
    Value* lhsV = lhs->codegen();
    Value* rhsV = rhs->codegen();

    if(typeInfo::isEnttyPtr(lhsV) or typeInfo::isEnttyPtr(rhsV)) {
        if(typeInfo::isListePtr(lhsV) and typeInfo::isListePtr(rhsV)) {
            if(Op != "+")
                irGenAide::LogCodeGenError("The only operation allowed between two entities is list concatenation");
            
            auto listConcat = std::make_unique<ListConcatenation>(lhsV, rhsV);
            return listConcat->codegen();
        }

        irGenAide::LogCodeGenError(
            "Your trying to perform a binary operation on two entities that are not both listes !!");
    }
    
    if(typeInfo::isReel(lhsV) and typeInfo::isReel(rhsV)) {
            if(Op == "+")
                return builder.CreateFAdd(lhsV, rhsV);

            if(Op == "-")
                return builder.CreateFSub(lhsV, rhsV);

            if(Op == "*")
                return builder.CreateFMul(lhsV, rhsV);
                
            if(Op == "/")
                // TODO check division by zero
                return builder.CreateFDiv(lhsV, rhsV);
            
            if(Op == "<")
                return builder.CreateFCmpOLT(lhsV, rhsV);
            
            if(Op == ">")
                return builder.CreateFCmpOGT(lhsV, rhsV);

            irGenAide::LogCodeGenError("Unkown Binary Operator << " + Op + " >> between two reels");
    }
}