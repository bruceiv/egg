#include <fstream>
#include <iostream>
#include <string>

#include "egg.hpp"
#include "parse.hpp"
#include "visitors/compiler.hpp"
#include "visitors/normalizer.hpp"
#include "visitors/printer.hpp"

/** Command to run */
enum egg_mode {
	PRINT_MODE,		/**< Print grammar */
	COMPILE_MODE	/**< Compile grammar */
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
		} else {
			return false;
		}
	}

	void parse_input(char* s) {
		in = new std::ifstream(s);
		if ( !nameFlag && out == 0 ) {
			pName = id_prefix(s);
		}
	}

	void parse_output(char* s) {
		out = new std::ofstream(s);
		if ( !nameFlag ) {
			pName = id_prefix(s);
		}
	}

	void parse_name(char* s) {
		pName = id_prefix(s);
		nameFlag = true;
	}

public:
	args(int argc, char** argv) {
		in = (std::ifstream*)0;
		out = (std::ofstream*)0;
		pName = std::string("");
		nameFlag = false;
		normFlag = true;
		eMode = COMPILE_MODE;

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
			} else if ( match("-c", "--command", argv[i]) ) {
				if ( i+1 >= argc ) return;
				if ( parse_mode(argv[++i]) ) { ++i; }
			} else if ( match("-n", "--name", argv[i]) ) {
				if ( i+1 >= argc ) return;
				parse_name(argv[++i]);
			} else if ( eq("--no-norm", argv[i]) ) {
				normFlag = false;
			} else break;
		}

		//parse optional input and output files
		if ( i < argc ) {
			parse_input(argv[i++]);
			if ( i < argc ) {
				parse_output(argv[i++]);
			}
		}
	}

	~args() {
		if ( in != 0 ) in->close();
	}

	std::istream& input() { if ( in ) return *in; else return std::cin; }
	std::ostream& output() { if ( out ) return *out; else return std::cout; }
	std::string name() { return pName; }
	bool norm() { return normFlag; }
	egg_mode mode() { return eMode; }

private:
	int i;				/**< next unparsed value */
	std::ifstream* in;	/**< pointer to input stream (0 for stdin) */
	std::ofstream* out;	/**< pointer to output stream (0 for stdout) */
	std::string pName;	/**< the name of the parser (empty if none) */
	bool nameFlag;		/**< has the parser name been explicitly set? */
	bool normFlag;      /**< should egg do grammar normalization? */
	egg_mode eMode;		/**< compiler mode to use */
};

/** Command line interface
 *  egg [command] [flags] [input-file [output-file]]
 *  
 *  Supported flags are
 *  -i --input		input file (default stdin)
 *  -o --output		output file (default stdout)
 *  -c --command	command - either compile or print (default compile)
 *  -n --name		grammar name - if none given, takes the longest prefix of 
 *  				the input or output file name (output preferred) which is a 
 *  				valid Egg identifier (default empty)
 *  --no-norm       turns off grammar normalization
 */
int main(int argc, char** argv) {

	args a(argc, argv);

	parse::state ps(a.input());
	ast::grammar_ptr g;
	
	if ( egg::grammar(ps)(g) ) {
		//std::cout << "DONE PARSING" << std::endl;
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
			c.compile(*g);
			break;
		}
		}
		
	} else {
		std::cerr << "PARSE FAILURE" << std::endl;
	}

	return 0;
} /* main() */

