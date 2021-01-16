#ifndef _PARSER_H_
#define _PARSER_H_

#include "Lexer.h"
#include "Nodes.h"

class Parser {
    public:
        static std::unique_ptr<Parser> CreateParser();

        virtual Token moveToNextToken(bool ignore_subsequent_newlines = false) = 0;
        virtual Token currToken() = 0;
        virtual std::unique_ptr<Node> ParseStmt() = 0;

        virtual ~Parser() {}
};


#endif