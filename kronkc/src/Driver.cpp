#include "Driver.h"
#include "Parser.h"
#include "IRGen.h"

#include <filesystem>


// Stack for storing scopes 
std::vector<std::unique_ptr<Scope>> ScopeStack; 


void CompilerDriver::compileInputFile() {
    if(not fileExists()) {
        std::cout << "Input file << " + inputFile + " >> doesn't exist" << std::endl;
        exit(EXIT_FAILURE);
    }

    InitializeNativeTarget();

    // then our mainfunction
    auto mainFnTy = llvm::FunctionType::get(builder.getInt32Ty(), false);
    auto mainFn  = Function::Create(mainFnTy, llvm::GlobalValue::ExternalLinkage, "main", module.get());
    
    auto mainblock = BasicBlock::Create(context, "ProgramEntry", mainFn);  
    auto mainScope = std::make_unique<Scope>();
    builder.SetInsertPoint(mainblock);
    ScopeStack.push_back(std::move(mainScope));
    
    driver();

    ScopeStack.pop_back(); // pop the last scope
    
    // The program termination block
    auto endProgramblock = BasicBlock::Create(context, "ProgramExit", mainFn);
    builder.CreateBr(endProgramblock);
    builder.SetInsertPoint(endProgramblock);
    builder.CreateRet(builder.getInt32(0)); // On termination our program always returns zero

    llvm::verifyFunction(*mainFn);

    module->print(llvm::errs(), nullptr);
    

}


void CompilerDriver::driver() {
    // This function handles parsing and ir code gen for every statement;

    auto parser = Parser::CreateParser(inputFile);
    parser->moveToNextToken(); // read the first token from the input file

    while (true) {
        if(parser->currToken() == Token::END_OF_FILE) {
            LogProgress("Compile Sucess!!");
            return;
        }
        
        auto stmt_ast = parser->ParseStmt();
        stmt_ast->codegen();
    }
}


bool CompilerDriver::fileExists() {
    namespace fs = std::filesystem;
    return fs::exists(inputFile) and fs::is_regular_file(inputFile);
}