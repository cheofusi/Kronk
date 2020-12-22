#include "Nodes.h"
#include "irGenAide.h"

SMDiagnostic error;
// module hold all kronk rutime functions
std::unique_ptr<Module> runtimeLibM = getLazyIRFileModule("bin/libkronkrt.bc", error, context);



Value* Prototype::codegen() {
    LogProgress("Creating Prototype for " + funcName->name);
    std::vector<Type*> ArgTypes;
    for(auto& type: argTypes){
        Type* t = builder.getInt64Ty();//typeOf(type->name);
        ArgTypes.push_back(t);
    }
    llvm::FunctionType *FnType = llvm::FunctionType::get(builder.getInt64Ty(), ArgTypes, false); //typeOf(returnType->name)
    llvm::Function* fn = llvm::Function::Create(FnType, llvm::GlobalValue::InternalLinkage, funcName->name, module.get());

    // Set names for all arguments so ir code is readable
    unsigned Idx = 0;
    for (auto &Arg : fn->args())
        Arg.setName(argNames[Idx++]->name);

    return fn;
}


Value* FunctionDefn::codegen(){
    BasicBlock* PreFuncBlock = builder.GetInsertBlock();
    LogProgress("Creating Function definition");
    if(!prototype->codegen())
        return nullptr;

    Function* f = module->getFunction(prototype->funcName->name);
    BasicBlock *bb = BasicBlock::Create(context, "entry", f);
    ScopeStack.push_back(std::make_unique<Scope>());
    
    builder.SetInsertPoint(bb);

    ScopeStack.back()->returnValue = builder.CreateAlloca(f->getReturnType(), nullptr, "returnValue"); 

    for(auto &arg : f->args()){ // set the name->address correspondence in the local symboltable for each argument
        llvm::AllocaInst *alloca = builder.CreateAlloca(arg.getType(), 0, ""); // allocate memory on the stack to hold the arguments
        builder.CreateStore(static_cast<llvm::Value*>(&arg), alloca); // store each argument in each allocated memory space
        ScopeStack.back()->SymbolTable[std::string(arg.getName())] = alloca; // we use the names set in the protottype declaration.
    }
    for(auto& s : Body){
        s->codegen();
    }
    builder.CreateRet(builder.CreateLoad(ScopeStack.back()->returnValue));

    if(!llvm::verifyFunction(*f))
        return irGenAide::LogCodeGenError("There's a problem with your function definition kronk can't figure out");
    ScopeStack.pop_back();
    // We should instead just return to main.
    builder.SetInsertPoint(PreFuncBlock); // WHY ARE WE DOING THIS?? Because Kronk has no main function. We can 
                                          // continue writing expressions after a function definition.
    return f;

}


Value* ReturnExpr::codegen() {
    LogProgress("Generating Return Stmt");
    Value* rvalue = returnExpr->codegen();
    Value* lvalue = ScopeStack.back()->returnValue;

    if(rvalue->getType()->getPointerTo() !=  lvalue->getType()){ // if lvalue type isneq to rvalue type, then try to cast rvalue type to lvalue type
        if(lvalue->getType()->getPointerElementType()->isDoubleTy() && rvalue->getType()->isIntegerTy()) // cast int to double  
            rvalue = builder.CreateCast(Instruction::SIToFP, rvalue, Type::getDoubleTy(context));
        else if(lvalue->getType()->getPointerElementType()->isIntegerTy() && rvalue->getType()->isDoubleTy())
            rvalue = builder.CreateCast(Instruction::FPToSI, rvalue, Type::getInt32Ty(context));
        else
            return irGenAide::LogCodeGenError("Return value does not correspond to function return type");
        
    }
    return builder.CreateStore(rvalue, lvalue);
}


Value* FunctionCallNode::codegen() {
    LogProgress("Creating function call to " + Callee->name);
    std::vector<Value*> ArgsV; // holds codegen IR for each argument
    std::string StringFormat; // holds string formatting for all arguments
    // we need to verify the type of the args
    if(Callee->name == "afficher") {
        for(auto& arg : Args){ // add a genHelper that does this work
            if(auto v = arg->codegen()){ 
                if(v->getType()->isDoubleTy())
                    StringFormat += "%.4f "; // we treat lire and afficher differently because of this
                else if(v->getType()->isIntegerTy())
                    StringFormat += "%d ";
                ArgsV.push_back(v);
            }
        }
        StringFormat += "\n";
        ArgsV.insert(ArgsV.begin(), builder.CreateGlobalStringPtr(StringFormat));

        auto printfF = module->getOrInsertFunction("printf", FunctionType::get(builder.getInt32Ty(), true));
        auto pfty = printfF.getFunctionType();
        return builder.CreateCall(printfF, ArgsV, "printfCall");
    }
    
    if(Callee->name == "lire"){
        for(auto& arg : Args){ // ar should always be an identifier.
            Identifier* id = dynamic_cast<Identifier*>(arg.get());
            if(auto v = arg->codegen()){ 
                if(v->getType()->isDoubleTy())
                    StringFormat += "%lf "; // 
                else if(v->getType()->isIntegerTy())
                    StringFormat += " %d";
                ArgsV.push_back(ScopeStack.back()->SymbolTable[id->name]);
            }else return nullptr;
        }
        ArgsV.insert(ArgsV.begin(), builder.CreateGlobalStringPtr(StringFormat));
        return builder.CreateCall(module->getFunction("scanf"), ArgsV, "scanfCall");
    }
    
    for(auto& arg : Args){
        if(auto v = arg->codegen())
            ArgsV.push_back(v);
        else return nullptr;
    }
    return builder.CreateCall(module->getFunction(Callee->name), ArgsV, "f_call");
}