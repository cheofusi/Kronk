#include "Nodes.h"
#include "Driver.h"
#include "IRGenAide.h"
#include "Names.h"


static void saveCurrModuleState() {
    auto currModuleState = std::make_unique<SuspendedModuleState>(std::move(Attr::ThisModule));

    currModuleState->EntitySignatures = std::move(Attr::EntitySignatures);
    currModuleState->EntityTypes = std::move(Attr::EntityTypes);
    currModuleState->Dependencies = std::move(Attr::Dependencies);

    currModuleState->BuilderInsertPoint = Attr::Builder.saveAndClearIP();
    currModuleState->CurrentLexerLine = Attr::CurrentLexerLine;
    currModuleState->ScopeStack = std::move(Attr::ScopeStack);

    Attr::SuspendedModuleStack.push_back(std::move(currModuleState));
}


static void restoreCurrModuleState() {
    auto lastModuleState = std::move(Attr::SuspendedModuleStack.back());
    Attr::SuspendedModuleStack.pop_back();

    Attr::ThisModule = std::move(lastModuleState->TheModule);
    Attr::EntitySignatures = std::move(lastModuleState->EntitySignatures);
    Attr::EntityTypes = std::move(lastModuleState->EntityTypes);
    Attr::Dependencies = std::move(lastModuleState->Dependencies);

    Attr::CurrentLexerLine =  lastModuleState->CurrentLexerLine;
    Attr::ScopeStack = std::move(lastModuleState->ScopeStack);

    Attr::Builder.restoreIP(lastModuleState->BuilderInsertPoint);
}


static void checkDuplicateIncludeIds(std::string& includeId) {
    // checks if a module is already identified with moduleId in an include statement in the 
    // current module

    auto it = std::find(Attr::Dependencies.begin(), Attr::Dependencies.end(), includeId);

    if(it != Attr::Dependencies.end()) {
        irGenAide::LogCodeGenError("An included module is already identified by << " + includeId + " >> !!");
    } 
}


Value* IncludeStmt::codegen() {
    LogProgress("Starting compilation of included file");

    if(not rtModule.empty()) {
        // the included module is a stdlib/rt module, hence we don't have any compilation to do because the stdlib
        // is already compiled. So we construct an Attr::ModuleAttrs object whose only non-null data member is the
        // actual name of the runtime module = rtModule

        // check if rtModule is a kronkrt module
        auto it = Attr::KronkrtModules.find(rtModule);
        if(it == Attr::KronkrtModules.end()) {
            irGenAide::LogCodeGenError("<< " + rtModule + " >> is not a kronk runtime module !!");
        }
        
        auto includeId = (alias.empty()) ? rtModule : std::move(alias);
        checkDuplicateIncludeIds(includeId);
        
        auto moduleId = names::mangleIncludeId(includeId);

        
        auto rtModuleAttrs = std::make_unique<ModuleAttrs>(std::move(rtModule));
        Attr::ModuleMap.insert( std::make_pair(std::move(moduleId), std::move(rtModuleAttrs) ));

        Attr::Dependencies.push_back(includeId);  
    }

    else {
        // the include module is a kronk file to be compiled

        auto includeId = (alias.empty()) ? userModule.back() : std::move(alias); 
        checkDuplicateIncludeIds(includeId);
        
        // get the identifier that will be used to compile the included module
        auto moduleId = names::mangleIncludeId(includeId);
        
        // first save compile state of the current module
        saveCurrModuleState();

        // then try compiling the included file
        Attr::INCLUDE_MODE = true;
        auto cdriver = std::make_unique<CompileDriver>(std::move(userModule));
        auto result = cdriver->compileInputFile(std::move(moduleId));
     
        // module compile success!. Restore compile state of module in which the include stmt occurs
        restoreCurrModuleState();

        Attr::INCLUDE_MODE = false;
        // add the compiled file/module as a dependency of the current module
        Attr::Dependencies.push_back(includeId); 

        LogProgress("Successfully compiled included module");
    }

    return nullptr;
}