#include "Nodes.h"
#include "irGenAide.h"


Value* Assignment::codegen() {
    LogProgress("Creating assignment");

    // first inject the respective contexts to the left and right hand side be codegen 'ing them.
    lhs->injectCtx("Store");
    rhs->injectCtx("Load"); // this isn't necessary as this is the default context

    auto lhsV = lhs->codegen(); // always returns a pointer type like i1*, i64*, double*, { i64 }**, { i64 }*
    auto rhsV = rhs->codegen(); // only returns a pointer in case there is an entity.

    auto lhsTy = lhsV->getType()->getPointerElementType();
    auto rhsTy = rhsV->getType();
    

    if(lhsTy->isStructTy() or irGenAide::isEnttyPtr(lhsTy)) {
        // if lhsV points to an entity, then rhsV must also point to an entity of the same type as lhsV

        // this takes care of the special case where rhs is an Identifier Node that codegens an entity ptr
        // which ignores the ctx, giving types like { i64 }* instead of { i64 }**. Any call to lhsTy->isStrucTy() inside
        // this if block is dealing with this exception
        auto lvalueTy = (lhsTy->isStructTy()) ? lhsV->getType() : lhsTy;

        if(not irGenAide::isEnttyPtr(rhsV)) 
            return irGenAide::LogCodeGenError("Trying to assign a primitive to an entity");
        if(not irGenAide::isEqual(lvalueTy->getPointerElementType(), rhsTy->getPointerElementType()))
            return irGenAide::LogCodeGenError("Assignment operand types do not match");

        if(lhsTy->isStructTy()) {
            irGenAide::copyEntty(lhsV, rhsV);
            return lhsV;
        }
    
        AllocaInst* alloc = builder.CreateAlloca(lvalueTy->getPointerElementType());
        irGenAide::copyEntty(alloc, rhsV);
        builder.CreateStore(alloc, lhsV);
        return alloc;
    }
    
    // lhsV is a primitive type
    if(irGenAide::isEnttyPtr(rhsV))
        return irGenAide::LogCodeGenError("Trying to assign an entity to a primitive");

    if(lhsTy->isDoubleTy() and rhsTy->isIntegerTy()) {
        rhsV = builder.CreateCast(Instruction::SIToFP, rhsV, builder.getDoubleTy());
    }

    else if(lhsTy->isIntegerTy() and rhsTy->isDoubleTy()) {
        rhsV = builder.CreateCast(Instruction::FPToSI, rhsV, builder.getInt64Ty());
    }

    else if(not irGenAide::isEqual(lhsTy, rhsTy)) {
        return irGenAide::LogCodeGenError("Assignment operand types do not match");
    }

    builder.CreateStore(rhsV, lhsV);
    return rhsV;
}


Value* BinaryExpr::codegen() {
    /**
     * For +,-,/,* , if one of operands is a float then we cast all other non floats to floats and create a float BinOp.
     * For comparison operators, cast operands must be the same.
     * */

    if(Op == '=') {
        auto assn = std::make_unique<Assignment>(std::move(LHS), std::move(RHS));
        return assn->codegen();
    }
    
    Value* L = LHS->codegen();
    Value* R = RHS->codegen();

    if (irGenAide::isEnttyPtr(L) or irGenAide::isEnttyPtr(R)) {
        if(irGenAide::isListePtr(L) and irGenAide::isListePtr(R)) {
            if(Op != '+')
                return irGenAide::LogCodeGenError(
                                "The only operation allowed between two entities is list concatenation");
            auto listConcat = std::make_unique<ListConcatenation>(L, R);
            return listConcat->codegen();
        }
        return irGenAide::LogCodeGenError(
                        "Your trying to perform a binary operation on two entities that are not both listes !!");
    }
    
    if(L->getType()->isDoubleTy() || R->getType()->isDoubleTy()){
        L = builder.CreateCast(Instruction::SIToFP, L, Type::getDoubleTy(context));
        R = builder.CreateCast(Instruction::SIToFP, R, Type::getDoubleTy(context));
    }
    
    switch (Op) {
        case '+':
            if(L->getType()->isDoubleTy())
                return builder.CreateFAdd(L, R);
            return builder.CreateAdd(L, R);
        case '-':
            if(L->getType()->isDoubleTy())
                return builder.CreateFSub(L, R);
            return builder.CreateNSWSub(L, R);
        case '*':
            if(L->getType()->isDoubleTy())
                return builder.CreateFMul(L, R);
            return builder.CreateMul(L, R);
        case '/':
            // check division by zero
            if(L->getType()->isDoubleTy())
                return builder.CreateFDiv(L, R);
            return builder.CreateSDiv(L, R, "", true);
        case '<':
            if(L->getType()->isDoubleTy())
                return builder.CreateFCmpOLT(L, R);
            
            return builder.CreateICmpSLT(L, R);
        case '>':
            if(L->getType()->isDoubleTy())
                return builder.CreateFCmpOGT(L, R);
            return builder.CreateICmpSGT(L, R);
        default:
            return irGenAide::LogCodeGenError("Unkown Binary Operator " + std::string(&Op));
    }
}