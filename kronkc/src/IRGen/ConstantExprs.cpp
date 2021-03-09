#include "IRGenAide.h"
#include "Nodes.h"


Value* BooleanLiteral::codegen() {
	if (value == "vrai") {
		return Attr::Builder.getTrue();
	}

	return Attr::Builder.getFalse();
}


Value* NumericLiteral::codegen() { return ConstantFP::get(Attr::Builder.getDoubleTy(), APFloat(value)); }


Value* Identifier::codegen() {
	LogProgress("Accessing value of identifier: " + name);

	if (Attr::ScopeStack.back()->SymbolTable.count(name) == 0)
		irGenAide::LogCodeGenError("Unknown Identifier << " + name + " >>");

	Value* ptr = Attr::ScopeStack.back()->SymbolTable[name];

	if (types::isEnttyPtr(ptr)) {
		// this is the one exception to the ctx rule. it is so because the symbol table stores pointers
		// to entities different than entities containing entities do. So it doesn't make sense to 'load'
		// from the symbol table
		return ptr;
	}
	return (ctx) ? Attr::Builder.CreateLoad(ptr) : ptr;
}

bool Identifier::injectCtx(bool ctx) {
	this->ctx = ctx;
	return true;
}