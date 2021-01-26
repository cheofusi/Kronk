#include "Attributes.h"

// This file holds definitions for compiler attributes whose values are shared are shared by all compiler
// components

namespace Attr {

    bool ALLOW_DEBUG_INFO;
    bool INCLUDE_MODE; // compilation mode for the kronk file

    // used for reporting line at which error occurs
    size_t CurrentLexerLine;
    // stack for storing scopes
    std::vector<std::unique_ptr<Scope>> ScopeStack; 
    
    llvm::LLVMContext Context;
    std::shared_ptr<llvm::Module> MainModule = std::make_shared<llvm::Module>("Main", Context);
    IRBuilder<> Builder(Context);

    SMDiagnostic error;
    // module hold all kronk rutime functions
    std::unique_ptr<Module> KronkrtModule = getLazyIRFileModule("bin/rt/libkronkrt.bc", error, Context);


    const std::array<std::string, 3> BuiltinTypes = {"bool", "reel", "str"};
    // maps entity type strings to their signature names
    std::unordered_map<std::string, std::vector<std::string>> EntitySignatures;
    // maps entity type strings to their signature types.
    std::unordered_map<std::string, StructType*> EntityTypes; 


    const std::unordered_map<std::string, uint8_t> KronkOperators = {
        { "!", 100 },
        { "~", 100 },

        { "**", 90 },

        { "*", 80 },
        { "/", 80 },
        { "mod", 80 },

        { "+", 70 },
        { "-", 70 },

        { "<<", 60 },
        { ">>", 60 },

        { "<", 50 },
        { ">", 50 },
        { "<=", 50 },
        { ">=", 50 },

        { "==", 40 },
        { "!=", 40 },

        { "&", 30 },
        { "^", 28 },
        { "|", 26 },

        { "et", 20 },
        { "ou", 18 },

        { "=", 10 }
    };

}   // end of namespace Attr 


// Progress Logging
void LogProgress(std::string str) {
    if(Attr::ALLOW_DEBUG_INFO)
        std::cout << "[ Debug Info ]: " << str << std::endl;
}
