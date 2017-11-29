#pragma once

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

#include <algorithm>

#include "../derivs.hpp"

namespace derivs {

	/// Visitor to instrument derivative parser tree with statistical data
	class instrumenter : visitor {
		/// Update bookkeeping when a leaf-level expression is seen
		void base_expr() {
			max_depth = std::max( crnt_depth + 1, max_depth );
			++crnt_leaves;
		}

	public:
		instrumenter()
			: crnt_depth(0), max_depth(0), crnt_leaves(0), max_leaves(0) {}
		
		/// Feed a new expression tree to the instrumenter
		void operator() ( expr& e ) {
			crnt_depth = 0;
			crnt_leaves = 0;
			e.accept( this );
			max_leaves = std::max( crnt_leaves, max_leaves );
		}

		/// Report maximum nesting depth seen
		unsigned max_nesting_depth() const { return max_depth; }

		/// Report maximum simultaneous backtracking options seen
		unsigned max_backtracks() const { return max_leaves; }

		/// Reset to initial state
		void reset() {
			crnt_depth = 0;
			max_depth = 0;
			crnt_leaves = 0;
			max_leaves = 0;
		}

		void visit(fail_expr& e)   { base_expr(); }
		void visit(inf_expr& e)    { base_expr(); }
		void visit(eps_expr& e)    { base_expr(); }
		void visit(char_expr& e)   { base_expr(); }
		void visit(except_expr& e) { base_expr(); }
		void visit(range_expr& e)  { base_expr(); }
		void visit(except_range_expr& e) { base_expr(); }
		void visit(any_expr& e)   { base_expr(); }
		void visit(none_expr& e)  { base_expr(); }
		void visit(str_expr& e)   { base_expr(); }
		// void visit(rule_expr& e)  { base_expr(); }
		
		void visit(not_expr& e)   {
			++crnt_depth;
			e.e->accept( this );
			--crnt_depth;
		}

		void visit(alt_expr& e)   {
			++crnt_depth;
			for( auto& x : e.es ) x->accept( this );
			--crnt_depth;
		}

		void visit(opt_expr& e)   {
			++crnt_depth;
			e.e->accept( this );
			--crnt_depth;
		}

		void visit(or_expr& e)  {
			++crnt_depth;
			for( auto& x : e.es ) x->accept( this );
			--crnt_depth;
		}

		void visit(and_expr& e) {
			++crnt_depth;
			for( auto& x : e.es ) x->accept( this );
			--crnt_depth;
		}

		void visit(seq_expr& e)   {
			++crnt_depth;
			e.a->accept( this );
			// NOTE: doesn't instrument un-normalized expressions.
			// This is (likely?) fine.
			for( auto& b : e.bs ) b.e->accept( this );
			--crnt_depth;
		}
	
	private:
		unsigned crnt_depth;   ///< Current nesting depth
		unsigned max_depth;    ///< Maximum nesting depth seen
		unsigned crnt_leaves;  ///< Current count of backtracks
		unsigned max_leaves;   ///< Maximum count of backtracks seen
	};

}
