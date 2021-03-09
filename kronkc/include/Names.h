#ifndef _NAMES_H
#define _NAMES_H

#include "Attributes.h"


namespace names {

std::string mangleNameAsLocalType(const std::string& name);
std::string mangleNameAsForeignType(const std::string& incModule, const std::string& name);

std::string mangleNameAsLocalFunction(const std::string& name);
std::string mangleNameAsForeignFunction(const std::string& kmodule, const std::string& name);

std::string mangleIncludeId(const std::string& includeId);

std::pair<std::string, std::string> demangleName(const std::string& name);
std::pair<std::string, std::string> demangleNameForErrMsg(const std::string& name);

std::optional<StructType*> EntityType(const std::string& name);
std::optional<std::vector<std::string>> EntitySignature(const std::string& name);

std::optional<llvm::Function*> Function(const std::string& name);

fs::path getModuleFile(const std::string& moduleId = "");

bool createsCircularDep(const std::string& moduleId);
}  // namespace names


#endif