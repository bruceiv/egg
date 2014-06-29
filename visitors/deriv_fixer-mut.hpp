#pragma once

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

#include "../derivs-mut.hpp"

namespace derivs {
	
	/// Calculates least fixed point of match sets for compound expressions and stores them in the 
	/// memo table. Approach based on Kleene's fixed point theorem, as implemented in the 
	/// derivative parser of Might et al. for CFGs.
	class fixer : public visitor {
	public:
		/// Calculates the least fixed point of x.match() and memoizes it
		void operator() (expr& x);
		/// Convenience call for shared_node
		void operator() (shared_node& x);
		
		// Implements visitor
		void visit(fail_node&);
		void visit(inf_node&);
		void visit(eps_node&);
		void visit(look_node&);
		void visit(char_node&);
		void visit(range_node&);
		void visit(any_node&);
		void visit(str_node&);
		void visit(shared_node&);
		void visit(rule_node&);
		void visit(not_node&);
		void visit(map_node&);
		void visit(alt_node&);
		void visit(seq_node&);
	
	private:
		/// Performs fixed point computation to calculate match set of x
		/// Based on Kleene's thm, iterates upward from a bottom set of {}
		gen_set fix_match(expr& x);
		
		/// Recursively calculates next iteration of match set
		/// @param x        The expression to match
		/// @param changed  Did some subexpression change its value for match?
		/// @param visited  Which subexpressions have been visited?
		/// @return The match set
		gen_set iter_match(expr& x, bool& changed, expr_set& visited);
		
		/// Wraps visitor pattern for actual calculation of next match set
		/// @param x        The expression to match
		/// @param changed  Did some subexpression change its value for match?
		/// @param visited  Which subexpressions have been visited?
		/// @return The match set
		gen_set calc_match(expr& x, bool& changed, expr_set& visited);
		
		expr_set  running;  ///< Set of expressions currently being fixed
		expr_set  fixed;    ///< Set of expressions already fixed
		expr_set* visited;  ///< Set of expressions visited in current fix
		bool*     changed;  ///< Has anything in the current fix changed?
		gen_set   match;    ///< Match returned by current fix
	}; // class fixer	
	
} // namespace derivs
