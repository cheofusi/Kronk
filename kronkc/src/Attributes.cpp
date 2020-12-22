#include "Attributes.h"

/** This file holds definitions for compiler attributes whose values are shared are shared by all compiler
 * components, but defined in neither of them. 
**/

llvm::LLVMContext context;
std::shared_ptr<llvm::Module> module = std::make_shared<llvm::Module>("Main", context);
IRBuilder<> builder(context);
 

// primitive types 
std::vector<std::string> PrimitiveTypes = {"entier", "reel"}; 
// maps entity types(user defined types) to their signature names
std::unordered_map<std::string, std::vector<std::string>> EntitySignatures;
// maps entity types to their signature types.
std::unordered_map<std::string, StructType*> EntityTypes; 
