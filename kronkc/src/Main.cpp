#include "Driver.h"
#include "Attributes.h"
#include "argparse.h"



int main(int argc, char* argv[]) {

    argparse::ArgumentParser argparser("kronkc");

    argparser.add_argument("-i")
        .help("compile the file in include mode i.e only compile functions")
        .default_value(false)
        .implicit_value(true);

    argparser.add_argument("-d")
        .help("print debug info as kronk compiles the file")
        .default_value(false)
        .implicit_value(true);

    argparser.add_argument("file");

    try {
        argparser.parse_args(argc, argv);
    }

    catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << argparser;
        exit(EXIT_FAILURE);
    }
    
    Attr::INCLUDE_MODE = argparser.get<bool>("-i");
    Attr::ALLOW_DEBUG_INFO = argparser.get<bool>("-d");                               
    auto file = argparser.get<std::string>("file");

    auto driver = std::make_unique<CompilerDriver>(std::move(file));
    driver->compileInputFile();
}
