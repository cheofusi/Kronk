#ifndef _PARSER_H_
#define _PARSER_H_

#include "IRGen.h"
#include "Lexer.h"


class Parser {
    public:
        static std::unique_ptr<Parser> CreateParser(fs::path&& inputFile);

        virtual Token moveToNextToken(bool ignore_subsequent_newlines = false,
                                    bool increment_irgen_line_offset = false) = 0;
        virtual Token currToken() = 0;
        virtual std::unique_ptr<Node> ParseStmt(bool caller_is_driver = false) = 0;

        virtual ~Parser() {}
};


#endif