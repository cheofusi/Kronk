#include "IRGenAide.h"
#include "Names.h"
#include "Nodes.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void editArgsForPrint(std::vector<Value*>& values) {
	std::string strFormat;
	for (int i = 0; i < values.size(); ++i) {
		auto v = values[i];

		if (types::isBool(v)) {
			strFormat += 'b';

		}

		else if (types::isReel(v)) {
			strFormat += 'r';
		}

		else if (types::isStringPtr(v)) {
			strFormat += 's';
			auto strSize =
			    Attr::Builder.CreateLoad(irGenAide::getGEPAt(v, irGenAide::getConstantInt(0)));
			auto strCharPtr =
			    Attr::Builder.CreateLoad(irGenAide::getGEPAt(v, irGenAide::getConstantInt(1)));

			values[i] = strCharPtr;
			strFormat += 'd';
			values.insert(values.begin() + i + 1, strSize);
			++i;
		}

		else {
			// get the type Value* string from types::type(v)
			strFormat += 's';
			values[i] = Attr::Builder.CreateGlobalStringPtr(types::typestr(v));
		}
	}

	values.insert(values.begin(), Attr::Builder.CreateGlobalStringPtr(strFormat));
}


void matchArgTys(Function* fn, std::vector<Value*>& values) {
	auto fnTy = fn->getFunctionType();
	// first match arg numbers
	if (fnTy->getNumParams() > values.size()) {
		irGenAide::LogCodeGenError("Too few arguments in the function call to << " +
		                           names::demangleName(fn->getName().str()).second + " >>");
	}

	if (fnTy->getNumParams() < values.size()) {
		irGenAide::LogCodeGenError("Too many arguments in the function call to " +
		                           names::demangleName(fn->getName().str()).second + " >>");
	}

	// then match arg types
	for (int i = 0; i < values.size(); ++i) {
		auto v = values[i];
		auto paramTy = fnTy->getParamType(i);

		if (not types::isEqual(v->getType(), paramTy)) {
			irGenAide::LogCodeGenError("Type mismatch for argument #" + std::to_string(i + 1) +
			                           " in function call to << " +
			                           names::demangleName(fn->getName().str()).second + " >>");
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


Value* Prototype::codegen() {
	LogProgress("Creating Prototype for " + fnName);

	// TODO Check if a function with this name already exists

	std::vector<Type*> paramTys;
	for (auto& typeId : paramTypeIds) {
		Type* Ty = typeId->typegen();
		Ty = types::isEnttyPtr(Ty) ? Ty->getPointerTo() : Ty;
		paramTys.push_back(Ty);
	}

	// remember entities are passed around as pointers
	auto fnReturnTy = fnTypeId->typegen();
	fnReturnTy = types::isEnttyPtr(fnReturnTy) ? fnReturnTy->getPointerTo() : fnReturnTy;

	llvm::FunctionType* fnType;
	if (paramTypeIds.size()) {
		fnType = llvm::FunctionType::get(fnReturnTy, paramTys, false);
	}

	else {
		fnType = llvm::FunctionType::get(fnReturnTy, false);
	}

	llvm::Function* fn = llvm::Function::Create(fnType, llvm::GlobalValue::ExternalLinkage, fnName,
	                                            Attr::ThisModule.get());

	// Set names for all arguments so ir code is readable
	unsigned Idx = 0;
	for (auto& Arg : fn->args()) Arg.setName(paramNames[Idx++]);

	return fn;
}


Value* FunctionDefn::codegen() {
	LogProgress("Creating Function definition");

	if (Attr::ScopeStack.size() > 1) {
		irGenAide::LogCodeGenError("kronk doesn't allow nesting of function definitions");
	}

	auto PreFuncBlock = Attr::Builder.GetInsertBlock();

	Attr::ScopeStack.push_back(std::make_unique<Scope>());

	auto fnEntryBB = BasicBlock::Create(Attr::Context, "FunctionEntry");
	auto fnExitBB = BasicBlock::Create(Attr::Context, "FunctionExit");
	Attr::ScopeStack.back()->fnExitBB = fnExitBB;

	auto fn = llvm::cast<Function>(prototype->codegen());

	fn->getBasicBlockList().push_back(fnEntryBB);
	Attr::Builder.SetInsertPoint(fnEntryBB);

	Attr::ScopeStack.back()->returnValue =
	    Attr::Builder.CreateAlloca(fn->getReturnType(), nullptr, "ReturnValue");

	// set the name->address correspondence in the local symboltable for each argument
	for (auto& arg : fn->args()) {
		// allocate memory on the stack to hold the arguments
		llvm::AllocaInst* alloca = Attr::Builder.CreateAlloca(arg.getType());
		// store each argument in each allocated memory space
		Attr::Builder.CreateStore(static_cast<llvm::Value*>(&arg), alloca);
		// we use the names set in the protottype declaration.
		Attr::ScopeStack.back()->SymbolTable[std::string(arg.getName())] = alloca;
	}

	Body->codegen();

	if (not Attr::Builder.GetInsertBlock()->getTerminator()) {
		// this takes care of functions with no return stmts.
		Attr::Builder.CreateBr(fnExitBB);
	}

	fn->getBasicBlockList().push_back(fnExitBB);
	Attr::Builder.SetInsertPoint(fnExitBB);

	Attr::Builder.CreateRet(Attr::Builder.CreateLoad(Attr::ScopeStack.back()->returnValue));

	// if(not llvm::verifyFunction(*fn))
	//    irGenAide::LogCodeGenError("There's a problem with your function definition kronk can't figure
	//    out");

	Attr::ScopeStack.pop_back();
	Attr::Builder.SetInsertPoint(PreFuncBlock);

	return nullptr;
}


Value* ReturnStmt::codegen() {
	LogProgress("Generating Return Stmt");

	// All we do is store the return expression in the returnValue variable of this scope

	if (Attr::ScopeStack.size() == 1) {
		irGenAide::LogCodeGenError("return statements must only be in function definitions");
	}

	auto rvalue = returnExpr->codegen();
	auto lvalue = Attr::ScopeStack.back()->returnValue;

	auto rvalueTy = rvalue->getType();
	auto lvalueTy = lvalue->getType()->getPointerElementType();

	if (not types::isEqual(rvalueTy, lvalueTy)) {
		irGenAide::LogCodeGenError("Return value type does not correspond to function return type");
	}

	Attr::Builder.CreateStore(rvalue, lvalue);
	Attr::Builder.CreateBr(Attr::ScopeStack.back()->fnExitBB);

	return nullptr;
}


Value* FunctionCallExpr::codegen() {
	LogProgress("Creating function call to " + Callee);

	std::vector<Value*> ArgsV;  // holds codegen for each argument

	auto fnExists = names::Function(Callee);

	if (fnExists) {
		auto fn = *fnExists;
		for (auto& arg : Args) {
			auto argV = arg->codegen();
			ArgsV.push_back(argV);
		}

		// if the fn is an IO function, we don't need to match the argument types
		if ((names::demangleName(Callee).second == "afficher")) {
			// replace entity pointers with their names
			editArgsForPrint(ArgsV);

			return Attr::Builder.CreateCall(fn, ArgsV);
		}

		// match ArgsV against fn prototype
		matchArgTys(fn, ArgsV);
		return Attr::Builder.CreateCall(fn, ArgsV);
	}

	auto [kmodule, symbol] = names::demangleNameForErrMsg(Callee);
	irGenAide::LogCodeGenError("No function in the module << " + kmodule + " >> matches the name << " +
	                           symbol + " >>");
}