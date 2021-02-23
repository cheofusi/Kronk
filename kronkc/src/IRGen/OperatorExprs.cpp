#include "Nodes.h"
#include "IRGenAide.h"
#include "Names.h"



static const std::unordered_map<std::string, uint8_t> OpIds = {
    { "non", 0 },
    { "~", 1 },

    { "**", 2 },

    { "*", 3 },
    { "/", 4 },
    { "mod", 5 },

    { "+", 6 },
    { "-", 7 },

    { "<<", 8 },
    { ">>", 9 },

    { "&", 10 },
    { "^", 11 },
    { "|", 12 },

    { "<", 13 },
    { ">", 14 },
    { "<=", 15 },
    { ">=", 16 },

    { "==", 17 },
    { "!=", 18 },


    { "et", 19 },
    { "ou", 20 },

    { "=", 21 }
};


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
    

    if(lhsTy->isStructTy() or types::isEnttyPtr(lhsTy)) {
        // if lhsV points to an entity, then rhsV must also point to an entity of the same type as lhsV

        // this takes care of the special case where rhs is an Identifier Node that codegens an entity ptr
        // which ignores the ctx, giving types like { double }* instead of { double }**. Any call to lhsTy->isStrucTy() 
        // inside this if block is dealing with this exception
        auto lvalueTy = (lhsTy->isStructTy()) ? lhsV->getType() : lhsTy;

        if(not types::isEnttyPtr(rhsV)) 
            irGenAide::LogCodeGenError("Trying to assign a primitive to an entity");

        if(not types::isEqual(lvalueTy->getPointerElementType(), rhsTy->getPointerElementType()))
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
        if(not types::isStringPtr(rhsV))
            irGenAide::LogCodeGenError("Trying to replace a character of a string with a non-string value");
        
        // TODO: insert runtime check to see if rhsV is a 1 character string. 
        auto charPtr = Attr::Builder.CreateLoad(irGenAide::getGEPAt(rhsV, irGenAide::getConstantInt(1)));
        auto character = Attr::Builder.CreateLoad(charPtr);
        Attr::Builder.CreateStore(character, lhsV);
        return rhsV;
    }
    
    // lhsV & rhsV are both primitives 

    if(not types::isEqual(lhsTy, rhsTy)) {
        irGenAide::LogCodeGenError("Operand types of assignment do not match");
    }

    Attr::Builder.CreateStore(rhsV, lhsV);
    return rhsV;
}


Value* UnaryExpr::codegen() {

    auto OpId = OpIds.at(Op);

    Value* rhsV = rhs->codegen();

    if(types::isBool(rhsV)) {
        if(not OpId) // OpId == 0 
            return Attr::Builder.CreateNot(rhsV);

        irGenAide::LogCodeGenError("Undefined unary operator << " + Op + " >> for a boolean");
    }

    if(types::isReel(rhsV)) {
        switch (OpId) {
            case 7:
                return Attr::Builder.CreateFNeg(rhsV);
        
            case 1:
                rhsV = irGenAide::DoubletoIntCast(rhsV);
                return irGenAide::InttoDoubleCast(Attr::Builder.CreateNot(rhsV));

            default:
                irGenAide::LogCodeGenError("Undefined unary operator << " + Op + " >> for a real number");
        }
    }

    irGenAide::LogCodeGenError("Incompatible operand type for the unary operator << " + Op + " >>");
}


Value* BinaryExpr::codegen() {

    auto OpId = OpIds.at(Op);

    if(OpId == 21) {
        auto assn = std::make_unique<Assignment>(std::move(lhs), std::move(rhs));
        return assn->codegen();
    }
    
    Value* lhsV = lhs->codegen();
    Value* rhsV = rhs->codegen();
    
    if(types::isEnttyPtr(lhsV) or types::isEnttyPtr(rhsV)) {
        if(types::isListePtr(lhsV) and types::isListePtr(rhsV)) {
            if(OpId != 6)
                irGenAide::LogCodeGenError(
                        "The only binary operation allowed between two entities is list concatenation");
            
            auto listConcat = std::make_unique<ListConcatenation>(lhsV, rhsV);
            return listConcat->codegen();
        }

        irGenAide::LogCodeGenError(
            "Your trying to perform a binary operation on two entities that are not both listes !!");
    }


    if(types::isBool(lhsV) and types::isBool(rhsV)) {
        switch (OpId) {
            case 17:
                return Attr::Builder.CreateICmpEQ(lhsV, rhsV);
            
            case 18:
                return Attr::Builder.CreateICmpNE(lhsV, rhsV);

            case 19:
                return Attr::Builder.CreateAnd(lhsV, rhsV);
            
            case 20:
                return Attr::Builder.CreateOr(lhsV, rhsV);
            
            default:
                irGenAide::LogCodeGenError("Undefined binary operator << " + Op + " >> between two booleans");
        }

    }
    
    if(types::isReel(lhsV) and types::isReel(rhsV)) {
            Value* lhsIntV = irGenAide::DoubletoIntCast(lhsV);
            Value* rhsIntV = irGenAide::DoubletoIntCast(rhsV);

            switch (OpId) {
                case 2:
                    // call runtime exponent function
                    return Attr::Builder.CreateCall(irGenAide::getRtModuleFn("_kmath_puiss"),
                                                    {lhsV, rhsV});
                
                case 3:
                    return Attr::Builder.CreateFMul(lhsV, rhsV);
                    
                case 4:
                    // check division by zero
                    irGenAide::emitRtCheck("_kronk_zero_div_check", {rhsV});
                    return Attr::Builder.CreateFDiv(lhsV, rhsV);

                case 5: 
                    return Attr::Builder.CreateFRem(lhsV, rhsV);

                case 6:
                    return Attr::Builder.CreateFAdd(lhsV, rhsV);

                case 7:
                    return Attr::Builder.CreateFSub(lhsV, rhsV);

                case 8:
                    return irGenAide::InttoDoubleCast(Attr::Builder.CreateShl(lhsIntV, rhsIntV));

                case 9:
                    return irGenAide::InttoDoubleCast(Attr::Builder.CreateAShr(lhsIntV, rhsIntV));

                case 10:
                    return irGenAide::InttoDoubleCast(Attr::Builder.CreateAnd(lhsIntV, rhsIntV));
                
                case 11:
                    return irGenAide::InttoDoubleCast(Attr::Builder.CreateXor(lhsIntV, rhsIntV));
                
                case 12:
                    return irGenAide::InttoDoubleCast(Attr::Builder.CreateOr(lhsIntV, rhsIntV));
                
                case 13:
                    return Attr::Builder.CreateFCmpOLT(lhsV, rhsV);
                
                case 14:
                    return Attr::Builder.CreateFCmpOGT(lhsV, rhsV);

                case 15:
                    return Attr::Builder.CreateFCmpOLE(lhsV, rhsV);

                case 16:
                    return Attr::Builder.CreateFCmpOGE(lhsV, rhsV);
                
                case 17:
                    return Attr::Builder.CreateFCmpOEQ(lhsV, rhsV);
                
                case 18:
                    return Attr::Builder.CreateFCmpONE(lhsV, rhsV);
                
                default:
                    irGenAide::LogCodeGenError("Undefined binary operator << " + Op + " >> between two real numbers");
            }
    }

    irGenAide::LogCodeGenError("Incompatible operand types for the binary operator << " + Op + " >>");
}