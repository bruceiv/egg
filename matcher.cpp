/*
 * Copyright (c) 2017 Aaron Moss
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

#include "derivs.hpp"
#include "matcher.hpp"
#include "norm.hpp"

#include "visitors/deriv_printer.hpp"
#include "visitors/instrumenter.hpp"

namespace derivs {
	norm nrm = {};

	bool match( const ast::grammar& gram, std::istream& in, const std::string& rule, 
			bool dbg, instrumenter* stats ) {
		/// Set global grammar state
		nrm.reset( gram );

		/// Get start rule
		ptr<expr> e = nrm( rule );
		gen_type g = 0;

		// Take derivatives until failure, match, or end-of-input
		while ( true ) {
			// if ( dbg ) { derivs::printer{}.print( e ); }
			if ( dbg ) dbgprt( e );
			if ( stats ) { (*stats)(*e); }

			switch ( e->type() ) {
			case fail_type: return false;
			case inf_type: return false;
			case eps_type: return true;
			default: break;
			}

			// Break on a match
			if ( ! e->match().empty() ) return true;

			// take next character, \0 for EOF
			char x;
			if ( ! in.get(x) ) { x = '\0'; }

			if ( dbg ) {
				std::cout << "d(\'" << (x == '\0' ? "\\0" : strings::escape(x)) 
					<< "\', " << g+1 << ") =====>" << std::endl;
			}

			e = e->d(x, ++g);  // take derivative

			if ( x == '\0' ) break;
		}
		if ( dbg ) { derivs::printer{}.print( e ); }
		if ( stats ) { (*stats)(*e); }

		// Match if final expression matched on terminator char
		return ! e->match().empty();
	}
}