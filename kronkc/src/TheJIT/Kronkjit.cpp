#include "Kronkjit.h"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Passes/PassBuilder.h>

#include <llvm/CodeGen/CommandFlags.inc>


static ExitOnError ExitOnErr("FATAL ERROR");

void Kronkjit::LinkAndOptimize() {
	// first link

	auto linker = std::make_unique<Linker>(*MainModule.get());

	for (auto& [_, kModule] : Attr::ModuleMap) {
		// skip those fake modules that we used to refer to modules in the kronk runtime.
		if (kModule->rtModule.empty()) {
			linker->linkInModule(std::move(kModule->TheModule));
		}
	}

	// then optimze

	llvm::PassBuilder passBuilder;
	llvm::LoopAnalysisManager loopAnalysisManager;  // true is just to output debug info
	llvm::FunctionAnalysisManager functionAnalysisManager;
	llvm::CGSCCAnalysisManager cGSCCAnalysisManager;
	llvm::ModuleAnalysisManager moduleAnalysisManager;

	passBuilder.registerModuleAnalyses(moduleAnalysisManager);
	passBuilder.registerCGSCCAnalyses(cGSCCAnalysisManager);
	passBuilder.registerFunctionAnalyses(functionAnalysisManager);
	passBuilder.registerLoopAnalyses(loopAnalysisManager);

	passBuilder.crossRegisterProxies(loopAnalysisManager, functionAnalysisManager, cGSCCAnalysisManager,
	                                 moduleAnalysisManager);

	llvm::ModulePassManager modulePassManager =
	    passBuilder.buildPerModuleDefaultPipeline(llvm::PassBuilder::OptimizationLevel::O3);
	modulePassManager.run(*MainModule.get(), moduleAnalysisManager);

	// MainModule->print(llvm::errs(), nullptr);
}


LLVM_ATTRIBUTE_NORETURN
static void jitCompileFailure() {
	outs() << "FATAL ERROR.." << '\n';
	exit(EXIT_FAILURE);
}


int Kronkjit::runOrcLazyJIT() {
	// Start setting up the JIT environment.
	// Parse the main module.
	orc::ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());
	SMDiagnostic Err;

	const auto& TT = MainModule->getTargetTriple();
	if (TT.empty()) {
		jitCompileFailure();
	}

	orc::LLLazyJITBuilder Builder;

	Builder.setJITTargetMachineBuilder(orc::JITTargetMachineBuilder(Triple(TT)));

	if (not MArch.empty()) {
		Builder.getJITTargetMachineBuilder()->getTargetTriple().setArchName(MArch);
	}

	Builder.getJITTargetMachineBuilder()
	    ->setCPU(getCPUStr())
	    .addFeatures(getFeatureList())
	    .setRelocationModel(RelocModel.getNumOccurrences() ? Optional<Reloc::Model>(RelocModel) : None)
	    .setCodeModel(CMModel.getNumOccurrences() ? Optional<CodeModel::Model>(CMModel) : None);

	Builder.setLazyCompileFailureAddr(pointerToJITTargetAddress(jitCompileFailure));
	Builder.setNumCompileThreads(2);

	auto J = ExitOnErr(Builder.create());

	J->setLazyCompileTransform(
	    [&](orc::ThreadSafeModule TSM, const orc::MaterializationResponsibility& R) {
		    TSM.withModuleDo([&](Module& M) {
			    if (verifyModule(M, &dbgs())) {
				    jitCompileFailure();
			    }
		    });
		    return TSM;
	    });

	J->getMainJITDylib().addGenerator(ExitOnErr(
	    orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(J->getDataLayout().getGlobalPrefix())));

	orc::MangleAndInterner Mangle(J->getExecutionSession(), J->getDataLayout());
	orc::LocalCXXRuntimeOverrides CXXRuntimeOverrides;
	ExitOnErr(CXXRuntimeOverrides.enable(J->getMainJITDylib(), Mangle));

	// Add the main module and the runtime.
	ExitOnErr(J->addLazyIRModule(orc::ThreadSafeModule(std::move(MainModule), TSCtx)));
	Attr::Kronkrt->materializeAll();
	ExitOnErr(J->addLazyIRModule(orc::ThreadSafeModule(std::move(Attr::Kronkrt), TSCtx)));

	// Run any static constructors.
	ExitOnErr(J->runConstructors());

	// Run main.
	auto MainSym = ExitOnErr(J->lookup("main"));
	using MainFnPtr = int (*)();

	auto Main = reinterpret_cast<MainFnPtr>(static_cast<uintptr_t>(MainSym.getAddress()));
	auto result = Main();

	// Run destructors.
	ExitOnErr(J->runDestructors());
	CXXRuntimeOverrides.runDestructors();

	return result;
}
