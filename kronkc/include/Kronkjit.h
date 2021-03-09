#ifndef _KRONK_JIT_H
#define _KRONK_JIT_H

#include "Attributes.h"

class Kronkjit {
	std::unique_ptr<llvm::Module> MainModule;

	void jitError();

public:
	void LinkAndOptimize();
	int runOrcLazyJIT();

	Kronkjit() : MainModule(std::make_unique<llvm::Module>("", Attr::Context)) {}
};


#endif