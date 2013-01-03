#include <fstream>
#include <iostream>
#include <string>

#include "egg.hpp"
#include "parse.hpp"
#include "visitors/printer.hpp"
#include "visitors/normalizer.hpp"

class args {
public:
	args(int argc, char** argv) {
		in = (std::ifstream*)0;
		
		for (i = 1; i < argc; ++i) {
			if ( argv[i][0] == '-' ) {
				switch (argv[i][1]) {
				case 'i':
					if ( i+1 >= argc ) return;
					in = new std::ifstream(argv[++i]);
					break;
				default: return;
				}
			} else return;
		}
	}

	~args() {
		if ( in != 0 ) in->close();
	}

	std::istream* input() { return ( in == 0 ) ? &std::cin : in; }

private:
	int i;				/**< next unparsed value */
	std::ifstream* in;	/**< pointer to input stream (0 for stdin) */
};

int main(int argc, char** argv) {

	args a(argc, argv);

	parse::state ps(*a.input());
	ast::grammar_ptr g;
	
	if ( egg::grammar(ps)(g) ) {
		//std::cout << "DONE PARSING" << std::endl;
		visitor::normalizer n;
		n.normalize(*g);
		visitor::printer p;
		p.print(*g);
	} else {
		std::cout << "PARSE FAILURE" << std::endl;
	}

	return 0;
} /* main() */

