#include "Attributes.h"

/** This file holds definitions for compiler attributes whose values are shared are shared by all compiler
 * components, but defined in neither of them. 
**/

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

const std::array<std::string, 2> PrimitiveTypes = {"bool", "reel"};

const std::unordered_map<std::string, uint8_t> KronkOperators = {
    { "*", 50 },
    { "/", 50 },
    { "+", 40 },
    { "-", 40 },
    { "<", 30 },
    { ">", 30 },
    { "=", 10 }
};
