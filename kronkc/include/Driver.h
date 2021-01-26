#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <string>


class CompilerDriver {
    std::string inputFile;
    
    bool fileExists();
    void driver();

    public:
        CompilerDriver(std::string inputFile)
            : inputFile(std::move(inputFile)) {}
        
        void compileInputFile();
};


#endif