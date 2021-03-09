#include "Attributes.h"
#include "Driver.h"
#include "Kronkjit.h"
#include "argparse.h"


int main(int argc, char* argv[]) {
	argparse::ArgumentParser argparser("kronkc");


	argparser.add_argument("-d")
	    .help("print debug info as kronk compiles the file")
	    .default_value(false)
	    .implicit_value(true);

	argparser.add_argument("inputFile");

	try {
		argparser.parse_args(argc, argv);
	}

	catch (const std::runtime_error& err) {
		std::cout << err.what() << '\n';
		std::cout << argparser;
		exit(EXIT_FAILURE);
	}


	auto inputFile = argparser.get<std::string>("inputFile");

	Attr::PRINT_DEBUG_INFO = argparser.get<bool>("-d");
	Attr::INCLUDE_MODE = false;

	auto driver = std::make_unique<CompileDriver>(std::move(inputFile));
	driver->compileInputFile();

	auto jit = std::make_unique<Kronkjit>();
	jit->LinkAndOptimize();
	jit->runOrcLazyJIT();
}
