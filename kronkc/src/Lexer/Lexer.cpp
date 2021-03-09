#include "Lexer.h"

#include "Names.h"

static const std::unordered_set<char> SuffixesOfTwoCharOps = { '*', '=', '<', '>' };


static const std::unordered_map<std::string, uint8_t> keywords = { { "inclu", 0 },

	                                                               { "fn", 1 },

	                                                               { "ret", 2 },

	                                                               { "Entite", 3 },

	                                                               { "soit", 4 },

	                                                               { "Si", 5 },

	                                                               { "Sinon", 6 },

	                                                               { "Tantque", 7 },

	                                                               { "vrai", 8 },

	                                                               { "faux", 8 }

};

Lexer::Lexer(fs::path&& inputFile) {
	ifile = std::ifstream(std::move(inputFile));
	Attr::CurrentLexerLine = 1;
}


Token Lexer::scanNextToken(bool increment_irgen_line_offset) {
	// Handle whitespace
	while (LastChar == ' ') {
		LastChar = ReadNextChar();
	}

	// identifier: [a-zA-Z_][a-zA-Z0-9_]*
	if (isalpha(LastChar) or (LastChar == '_')) {
		IdentifierStr.clear();
		do {
			IdentifierStr += LastChar;
			LastChar = ReadNextChar();

		} while (isalnum(LastChar) or (LastChar == '_'));

		auto it = keywords.find(IdentifierStr);

		if (it != keywords.end()) {
			switch (it->second) {
				case 0:
					return Token::INCLUDE_STMT;

				case 1:
					return Token::FUNCTION_DEFN;

				case 2:
					return Token::RETURN_STMT;

				case 3:
					return Token::ENTITY_DEFN;

				case 4:
					return Token::DECLR_STMT;

				case 5:
					return Token::IF_STMT;

				case 6:
					return Token::ELSE_STMT;

				case 7:
					return Token::WHILE_STMT;

				case 8:
					return Token::BOOLEAN_LITERAL;
			}
		}

		if (Attr::KronkOperators.find(IdentifierStr) != Attr::KronkOperators.end())
			return Token::KRONK_OPERATOR;

		return Token::IDENTIFIER;
	}

	// string literal: ["*"]
	if (LastChar == 34) {  // ascii for double apostrophe
		IdentifierStr.clear();
		LastChar = ReadNextChar();
		while (LastChar != 34) {
			if (LastChar == EOF) LogTokenReadError("Incomplete string !!");

			IdentifierStr += LastChar;
			LastChar = ReadNextChar();
		}

		LastChar = ReadNextChar();  // get rid of the closing apostrophe
		return Token::STRING_LITERAL;
	}

	// Number: [0-9][0-9.e-]*
	if (isdigit(LastChar)) {
		std::string NumStr;
		char previousChar;
		do {
			NumStr += LastChar;
			previousChar = LastChar;
			LastChar = ReadNextChar();
			if (LastChar == '-' &&
			    previousChar != 'e') {  // test determining if negative sign is part of NumStr.
				break;
			}

		} while (isdigit(LastChar) || LastChar == '.' || LastChar == 'e' || LastChar == '-');

		if (verifyNumericStr(NumStr)) {
			NumericLiteral = strtod(NumStr.c_str(), 0);
			return Token::NUMERIC_LITERAL;
		}

		LogTokenReadError("Error reading Number " + NumStr);
	}

	// Non alphanumeric operators
	if ((Attr::KronkOperators.find(std::string(1, LastChar)) != Attr::KronkOperators.end()) or
	    (LastChar == '!')) {
		IdentifierStr = LastChar;
		LastChar = ReadNextChar();

		if (SuffixesOfTwoCharOps.find(LastChar) != SuffixesOfTwoCharOps.end()) {
			// all 2-nonalphanum-character operators end with  *, = , < or >
			IdentifierStr += LastChar;
			LastChar = ReadNextChar();
		}

		return Token::KRONK_OPERATOR;
	}

	// Handle comments && recursively call scanNextToken after comment body
	if (LastChar == '#') {
		do {
			LastChar = ReadNextChar();
		} while (LastChar != EOF and LastChar != '\n');  // skip whole line beginning with #

		// skip all subsequent new lines
		if (LastChar != EOF) {
			// do {
			//     Attr::CurrentLexerLine++;
			//     LastChar = ReadNextChar();
			// } while (LastChar == '\n');

			return scanNextToken(increment_irgen_line_offset);
		}
	}

	if (LastChar == '\n') {
		Attr::CurrentLexerLine++;
		(increment_irgen_line_offset) ? Attr::IRGenLineOffset++ : 0;

		LastChar = ReadNextChar();
		NonAlphaNumchar = LastChar;
		return Token::NEW_LINE;
	}

	if (LastChar == EOF) {
		ifile.close();
		return Token::END_OF_FILE;
	}

	// After all tests above, just return the non alphanumeric character as its ascii value.
	NonAlphaNumchar = LastChar;
	LastChar = ReadNextChar();
	return Token::NON_ALPHANUM_CHAR;
}


LLVM_ATTRIBUTE_NORETURN
void Lexer::LogTokenReadError(std::string errMsg) {
	outs() << "Token Read Error in " << names::getModuleFile().filename() << '\n'
	       << "[Line " << Attr::CurrentLexerLine << "]:  " << errMsg << '\n';

	exit(EXIT_FAILURE);
}


int Lexer::ReadNextChar() {
	// emulates getchar() function
	return ifile.get();
}


// Validates num string format. Allows stuff like 1e15, -1e-2. 10.1e2;
// The lexer doesn't decide whether to treat '-' as a negative number qualifier or a substraction
// operation. So it just returns the hyphen and it's up to the parser to decide what to do with it. So
// this verifyNumString function only checks for one hyphen; the one after the exponent.
bool Lexer::verifyNumericStr(const std::string& string) {
	bool dotPresent = false;
	bool ePresent = false;
	bool hyphenPresent = false;

	for (auto it = string.begin(); it != string.end(); ++it) {
		// Subsequent characters must be (e) or (.) or (-) or digit. Previous loop before this
		// verifyNumString function was called already checked for that. So we just make sure the number
		// of occurences and position of occurence of each valid character is valid.
		if (*it == '.') {
			if (dotPresent or ePresent) return false;  // cannot have decimal after exponent
			// if(i == string.size() - 1) return false; // for now lets allow user to have floats
			// terminate with decimals
			dotPresent = true;
		}

		if (*it == '-') {
			if (hyphenPresent) return false;
			if (*(it - 1) != 'e') return false;
		}

		if (*it == 'e') {
			if (ePresent) return false;
			if (not(isdigit(*(it - 1)))) return false;   // must proceed a digit;
			if ((it + 1) == string.end()) return false;  // number cannot terminate with exponent
			if ((not isdigit(*(it + 1))) and
			    (not(*(it + 1) == '-')))  // what proceeds e must be a digit or hyphen
				return false;
			ePresent = true;
		}
	}
	// All test are verified. numeric string is valid
	return true;
}