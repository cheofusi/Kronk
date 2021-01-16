#include "Parser.h"


// Stack for storing scopes 
std::vector<std::unique_ptr<Scope>> ScopeStack;

// Progress Logging
void LogProgress(std::string str) {
    std::cout << "[ Debug Info ]: " << str << std::endl;
}


void ParseDriver() {
    // This function handles parsing after every statement;

    auto parser = Parser::CreateParser();
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


int main() {
    std::cout <<"Hey there"<<std::endl;
    
    InitializeNativeTarget();

    // then our mainfunction
    FunctionType *mainFnTy = llvm::FunctionType::get(builder.getInt32Ty(), false);
    Function* mainFn  = Function::Create(mainFnTy, llvm::GlobalValue::ExternalLinkage, "main", module.get());
    
    BasicBlock* mainblock = BasicBlock::Create(context, "ProgramEntry", mainFn);  
    std::unique_ptr<Scope> mainScope = std::make_unique<Scope>();
    builder.SetInsertPoint(mainblock);
    ScopeStack.push_back(std::move(mainScope));
    
    ParseDriver();

    ScopeStack.pop_back(); // pop the last scope
    
    // The program termination block
    BasicBlock* endProgramblock = BasicBlock::Create(context, "ProgramExit", mainFn);
    builder.CreateBr(endProgramblock);
    builder.SetInsertPoint(endProgramblock);
    builder.CreateRet(builder.getInt32(0)); // On termination our program always returns zero

    llvm::verifyFunction(*mainFn);

    module->print(llvm::errs(), nullptr);
}
