#include "Driver.h"
#include "Parser.h"
#include "Names.h"


CompileDriver::CompileDriver(std::string&& inputFile) {
    // this construtor is used by Main.cpp i.e for main kronk file

    this->inputFile = fs::path(inputFile);
}


CompileDriver::CompileDriver(std::vector<std::string>&& inputFile) {
    // this constructor is used by IncludeStmt

    for(auto& path_element : inputFile) {
        this->inputFile.append(path_element);
    }

    this->inputFile.replace_extension("krk");
}


std::string CompileDriver::compileInputFile(std::string&& moduleId) {
    // tries to compile this->inputFile with moduleId. returns the moauleId on success

    if(not inputFileExists()) {
        auto errStr = "The file '" + inputFile.string() + "' doesn't exist !! ";
        LogImportError(errStr);
    }
    
    // construct the module for this file. If the caller didn't provide a name for the module, use the file name
    // without the extension
    if(moduleId.empty())
        moduleId = inputFile.stem().string();

    // check if the file has already been referenced (i.e it has already been compiled or it is somewhere in the
    // Attr::ModuleStack) 
    auto [it, not_referenced] = Attr::FileToModuleIdMap.insert(std::make_pair(inputFile, moduleId));
    if(not not_referenced) { // a double negative...I know
        // file has been referenced. so just point moduleId to the primordial moduleId used to reference the file
        // and return. But before that make sure we're not dealing with a circular dependency ex in A.krk
        // we have `inclu B`, and in B.krk we have `inclu A. This is a fatal fucking error

        // the primordial/original/first moduleId used to compile inputFile
        auto prModuleId = Attr::FileToModuleIdMap[inputFile];

        if(Attr::INCLUDE_MODE and names::createsCircularDep(prModuleId)) {
            LogImportError("Circular dependency detected!! Aborting..");
        }  
        
        Attr::DuplicateModuleMap[moduleId] = prModuleId;
        return moduleId;
    }
    
    // the file has not yet been compiled. so moduleId will be used to compile it
    Attr::ThisModule = std::make_unique<llvm::Module>(moduleId, Attr::Context);
    Attr::FileToModuleIdMap[inputFile] = moduleId;
    
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    Attr::ThisModule->setTargetTriple(llvm::sys::getDefaultTargetTriple());

    // then our mainfunction
    auto mainFnTy = llvm::FunctionType::get(Attr::Builder.getInt32Ty(), false);
    auto mainFn  = Function::Create(mainFnTy, llvm::GlobalValue::ExternalLinkage, "main", Attr::ThisModule.get());
    
    auto mainblock = BasicBlock::Create(Attr::Context, "ProgramEntry", mainFn);  
    auto mainScope = std::make_unique<Scope>();
    Attr::Builder.SetInsertPoint(mainblock);
    Attr::ScopeStack.push_back(std::move(mainScope));
    
    driver();
   
    Attr::ScopeStack.pop_back(); // pop the last scope
    
    // The program termination block
    auto endProgramblock = BasicBlock::Create(Attr::Context, "ProgramExit", mainFn);
    Attr::Builder.CreateBr(endProgramblock);
    Attr::Builder.SetInsertPoint(endProgramblock);
    Attr::Builder.CreateRet(Attr::Builder.getInt32(0)); // On termination our program always returns zero

    llvm::verifyFunction(*mainFn);

    if(Attr::INCLUDE_MODE) {
        // removes the main function if this module was included (i.e compiled in include mode)
        Attr::ThisModule->getFunction("main")->eraseFromParent();
    }

    registerModule();
    
    return moduleId;
}


void CompileDriver::registerModule() {
    // Add the current module (which was just compiled) into the module map. 

    auto moduleAttrs = std::make_unique<ModuleAttrs>(std::move(Attr::ThisModule));

    moduleAttrs->EntitySignatures = std::move(Attr::EntitySignatures);
    moduleAttrs->EntityTypes = std::move(Attr::EntityTypes);

    // the module names are guaranteed to be unique
    Attr::ModuleMap.insert( std::make_pair(moduleAttrs->TheModule->getModuleIdentifier(), std::move(moduleAttrs) )); 
}


void CompileDriver::driver() {
    // This function handles parsing and ir code gen for every statement;

    auto parser = Parser::CreateParser(std::move(inputFile));
    parser->moveToNextToken(); // read the first token from the input file

    while (true) {
        if(parser->currToken() == Token::END_OF_FILE) {
            LogProgress("Compile Sucess!!");
            return;
        }
        
        auto stmt_ast = parser->ParseStmt(true);
        if(stmt_ast) { 
            stmt_ast->codegen();
        }

        Attr::IRGenLineOffset = 0;    
    }
}


bool CompileDriver::inputFileExists() {
    if(inputFile.string().find("/") != std::string::npos) {
        // a relative path from the current directory was provided to the file 
        if(fs::exists(inputFile) and fs::is_regular_file(inputFile)) {
            inputFile = fs::absolute(inputFile);
            return true;
        }
    }
    
    else {
        // the file's full or relative path from the pwd was not specified. so search for it recursively
        // from the pwd and replace this->inputFile with the first match.

        for (auto& file : fs::recursive_directory_iterator(
                        fs::current_path(), fs::directory_options(fileSearchOptions))) {
            
            if(file.is_regular_file()) {
                if(file.path().filename() == inputFile) {
                    inputFile = file.path();
                    return true;
                }
            }
        }
    }

    return false;
}


LLVM_ATTRIBUTE_NORETURN
void CompileDriver::LogImportError(std::string errMsg) {
    outs()  << "Module Include Error in "
            << names::getModuleFile().filename() << '\n'
            << "[Line "
            << Attr::CurrentLexerLine  
            << "]:  " 
            << errMsg 
            << '\n';
    
    exit(EXIT_FAILURE);
}