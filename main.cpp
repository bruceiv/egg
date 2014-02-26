/*
 * Copyright (c) 2013 Aaron Moss
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <fstream>
#include <iostream>
#include <string>

#include "egg.hpp"
#include "parser.hpp"
#include "visitors/compiler.hpp"
#include "visitors/normalizer.hpp"
#include "visitors/printer.hpp"
#include "visitors/interpreter.hpp"

/** Egg version */
static const char* VERSION = "0.3.1";

/** Egg usage string */
static const char* USAGE = 
"[-c print|compile|interpret] [-d defn_file] [-i input_file] [-o output_file]\n\
 [-r rule_name] [--no-norm] [--no-memo] [--help] [--version] [--usage]";

/** Full Egg help string */
static const char* HELP = 
"egg [command] [flags] [input-file [output-file]]\n\
\n\
Supported flags are\n\
 -d --defn     grammar definition file (default stdin)\n\
 -i --input    input file (default stdin)\n\
 -o --output   output file (default stdout)\n\
 -c --command  command - either compile, interpret, print, help, usage, or\n\
               version (default compile, interpret takes two arguments)\n\
 -r --rule     interpreter rule name (default empty)\n\
 -n --name     grammar name - if none given, takes the longest prefix of\n\
               the input or output file name (output preferred) which is a\n\
               valid Egg identifier (default empty)\n\
 --no-norm     turns off grammar normalization\n\
 --no-memo     turns of grammar memoization\n\
 --usage       print usage message\n\
 --help        print full help message\n\
 --version     print version string\n";

/** Command to run */
enum egg_mode {
	PRINT_MODE,      /**< Print grammar */
	COMPILE_MODE,    /**< Compile grammar */
	INTERPRET_MODE,  /**< Interpret grammar */
	USAGE_MODE,      /**< Print usage */
	HELP_MODE,       /**< Print help */
	VERSION_MODE     /**< Print version */
};

class args {
private:
	bool eq(const char* lit, char* arg) {
		return std::string(lit) == std::string(arg);
	}

	bool match(const char* shrt, const char* lng, char* arg) {
		std::string a(arg);
		return std::string(shrt) == a || std::string(lng) == a;
	}

	std::string id_prefix(char* s) {
		int len = 0;
		char c = s[len];
		if ( ( c >= 'A' && c <= 'Z' )
				|| ( c >= 'a' && c <= 'z' )
				|| c == '_' ) {
			c = s[++len];
		} else {
			return std::string("");
		}

		while ( ( c >= 'A' && c <= 'Z' )
				|| ( c >= 'a' && c <= 'z' )
				|| ( c >= '0' && c <= '9' )
				|| c == '_' ) {
			c = s[++len];
		}

		return std::string(s, len);
	}

	bool parse_mode(char* s) {
		if ( eq("print", s) ) {
			eMode = PRINT_MODE;
			return true;
		} else if ( eq("compile", s) ) {
			eMode = COMPILE_MODE;
			return true;
		} else if ( eq("interpret", s) ) {
			eMode = INTERPRET_MODE;
			return true;
		} else if ( eq("help", s) ) {
			eMode = HELP_MODE;
			return true;
		} else if ( eq("usage", s) ) {
			eMode = USAGE_MODE;
			return true;
		} else if ( eq("version", s) ) {
			eMode = VERSION_MODE;
			return true;
		} else {
			return false;
		}
	}

	void parse_input(char* s) {
		in = new std::ifstream(s);
		inName = s;
		if ( !nameFlag && out == nullptr ) {
			pName = id_prefix(s);
		}
	}

	void parse_output(char* s) {
		out = new std::ofstream(s);
		outName = s;
		if ( !nameFlag ) {
			pName = id_prefix(s);
		}
	}
	
	void parse_source(char* s) {
		src = new std::ifstream(s);
		srcName = s;
	}
	
	void parse_rule(char* s) {
		rName = id_prefix(s);
	}

	void parse_name(char* s) {
		pName = id_prefix(s);
		nameFlag = true;
	}

public:
	args(int argc, char** argv) 
		: in(nullptr), out(nullptr), src(nullptr),
		  inName(), outName(), srcName(), pName(), rName(),
		  nameFlag(false), normFlag(true), memoFlag(true), eMode(COMPILE_MODE) {
		
		i = 1;
		if ( argc <= 1 ) return;

		//parse optional sub-command
		if ( parse_mode(argv[i]) ) { ++i; }
		
		//parse explicit flags
		for (; i < argc; ++i) {
			if ( match("-i", "--input", argv[i]) ) {
				if ( i+1 >= argc ) return;
				parse_input(argv[++i]);
			} else if ( match("-o", "--output", argv[i]) ) {
				if ( i+1 >= argc ) return;
				parse_output(argv[++i]);
			} else if ( match("-s", "--source", argv[i]) ) {
				if ( i+1 >= argc ) return;
				parse_source(argv[++i]);
			} else if ( match("-c", "--command", argv[i]) ) {
				if ( i+1 >= argc ) return;
				parse_mode(argv[++i]);
			} else if ( match("-n", "--name", argv[i]) ) {
				if ( i+1 >= argc ) return;
				parse_name(argv[++i]);
			} else if ( match("-r", "--rule", argv[i]) ) {
				if ( i+1 >= argc ) return;
				parse_rule(argv[++i]);
			} else if ( eq("--no-norm", argv[i]) ) {
				normFlag = false;
			} else if ( eq("--no-memo", argv[i]) ) {
				memoFlag = false;
			} else if ( eq("--usage", argv[i]) ) {
				eMode = USAGE_MODE;
			} else if ( eq("--help", argv[i]) ) {
				eMode = HELP_MODE;
			} else if ( eq("--version", argv[i]) ) {
				eMode = VERSION_MODE;
			} else break;
		}

		//parse optional input, source, and output files
		if ( i < argc && in == nullptr )  parse_input(argv[i++]);
		if ( eMode == INTERPRET_MODE ) {
			if ( i < argc && rName.empty() )  parse_rule(argv[i++]);
			if ( i < argc && src == nullptr ) parse_source(argv[i++]);
		}
		if ( i < argc && out == nullptr ) parse_output(argv[i++]);
	}

	~args() {
		if ( in != nullptr ) in->close();
		if ( out != nullptr ) out->close();
		if ( src != nullptr ) src->close();
	}

	std::istream& input() { if ( in ) return *in; else return std::cin; }
	std::ostream& output() { if ( out ) return *out; else return std::cout; }
	std::istream& source() { if ( src ) return *src; else return std::cin; }
	std::string inputFile() { return in ? inName : "<STDIN>"; }
	std::string outputFile() { return out ? outName : "<STDOUT>"; }
	std::string sourceFile() { return src ? srcName : "<STDIN>"; }
	std::string name() { return pName; }
	std::string rule() { return rName; }
	bool norm() { return normFlag; }
	bool memo() { return memoFlag; }
	egg_mode mode() { return eMode; }

private:
	int i;				  /**< next unparsed value */
	std::ifstream* in;	  /**< pointer to input stream (0 for stdin) */
	std::ofstream* out;	  /**< pointer to output stream (0 for stdout) */
	std::ifstream* src;   /**< pointer to the interpreted source file (0 for stdin) */
	std::string inName;   /**< Name of the input file (empty if none) */
	std::string outName;  /**< Name of the output file (empty if none) */
	std::string srcName;  /**< Name of the interpreted source file (empty if none) */
	std::string pName; 	  /**< the name of the parser (empty if none) */
	std::string rName;    /**< the name of the rule to interpret (empty if none) */
	bool nameFlag;		  /**< has the parser name been explicitly set? */
	bool normFlag;        /**< should egg do grammar normalization? */
	bool memoFlag;        /**< should the generated grammar do memoization? */
	egg_mode eMode;		  /**< compiler mode to use */
};

/** Command line interface
 *  egg [command] [flags] [input-file [output-file]]
 *  
 *  Supported flags are
 *  -i --input    input file (default stdin)
 *  -o --output   output file (default stdout)
 *  -c --command  command - either compile, interpret, print, help, usage, or
 *                version (default compile, interpret takes two arguments)
 *  -n --name     grammar name - if none given, takes the longest prefix of 
 *                the input or output file name (output preferred) which is a 
 *                valid Egg identifier (default empty)
 *  --no-norm     turns off grammar normalization
 *  --usage       print usage message
 *  --help        print full help message
 *  --version     print version string
 */
int main(int argc, char** argv) {

	args a(argc, argv);
	
	switch ( a.mode() ) {
	case USAGE_MODE:
		std::cout << argv[0] << " " << USAGE << std::endl;
		return 0;
	case HELP_MODE:
		std::cout << HELP << std::endl;
		return 0;
	case VERSION_MODE:
		std::cout << "Egg version " << VERSION << std::endl;
		return 0;
	default: break;
	}
	
std::cout << "DBG mode:`" << a.mode()
          << "` source:`" << a.sourceFile() 
          << "` rule:`" << a.rule()
          << "` in:`" << a.inputFile()
          << "` out:`" << a.outputFile()
          << "`" << std::endl;

	parser::state ps(a.input());
	ast::grammar_ptr g;
	
	if ( egg::grammar(ps, g) ) {
		std::cout << "DONE PARSING" << std::endl;
		if ( a.norm() ) {
			visitor::normalizer n;
			n.normalize(*g);
		}

		switch ( a.mode() ) {
		case PRINT_MODE: {
			visitor::printer p(a.output());
			p.print(*g);
			break;
		} case COMPILE_MODE: {
			visitor::compiler c(a.name(), a.output());
			c.memo(a.memo());
			c.compile(*g);
			break;
		} case INTERPRET_MODE: {
			derivs::interpreter i = derivs::interpreter::from_ast(*g);
			parser::state in(a.source());
			bool b = i.match(in, a.rule());
			std::cout << "Grammar " << ( b ? "matched" : "DID NOT match" ) << std::endl;
		} default: break;
		}
		
	} else {
		const parser::error& err = ps.error();
			
		std::cerr << "PARSE FAILURE @" << err.pos.line() << ":" << err.pos.col() << std::endl;
		for (auto msg : err.messages) {
			std::cerr << "\t" << msg << std::endl;
		}
		for (auto exp : err.expected) {
			std::cerr << "\tExpected " << exp << std::endl;
		}
	}

	return 0;
} /* main() */

