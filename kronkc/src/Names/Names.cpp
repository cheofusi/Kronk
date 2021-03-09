#include "Names.h"

#include "IRGenAide.h"


std::string getPrimordialModuleId(const std::string& moduleId) {
	// returns the original identifier that is/was used to compile the file moduleId refers to.
	// moduleId could be that id.

	auto it = Attr::DuplicateModuleMap.find(moduleId);

	if (it != Attr::DuplicateModuleMap.end()) {
		// moduleId is not primordial
		return it->second;
	}

	// moduleId is primordial
	return moduleId;
}


namespace names {

std::string mangleNameAsLocalType(const std::string& name) {
	// mangles name by prefixing it with the current module identifier and the string 'Entity'
	// ex. in main.krk Entity.main::Point

	auto moduleId = Attr::ThisModule->getModuleIdentifier();
	auto mname = "Entity." + moduleId + "::" + name;

	return std::move(mname);
}

std::string mangleNameAsForeignType(const std::string& kmoduleId, const std::string& name) {
	// mangles name as an entity by prefixing it with the Attr::ThisModuleId + kmoduleId and
	// some other shit. Used in cases where the parser encounters a foreign symbol reference
	// e.x someModule::Point in main.krk -> Entity.main_someModule::Point

	auto moduleId = Attr::ThisModule->getModuleIdentifier() + "_" + kmoduleId;
	auto mname = "Entity." + moduleId + "::" + name;

	return std::move(mname);
}


std::string mangleNameAsLocalFunction(const std::string& name) {
	// mangles name by prefixing it with the current module identifier and some other shit.
	// ex. rotatePoint() in a file main.krnk -> _Z4main11rotatePoint

	auto moduleId = Attr::ThisModule->getModuleIdentifier();
	auto mname = "_Z" + std::to_string(moduleId.size()) + moduleId + std::to_string(name.size()) + name;

	return std::move(mname);
}

std::string mangleNameAsForeignFunction(const std::string& kmoduleId, const std::string& name) {
	// mangles name by prefixing it with kmodule and some other shit. Used in cases where the parser
	// encounters a foreign symbol reference.
	// ex, myMathlib::rotatePoint() in a file main.krk -> _Z14main_myMathlib11rotatePoint

	auto moduleId = Attr::ThisModule->getModuleIdentifier() + "_" + kmoduleId;
	auto mname = "_Z" + std::to_string(moduleId.size()) + moduleId + std::to_string(name.size()) + name;

	return std::move(mname);
}


std::string mangleIncludeId(const std::string& includeId) {
	// mangles the identifier use to include a kronk module/file by prefixing it with the current module
	// identifier ex. in main.krk we can have `inclu mylib as l`. The mylib file/module will compiled
	// with the moduleId `main_l`, given of course that main.krk 's moduleId is `main`

	return Attr::ThisModule->getModuleIdentifier() + "_" + includeId;
}


std::pair<std::string, std::string> demangleName(const std::string& name) {
	// demangles name by spliting it into 2 components; the prefix, which is a (supposed) moduleId in
	// which the suffix, which is either an entity type or function identifier, is defined. The function
	// then tries replacing the prefix with its primordial moduleId.

	if (name[0] == 'E') {
		// name begins with the string Entity ex. Entity.mainsomeModule::Point. So demangle it as a type

		auto str = name.substr(7);  // strip 'Entity.' prefix
		auto splitIdx = str.find(':');
		auto prefix = str.substr(0, splitIdx);
		auto suffix = str.substr(splitIdx + 2);

		return std::make_pair(prefix, suffix);
	}

	// name is function ex. _Z11mainmyMathlib11rotatePoint

	auto str = name.substr(2);  // strip '_Z' prefix
	auto it = str.begin();
	while (isdigit(*it)) it++;

	auto prefixLenStr = str.substr(0, std::distance(str.begin(), it));
	auto prefix = str.substr(prefixLenStr.size(), std::stoi(prefixLenStr));

	auto str2 = str.substr(prefixLenStr.size() + prefix.size());
	auto it2 = str2.begin();
	while (isdigit(*it2)) it2++;

	auto suffixLenStr = str2.substr(0, std::distance(str2.begin(), it2));
	auto suffix = str2.substr(suffixLenStr.size(), std::stoi(suffixLenStr));

	prefix = getPrimordialModuleId(prefix);

	return std::make_pair(prefix, suffix);
}

std::pair<std::string, std::string> demangleNameForErrMsg(const std::string& name) {
	// demangles name, returning the filename of the file in which name was defined, and the symbol.
	// Doesn't do any checks

	auto [kmodule, symbol] = demangleName(name);
	auto filename = getModuleFile(kmodule).stem().string();

	return std::make_pair(std::move(filename), std::move(symbol));
}


std::optional<StructType*> EntityType(const std::string& name) {
	// demangles name into a pair and checks if pair.first is actually a module that contains
	// an entity type identified by name. Returns the correspoding struct type.

	auto [moduleId, _] = demangleName(name);
	// moduleId either refers to the current module or is a dep or the current module. The caller must
	// have checked that before calling this function

	// first check the current module
	auto str = Attr::ThisModule->getModuleIdentifier();
	if (Attr::ThisModule->getModuleIdentifier() == moduleId) {
		// moduleId refers to the current module

		auto foundTy = Attr::EntityTypes.find(name);
		if (foundTy != Attr::EntityTypes.end()) {
			return (foundTy->second);
		}

	}

	else {
		// moduleId is a dep of the current module which is primordial as the call to demangleName()
		// ensured that. Also the module it refers to must have already
		// been compiled or is a stdlib/rtModule because the compile driver doesn't allow circular deps.

		auto& moduleAttrs = Attr::ModuleMap[moduleId];

		if (moduleAttrs->rtModule.empty()) {
			// user module
			auto foundTy = moduleAttrs->EntityTypes.find(name);
			if (foundTy != moduleAttrs->EntityTypes.end()) {
				return (foundTy->second);
			}

		}

		else {
			// kronk rtModule.
		}
	}


	return std::nullopt;
}


std::optional<std::vector<std::string>> EntitySignature(const std::string& name) {
	// demangles name into a pair and checks if pair.first is actually a module that contains
	// an entity type identified by name. Returns the correspoding entity signature

	auto [moduleId, _] = demangleName(name);

	// moduleId either refers to the current module or is a dep or the current module. The caller must
	// have checked that before calling this function

	// first check the current module
	auto str = Attr::ThisModule->getModuleIdentifier();
	if (Attr::ThisModule->getModuleIdentifier() == moduleId) {
		// moduleId refers to the current module

		auto foundTy = Attr::EntitySignatures.find(name);
		if (foundTy != Attr::EntitySignatures.end()) {
			return (foundTy->second);
		}

	}

	else {
		// moduleId is a dep of the current module. It may not be primordial in which case the
		// call to demangleName() must have fixed that. Also the module it refers to must have already
		// been compiled or is a stdlib/rtModule because the compile driver doesn't allow circular deps.

		auto& moduleAttrs = Attr::ModuleMap[moduleId];

		if (moduleAttrs->rtModule.empty()) {
			// user module
			auto foundSig = moduleAttrs->EntitySignatures.find(name);
			if (foundSig != moduleAttrs->EntitySignatures.end()) {
				return (foundSig->second);
			}

		}

		else {
			// kronk rtModule.
		}
	}


	return std::nullopt;
}


std::optional<llvm::Function*> Function(const std::string& name) {
	// demangles name into a pair and checks if pair.first is actually a module that contains
	// a function identified by name. Returns the correspoding llvm::Function*

	auto [moduleId, symbol] = demangleName(name);

	// first check the current module
	if (Attr::ThisModule->getModuleIdentifier() == moduleId) {
		auto foundFn = Attr::ThisModule->getFunction(name);
		if (foundFn) {
			return foundFn;
		}

		// check if its a special function like afficher
		else if (auto it = Attr::DirectFunctions.find(symbol); it != Attr::DirectFunctions.end()) {
			auto mname = "_k" + it->second + "_" + symbol;
			return Attr::Kronkrt->getFunction(mname);
		}
	}

	else {
		// moduleId is a dep of the current module. The caller must have checked that before calling this
		// function moduleId may not be primordial but in that case the call to demangleName() must have
		// fixed that. Also the module it refers to must have already been compiled because the compile
		// driver doesn't allow circular deps.

		auto& moduleAttrs = Attr::ModuleMap[moduleId];

		if (moduleAttrs->rtModule.empty()) {
			// user module
			auto foundFn = moduleAttrs->TheModule->getFunction(name);
			if (foundFn) {
				return foundFn;
			}
		}

		else {
			// kronkrt module. We need to mangle the symbol before we look for it. ex in the runtime
			// module `math`, the function `sin` is defined as `_kmath_sin`

			auto mname = "_k" + moduleAttrs->rtModule + "_" + symbol;
			if (auto foundFn = Attr::Kronkrt->getFunction(mname)) {
				// insert runtime function declaration if it doesn't exist yet.
				Attr::ThisModule->getOrInsertFunction(foundFn->getName(), foundFn->getFunctionType());
				return foundFn;
			}
		}
	}

	return std::nullopt;
}


fs::path getModuleFile(const std::string& moduleId) {
	// returns the file associated with moduleId i.e the file compiled with the identifier moduleId

	auto mId = moduleId.empty() ? Attr::ThisModule->getModuleIdentifier() : moduleId;

	if (auto it = Attr::ModuleMap.find(moduleId); it != Attr::ModuleMap.end()) {
		if (not it->second->rtModule.empty()) {
			// kronk runtime module, which has no associated file.
			return it->second->rtModule;
		}
	}

	auto it = std::find_if(Attr::FileToModuleIdMap.begin(), Attr::FileToModuleIdMap.end(),
	                       [mId](const auto& entry) { return entry.second == mId; });

	return it->first;
}


bool createsCircularDep(const std::string& moduleId) {
	// returns false if moduleId doesn't create circular dependency and true otherwise. If A.krk includes
	// B.krk, we can denote the relation as A <-- B. Then examples of circular deps would be
	//      A <-- A
	//      A <-- B <-- A
	//      A <-- B <-- C < -- A
	//      A <-- B <-- C < -- B

	// This happens when we try to include a module whose compilation state is pending i.e it is
	// somewhere in the module stack.

	for (const auto& halfCompiledModule : Attr::SuspendedModuleStack) {
		if (halfCompiledModule->TheModule->getModuleIdentifier() == moduleId) {
			return true;
		}
	}

	return false;
}

}  // namespace names