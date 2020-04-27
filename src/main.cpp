#include "Parser.h"

std::map<char, int> BinaryOperatorPrecedence;
int currentToken;

void initBinOpPrecedences();

Function *createFunction(std::string Name){
    FunctionType *funcType = llvm::FunctionType::get(builder.getInt32Ty(), false);
    Function *fooFunc = Function::Create(funcType, llvm::GlobalValue::InternalLinkage, Name, module.get());
    return fooFunc;
}

void parseManager(){
    // This function handles parsing after every statement;
    moveToNextToken(); // read the first token from input file
    while (true){
        switch (currentToken){
            case tok_DOUBLE:
            case tok_INT:
                if(auto varD = ParseVariableDeclaration()){
                    if(auto varDIR = varD->codegen()){
                        std::cout << "Read Declaration" << std::endl;
                        moveToNextToken(); // eat ";" we should check here
                        }
                }
                break;

            case tok_function:
                if (auto varF = ParseFunctionDefinition()){
                    if(auto varFR = varF->codegen()){
                        std::cout << "Read Function Definition" << std::endl;
                    }
                    else  std::cout << "Failed to Read Function Definition" << std::endl;
                    
                }
                break;

            case tok_eof:
                std::cout<< "Reached end of file" << std::endl;
                return;

            case tok_identifier:
            case tok_if:
            case tok_whileLoop:
                if (auto varE = ParseExpression()){
                    if(varE->codegen()){
                        std::cout << "Read an Expression" << std::endl;
                        if(currentToken == ';') moveToNextToken();
                    }
                }
                break;

            default:
                break;
        }
    }
    
}

int main(){
    std::cout <<"Hey there"<<std::endl;
    initBinOpPrecedences();
    
    // first declare all syscalls
    llvm::FunctionType *printFnType = llvm::FunctionType::get(builder.getInt32Ty(), true); 
    llvm::Function* printfn = llvm::Function::Create(printFnType, llvm::GlobalValue::ExternalLinkage, "printf", module.get());

    llvm::FunctionType *readFnType = llvm::FunctionType::get(builder.getInt32Ty(), true); 
    llvm::Function* readfn = llvm::Function::Create(readFnType, llvm::GlobalValue::ExternalLinkage, "scanf", module.get());


    // then our mainfunction
    Function* mainFunction = createFunction("main");
    BasicBlock* mainblock = BasicBlock::Create(context, "entrypoint", mainFunction);  
    std::shared_ptr<Scope> mainScope = std::make_shared<Scope>(mainblock);
    builder.SetInsertPoint(mainblock);
    scopeStack.push_back(mainScope) ;

    parseManager();
 
    scopeStack.pop_back(); // pop the last lexical scope

    // The program termination block
    BasicBlock* endProgramblock = BasicBlock::Create(context, "ProgramExit", mainFunction);
    builder.CreateBr(endProgramblock);
    builder.SetInsertPoint(endProgramblock);
    builder.CreateRet(ConstantInt::get(Type::getInt32Ty(context), 0)); // On termination our program always returns zero

    llvm::verifyFunction(*mainFunction);

    module->print(llvm::errs(), nullptr);
}



void initBinOpPrecedences(){
    BinaryOperatorPrecedence['<'] = 10;
    BinaryOperatorPrecedence['>'] = 10;
    BinaryOperatorPrecedence['+'] = 20;
    BinaryOperatorPrecedence['-'] = 20; 
    BinaryOperatorPrecedence['*'] = 40;
    BinaryOperatorPrecedence['/'] = 40;
}