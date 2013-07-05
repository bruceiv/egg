#include <iostream>
#include <sstream>

#include "abc.hpp"

/**
 * Test harness for abc grammar.
 * @author Aaron Moss
 */
int main(int argc, char** argv) {
	using namespace std;
	
	string s;
	while ( getline(cin, s) ) {
		stringstream ss(s);
		parse::state ps(ss);
		cout << "`" << s << "' " << (abc::g1(ps) ? "MATCHES" : "DOESN'T MATCH") << endl;
	}
}

