/*
 * Copyright (c) 2014 Aaron Moss
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
#include <iostream>

#include "parser.hpp"
#ifdef NOMEMO
  #ifdef UNT
    #include "java-nomemo-unt.hpp"
  #else 
    #include "java-nomemo.hpp"
  #endif
#else
  #ifdef UNT
    #include "java-unt.hpp"
  #else
    #include "java.hpp"
  #endif
#endif

int main(int argc, char** argv) {
	using namespace std;
	
	parser::state ps(cin);
	
	if ( java::CompilationUnit(ps) ) {
		cout << "MATCH" << endl;
	} else {
		const parser::error& err = ps.error();
			
		cout << "DOESN'T MATCH  @" << err.pos.line() << ":" << err.pos.col() << endl;
		for (auto msg : err.messages) {
			cout << "\t" << msg << endl;
		}
		for (auto exp : err.expected) {
			cout << "\tExpected " << exp << endl;
		}
	}

	#ifdef EGG_INST
	cout << "," << ps.max_backtracks() << "," << ps.max_nesting_depth() << "," << endl;
	#endif
}
