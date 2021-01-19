#ifndef _ATTRIBUTES_H_
#define _ATTRIBUTES_H_

#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IRReader/IRReader.h>

#include <memory>
#include <string>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>


using namespace llvm;

extern llvm::LLVMContext context;
extern std::shared_ptr<llvm::Module> module;
extern IRBuilder<> builder;

extern std::unique_ptr<Module> runtimeLibM;
 
extern std::unordered_map<std::string, std::vector<std::string>> EntitySignatures;
extern std::unordered_map<std::string, StructType*> EntityTypes; 

extern const std::array<std::string, 3> BuiltinTypes;
extern const std::unordered_map<std::string, uint8_t> KronkOperators;

// externs variables and functions used by ALL compiler components, and defined in just one of them
extern unsigned int currentLexerLine;
extern void LogProgress(std::string str);


#endif