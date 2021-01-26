#include "Attributes.h"

// This file holds definitions for compiler attributes whose values are shared are shared by all compiler
// components


bool ALLOW_DEBUG_INFO;
bool INCLUDE_MODE; // compilation mode for the kronk file

// Progress Logging
void LogProgress(std::string str) {
    if(ALLOW_DEBUG_INFO)
        std::cout << "[ Debug Info ]: " << str << std::endl;
}


llvm::LLVMContext context;
std::shared_ptr<llvm::Module> module = std::make_shared<llvm::Module>("Main", context);
IRBuilder<> builder(context);

SMDiagnostic error;
// module hold all kronk rutime functions
std::unique_ptr<Module> runtimeLibM = getLazyIRFileModule("bin/rt/libkronkrt.bc", error, context);


// maps entity type strings to their signature names
std::unordered_map<std::string, std::vector<std::string>> EntitySignatures;
// maps entity type strings to their signature types.
std::unordered_map<std::string, StructType*> EntityTypes; 

const std::array<std::string, 3> BuiltinTypes = {"bool", "reel", "str"};

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
