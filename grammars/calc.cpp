#include <iostream>
#include <sstream>

#include "calc.hpp"

/**
 * Test harness for calculator grammar.
 * @author Aaron Moss
 */
int main(int argc, char** argv) {
	using namespace std;
	
	string s;
	while ( getline(cin, s) ) {
		stringstream ss(s);
		parse::state ps(ss);
		parse::result<int> res = calc::sum(ps);
		if ( res ) cout << *res << endl;
		else cout << "PARSE FAILURE `" << s << "'" << endl;
	}
}

