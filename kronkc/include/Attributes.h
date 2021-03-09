#ifndef _ATTRIBUTES_H_
#define _ATTRIBUTES_H_

#include "IRGen.h"


class ModuleAttrs;
class SuspendedModuleState;

// holds general attributes used throughout the compiler and the specific attributes of the current
// module being compiled.
namespace Attr {
extern bool PRINT_DEBUG_INFO;
extern bool INCLUDE_MODE;

////////////////////// module specific attributes /////////////////////////////////
extern std::unique_ptr<llvm::Module> ThisModule;

extern std::unordered_map<std::string, std::vector<std::string>> EntitySignatures;
extern std::unordered_map<std::string, StructType*> EntityTypes;

extern size_t CurrentLexerLine;
extern std::vector<std::unique_ptr<Scope>> ScopeStack;

extern std::vector<std::string> Dependencies;
//////////////////////////////////////////////////////////////////////////////////

extern std::unordered_map<std::string, std::unique_ptr<ModuleAttrs>> ModuleMap;
extern std::vector<std::unique_ptr<SuspendedModuleState>> SuspendedModuleStack;

extern std::unordered_map<std::string, std::string> DuplicateModuleMap;
extern std::map<fs::path, std::string> FileToModuleIdMap;


extern llvm::LLVMContext Context;
extern IRBuilder<> Builder;

extern std::unique_ptr<Module> Kronkrt;
extern const std::unordered_set<std::string> KronkrtModules;

extern const std::unordered_map<std::string, std::string> DirectFunctions;

extern const std::unordered_set<std::string> BuiltinTypes;

extern const std::unordered_map<std::string, uint8_t> KronkOperators;
extern const std::unordered_set<std::string> UnaryOps;
extern const std::unordered_set<std::string> RelationalOps;
extern const std::unordered_set<std::string> RightAssociativeOps;

extern size_t IRGenLineOffset;

}  // namespace Attr


// type for holding module specific attributes.
struct ModuleAttrs {
	std::unique_ptr<llvm::Module> TheModule;

	std::unordered_map<std::string, std::vector<std::string>> EntitySignatures;
	std::unordered_map<std::string, StructType*> EntityTypes;

	std::vector<std::string> Dependencies;

	// ex in the stmt `inclu :math as km`, km is the rtModuleAttrs.second (moduleId) and math is
	// the rtModuleAttrs.first (rtModule). In the stmt`inclu :math`, math is both the moduleId and
	// the rtModule.
	std::string rtModule;

	// This constructor is used when putting together the attributes of a user defined module or kronk
	// file
	ModuleAttrs(std::unique_ptr<llvm::Module> ThisModule) : TheModule(std::move(ThisModule)) {}

	// This constructor is used when putting together the attributes of a kronk stdlib/rtModule
	ModuleAttrs(std::string&& rtModule) : rtModule(std::move(rtModule)) {}
};


// type for holding module state when include statement suspends compilation of the current module
// to compile an included module

struct SuspendedModuleState : public ModuleAttrs {
	// point at which Attr::Builder was before the include statement
	IRBuilderBase::InsertPoint BuilderInsertPoint;

	size_t CurrentLexerLine;
	std::vector<std::unique_ptr<Scope>> ScopeStack;

	SuspendedModuleState(std::unique_ptr<llvm::Module> ThisModule)
	    : ModuleAttrs(std::move(ThisModule)) {}
};


void LogProgress(std::string str);


#endif