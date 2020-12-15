#include<llvm/Support/TargetSelect.h>
#include<llvm/ExecutionEngine/MCJIT.h>
#include<llvm/Bitcode/BitcodeReader.h>
#include<llvm/Support/ErrorOr.h>
#include<llvm/Support/MemoryBuffer.h>
#include<llvm/Support/FileSystem.h>
#include<llvm/Support/ManagedStatic.h>


#include "Parser.h"

std::unordered_map<char, int> BinaryOperatorPrecedence;
Token currentToken;
extern std::shared_ptr<llvm::Module> module;

void initExternalFunctionDeclarations();

// Progress Loggin
void LogProgress(std::string str){
    std::cout << "[ Debug Info ]: " << str << std::endl;
}


void ParseDriver() {
    // This function handles parsing after every statement;
    moveToNextToken(); // read the first token from input file
    while (true) {
        if(currentToken == Token::END_OF_FILE) {
            LogProgress("Compile Sucess!!");
            return;
        }
        
        auto stmt_ast = ParseStmt();
        stmt_ast->codegen();
    }
    
}

int main() {
    std::cout <<"Hey there"<<std::endl;
    
    InitializeNativeTarget();
    //InitializeNativeTargetAsmPrinter();
    //InitializeNativeTargetAsmParser();
    
    // std::string ErrorMessage;
    // auto buffer = MemoryBuffer::getFile("./sum.bc");
    // if (auto error = buffer.getError()) {
    //     errs() << error.message();
    //     return -1;
    // }
    // MemoryBufferRef bufferRef(*buffer.get());
    // auto M = parseBitcodeFile(bufferRef, context);
    // if (!M) {
    //     errs() << M.takeError() << "\n";
    //     return -1;
    // }
    // std::unique_ptr<ExecutionEngine> EE;
    // //EE.reset(EngineBuilder(M.get()).create());
    // module->setDataLayout("e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128");
    // module->setTargetTriple("x86_64-pc-linux-gnu");
    

    initExternalFunctionDeclarations();
    // then our mainfunction
    FunctionType *mainFuncType = llvm::FunctionType::get(builder.getInt32Ty(), false);
    Function* mainFunction  = Function::Create(mainFuncType, llvm::GlobalValue::ExternalLinkage, "main", module.get());

    BasicBlock* mainblock = BasicBlock::Create(context, "entrypoint", mainFunction);  
    std::shared_ptr<Scope> mainScope = std::make_shared<Scope>();
    builder.SetInsertPoint(mainblock);
    ScopeStack.push_back(mainScope) ;

    ParseDriver();
 
    ScopeStack.pop_back(); // pop the last lexical scope

    // The program termination block
    BasicBlock* endProgramblock = BasicBlock::Create(context, "ProgramExit", mainFunction);
    builder.CreateBr(endProgramblock);
    builder.SetInsertPoint(endProgramblock);
    builder.CreateRet(ConstantInt::get(Type::getInt32Ty(context), 0)); // On termination our program always returns zero

    llvm::verifyFunction(*mainFunction);
    builder.CreateGlobalStringPtr("Hey ya!");
    module->print(llvm::errs(), nullptr);
}


void initExternalFunctionDeclarations() {
    // declaration of all syscalls
    llvm::Function::Create(FunctionType::get(builder.getInt32Ty(), true), 
                            GlobalValue::ExternalLinkage, "printf", module.get());

    llvm::Function::Create(FunctionType::get(builder.getInt32Ty(),  builder.getInt8PtrTy(), false), 
                            GlobalValue::ExternalLinkage, "_kronk_runtime_error", module.get());

    llvm::Function::Create(FunctionType::get(builder.getVoidTy(), std::vector<Type*>{ builder.getInt8PtrTy(),
                                                                                    builder.getInt8PtrTy(),
                                                                                    builder.getInt64Ty() },
                                                                                    false ), 
                            GlobalValue::ExternalLinkage, "_kronk_memcpy", module.get());

    llvm::Function::Create(FunctionType::get(builder.getInt1Ty(), std::vector<Type*>(2, builder.getInt64Ty()), false), 
                            GlobalValue::ExternalLinkage, "_kronk_list_index_check", module.get());
    
    llvm::Function::Create(FunctionType::get(builder.getInt1Ty(), std::vector<Type*>{ builder.getInt64Ty(),
                                                                                    builder.getInt64Ty(),
                                                                                    builder.getInt64Ty() },
                                                                                    false ), 
                            GlobalValue::ExternalLinkage, "_kronk_list_splice_check", module.get());
        
    llvm::Function::Create(FunctionType::get(builder.getInt64Ty(), std::vector<Type*>{ builder.getInt64Ty(),
                                                                                    builder.getInt64Ty() },
                                                                                    false ), 
                            GlobalValue::ExternalLinkage, "_kronk_list_fix_idx", module.get());
}
