#ifndef _ATTRIBUTES_H_
#define _ATTRIBUTES_H_

#include "IRGen.h"

//using namespace llvm;


namespace Attr {
    extern bool ALLOW_DEBUG_INFO;
    extern bool INCLUDE_MODE;

    extern size_t CurrentLexerLine;
    extern std::vector<std::unique_ptr<Scope>> ScopeStack; 

    extern llvm::LLVMContext Context;
    extern std::shared_ptr<llvm::Module> MainModule;
    extern IRBuilder<> Builder;

    extern std::unique_ptr<Module> KronkrtModule;
    
    extern const std::array<std::string, 3> BuiltinTypes;
    extern std::unordered_map<std::string, std::vector<std::string>> EntitySignatures;
    extern std::unordered_map<std::string, StructType*> EntityTypes; 

    extern const std::unordered_map<std::string, uint8_t> KronkOperators;

} // end of Attr namespace


void LogProgress(std::string str);


#endif