#ifndef _IRGen_H
#define _IRGen_H

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>


namespace fs = std::filesystem;

using namespace llvm;


class Node {
public:
	// ctx is 1 for load (default context), 0 for store. This has nothing to do with llvm's context
	bool ctx = 1;

	// Some Node classes have no context e.x Declr, FunctionDefn, CompoundStmt. While some can only allow
	// a load context e.x ListSlice, FunctionCallExpr. Node classes that can have a load or store ctx
	// (i.e Nodes allowed on the left hand side of an assigment) override this method.
	virtual bool injectCtx(bool ctx) { return false; }

	virtual std::unique_ptr<Node> clone(){};
	virtual Value* codegen(){};
	virtual ~Node() {}
};


class Scope {
public:
	// symbol table for variables of this scope
	std::unordered_map<std::string, Value*> SymbolTable;

	// record of all heap allocations for this scope
	std::vector<Value*> HeapAllocas;

	// last block in a function definition.
	BasicBlock* fnExitBB;
	// holds return value for function. Helps to handle multiple return values in function definition
	Value* returnValue = nullptr;
};

#endif