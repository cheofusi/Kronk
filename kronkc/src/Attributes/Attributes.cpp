#include "Attributes.h"

// This file holds definitions for compiler attributes whose values are shared are shared by all compiler
// components

namespace Attr {

bool PRINT_DEBUG_INFO;
bool INCLUDE_MODE;  // compilation mode for the kronk file

llvm::LLVMContext Context;
IRBuilder<> Builder(Context);

//////////////////////// module specific attributes ///////////////////////////////////////
// used for reporting line at which error occurs
std::unique_ptr<llvm::Module> ThisModule;

// maps entity type strings to their signature names
std::unordered_map<std::string, std::vector<std::string>> EntitySignatures;
// maps entity type strings to their signature types.
std::unordered_map<std::string, StructType*> EntityTypes;

size_t CurrentLexerLine;
// stack for storing scopes
std::vector<std::unique_ptr<Scope>> ScopeStack;

// dependency moduleIds for ThisModule
std::vector<std::string> Dependencies;
///////////////////////////////////////////////////////////////////////////////////////////

// maps module names to their respective attributes.
std::unordered_map<std::string, std::unique_ptr<ModuleAttrs>> ModuleMap;
// A stack whose top element holds the name of the current module being compiled, whose bottom element
// holds the name of the main module, and whose other elements are modules whose compilation has been
// suspended just like main module.
std::vector<std::unique_ptr<SuspendedModuleState>> SuspendedModuleStack;

// maps a moduleId of a module to the primordial moduleId that was used to compile that module. This
// prevents multiple compilation of the same module with different moduleIds.
std::unordered_map<std::string, std::string> DuplicateModuleMap;
// maps a filename ( the absolute file path ) to the moduleId used to compile it (this moduleId is
// referred) to as the primordial moduleId
std::map<fs::path, std::string> FileToModuleIdMap;


SMDiagnostic error;
// module hold all kronk rutime functions
std::unique_ptr<Module> Kronkrt = getLazyIRFileModule("bin/libkronkrt.bc", error, Context);
const std::unordered_set<std::string> KronkrtModules = { "io", "math" };

// special functions in the kronkrt that can be called directly without including their rtModule;
const std::unordered_map<std::string, std::string> DirectFunctions = { { "afficher", "io" } };

const std::unordered_set<std::string> BuiltinTypes = { "bool", "reel", "str" };

const std::unordered_map<std::string, uint8_t> KronkOperators = {
	{ "non", 100 }, { "~", 100 },

	{ "**", 90 },

	{ "*", 80 },    { "/", 80 },  { "mod", 80 },

	{ "+", 70 },    { "-", 70 },

	{ "<<", 60 },   { ">>", 60 },

	{ "&", 50 },    { "^", 40 },  { "|", 30 },

	{ "<", 20 },    { ">", 20 },  { "<=", 20 },  { ">=", 20 },

	{ "==", 20 },   { "!=", 20 },

	{ "et", 10 },   { "ou", 8 },

	{ "=", 1 }
};

// Unary operators have the highest precedence i.e the immediately grab the atomic that follows.
// `-` is considered unary when it appears at the start of an expected atomic expr.
const std::unordered_set<std::string> UnaryOps = { "-", "non", "~" };

// const std::unordered_set<std::string> AccessOps = {".", "["};

const std::unordered_set<std::string> RelationalOps = { "<", ">", "<=", ">=", "==", "!=" };

const std::unordered_set<std::string> RightAssociativeOps = { "**", "=" };

// attribute used by irgen to accurately report the line at which an error ocurrs since
// the lexer moves to the next statement before ir is generated for the current statement's ast
size_t IRGenLineOffset;


}  // end of namespace Attr


// Progress Logging
void LogProgress(std::string str) {
	if (Attr::PRINT_DEBUG_INFO) std::cout << "[ Debug Info ]: " << str << std::endl;
}
