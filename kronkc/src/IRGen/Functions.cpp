#include "Nodes.h"
#include "irGenAide.h"

std::array<std::string, 2> IOFunctions = {"afficher", "lire"};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool isIOFunction(Function* fn) {
    if(std::find(IOFunctions.begin(), IOFunctions.end(), fn->getName()) != IOFunctions.end()) {
        return true;
    }
    return false;
}

void prettyPrint();
void editArgs();
void replaceEnttyPtrs(std::vector<Value*>& values) {
    for(int i = 0; i < values.size(); ++i) {
        auto v = values[i];
        if(auto vTy = typeInfo::isEnttyPtr(v)) {
            std::string str = vTy->isLiteral() ? "liste" : vTy->getName();
            values[i] = builder.CreateGlobalStringPtr(str);
        }
    }
}


void formatArgs(std::vector<Value*>& values) {
    std::string strFormat;
    for(auto& v : values) {
        if(typeInfo::isBool(v)) {
            strFormat += 'b';

        }

        else if(typeInfo::isReel(v)) {
            strFormat += 'r';
        }

        else {
            strFormat += 's';
        }
    }

     values.insert(values.begin(), builder.CreateGlobalStringPtr(strFormat));
}


void matchArgTys(Function* fn,  std::vector<Value*>& values) {
    auto fnTy = fn->getFunctionType();
    // first match arg numbers
    if(fnTy->getNumParams() > values.size()) {
        irGenAide::LogCodeGenError("Too few arguments in the function call to << " + 
                                    std::string(fn->getName()) + " >>");
    }

    if(fnTy->getNumParams() < values.size()) {
        irGenAide::LogCodeGenError("Too many arguments in the function call to " +
                                    std::string(fn->getName()) + " >>");
    }

    // then match arg types
    for(int i = 0; i < values.size(); ++i) {
        auto v = values[i];
        auto paramTy = fnTy->getParamType(i);
        
        if(not typeInfo::isEqual(v->getType(), paramTy)) {

            irGenAide::LogCodeGenError("Type mismatch for argument #" + std::to_string(i+1) 
                                        + " in function call to << " + std::string(fn->getName()) + " >>");
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




Value* Prototype::codegen() {
    LogProgress("Creating Prototype for " + fnName);
    
    // TODO Check if a function with this name already exists

    std::vector<Type*> paramTys;
    for(auto& typeId: paramTypeIds) {
        Type* Ty = typeId->typegen();
        Ty = typeInfo::isEnttyPtr(Ty) ? Ty->getPointerTo() : Ty;
        paramTys.push_back(Ty);
    }

    // remember entities are passed around as pointers
    auto fnReturnTy = fnTypeId->typegen();
    fnReturnTy = typeInfo::isEnttyPtr(fnReturnTy) ? fnReturnTy->getPointerTo() : fnReturnTy;

    llvm::FunctionType* fnType;
    if(paramTypeIds.size()) {
        fnType = llvm::FunctionType::get(fnReturnTy, paramTys, false);
    }

    else {
        fnType = llvm::FunctionType::get(fnReturnTy, false);
    }

    llvm::Function* fn = llvm::Function::Create(fnType, llvm::GlobalValue::InternalLinkage, fnName, module.get());
    
    // Set names for all arguments so ir code is readable
    unsigned Idx = 0;
    for (auto& Arg : fn->args())
        Arg.setName(paramNames[Idx++]);

    return fn;
}


Value* FunctionDefn::codegen(){
    LogProgress("Creating Function definition");

    if(builder.GetInsertBlock()->getParent()->getName() != "main") {
        irGenAide::LogCodeGenError("kronk doesn't allow nesting of function definitions");
    }

    BasicBlock* PreFuncBlock = builder.GetInsertBlock();
    
    ScopeStack.push_back(std::make_unique<Scope>());

    auto fn = cast<Function>(prototype->codegen()); 
    BasicBlock *bb = BasicBlock::Create(context, "entry", fn);
    
    builder.SetInsertPoint(bb);

    ScopeStack.back()->returnValue = builder.CreateAlloca(fn->getReturnType(), nullptr, "returnValue"); 

    // set the name->address correspondence in the local symboltable for each argument
    for(auto& arg : fn->args()) { 
        // allocate memory on the stack to hold the arguments
        llvm::AllocaInst *alloca = builder.CreateAlloca(arg.getType());
        // store each argument in each allocated memory space 
        builder.CreateStore(static_cast<llvm::Value*>(&arg), alloca); 
        // we use the names set in the protottype declaration.
        ScopeStack.back()->SymbolTable[std::string(arg.getName())] = alloca; 
    }

    Body->codegen();
    
    auto fnExitBB = ScopeStack.back()->fnExitBB;
    fn->getBasicBlockList().push_back(fnExitBB);
    
    builder.CreateBr(fnExitBB);
    builder.SetInsertPoint(fnExitBB);
    builder.CreateRet(builder.CreateLoad(ScopeStack.back()->returnValue));
    
    if(not llvm::verifyFunction(*fn))
        irGenAide::LogCodeGenError("There's a problem with your function definition kronk can't figure out");
    
    ScopeStack.pop_back();
    // We should instead just return to main.
    builder.SetInsertPoint(PreFuncBlock); // WHY ARE WE DOING THIS?? Because Kronk has no main function. We can 
                                          // continue writing expressions after a function definition.
    return static_cast<Value*>(nullptr);

}


Value* ReturnStmt::codegen() {
    LogProgress("Generating Return Stmt");
    
    // All we do is store the return expression in the returnValue variable of this scope
    
    if(builder.GetInsertBlock()->getParent()->getName() == "main") {
        irGenAide::LogCodeGenError("return statements must only be in function definitions");
    }

    auto rvalue = returnExpr->codegen();
    auto lvalue = ScopeStack.back()->returnValue;

    auto rvalueTy = rvalue->getType();
    auto lvalueTy = lvalue->getType()->getPointerElementType();

    if(not typeInfo::isEqual(rvalueTy, lvalueTy)) {
        irGenAide::LogCodeGenError("Return value type does not correspond to function return type");
    }   

    builder.CreateStore(rvalue, lvalue);
    builder.CreateBr(ScopeStack.back()->fnExitBB);

    return static_cast<Value*>(nullptr);
}


Value* FunctionCallExpr::codegen() {
    LogProgress("Creating function call to " + callee);

    std::vector<Value*> ArgsV; // holds codegen for each argument
    
    auto fnExists = irGenAide::getFunction(callee);

    if(fnExists) {
        auto fn = *fnExists;
        for(auto& arg: Args) {
            auto argV = arg->codegen();
            ArgsV.push_back(argV);
        }

        // if the fn is an IO function, we don't need to match the argument types 
        if(isIOFunction(fn)) {
            // replace entity pointers with their names
            replaceEnttyPtrs(ArgsV);
            formatArgs(ArgsV);
            
            return builder.CreateCall(fn, ArgsV);
        }

        // match ArgsV against fn prototype
        matchArgTys(fn, ArgsV);
        return builder.CreateCall(fn, ArgsV);   
    }

    irGenAide::LogCodeGenError("The function << " + callee + " >> is not defined!!");

}