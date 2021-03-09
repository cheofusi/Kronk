#ifndef _DRIVER_H_
#define _DRIVER_H_

#include "IRGen.h"


class CompileDriver {
	fs::path inputFile;

	const fs::directory_options fileSearchOptions = (fs::directory_options::follow_directory_symlink |
	                                                 fs::directory_options::skip_permission_denied);


	void driver();
	bool inputFileExists();
	void registerModule();
	void LogImportError(std::string errMsg);

public:
	CompileDriver(std::string&& inputFile);
	CompileDriver(std::vector<std::string>&& inputFile);

	std::string compileInputFile(std::string&& moduleId = "");
};


#endif