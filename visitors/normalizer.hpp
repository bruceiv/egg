#pragma once

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

#include "../ast.hpp"

namespace visitor {

	/** Normalizes an Egg AST */
	class normalizer : ast::visitor {
	public:
		void visit(ast::char_matcher& m) {
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::char_matcher>(m));
		}

		void visit(ast::str_matcher& m) {
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::str_matcher>(m));
		}

		void visit(ast::range_matcher& m) {
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::range_matcher>(m));
		}

		void visit(ast::rule_matcher& m) {
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::rule_matcher>(m));
		}

		void visit(ast::any_matcher& m) {
			rVal = ast::make_ptr<ast::any_matcher>();
		}
		
		void visit(ast::empty_matcher& m) {
			rVal = ast::make_ptr<ast::empty_matcher>();
		}

		void visit(ast::action_matcher& m) {
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::action_matcher>(m));
		}
		
		void visit(ast::opt_matcher& m) {
			m.m->accept(this);
			m.m = rVal;
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::opt_matcher>(m));
		}

		void visit(ast::many_matcher& m) {
			m.m->accept(this);
			m.m = rVal;
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::many_matcher>(m));
		}

		void visit(ast::some_matcher& m) {
			m.m->accept(this);
			m.m = rVal;
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::some_matcher>(m));
		}

		void visit(ast::seq_matcher& m) {
			switch( m.ms.size() ) {
			case 0:
				rVal = ast::make_ptr<ast::empty_matcher>();
				break;
			case 1:
				m.ms.front()->accept(this);
				// rVal = rVal;
				break;
			default:
				ast::seq_matcher_ptr p = ast::make_ptr<ast::seq_matcher>();
				for (auto it = m.ms.begin(); it != m.ms.end(); ++it) {
					(*it)->accept(this);
					*p += rVal;
				}
				rVal = ast::as_ptr<ast::matcher>(p);
				break;
			}
		}
		
		void visit(ast::alt_matcher& m) {
			switch( m.ms.size() ) {
			case 0:
				rVal = ast::make_ptr<ast::empty_matcher>();
				break;
			case 1:
				m.ms.front()->accept(this);
				// rVal = rVal;
				break;
			default:
				ast::alt_matcher_ptr p = ast::make_ptr<ast::alt_matcher>();
				for (auto it = m.ms.begin(); it != m.ms.end(); ++it) {
					(*it)->accept(this);
					*p += rVal;
				}
				rVal = ast::as_ptr<ast::matcher>(p);
				break;
			}
		}

		void visit(ast::look_matcher& m) {
			m.m->accept(this);
			m.m = rVal;
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::look_matcher>(m));
		}

		void visit(ast::not_matcher& m) {
			m.m->accept(this);
			m.m = rVal;
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::not_matcher>(m));
		}

		void visit(ast::capt_matcher& m) {
			m.m->accept(this);
			m.m = rVal;
			rVal = ast::as_ptr<ast::matcher>(
					ast::make_ptr<ast::capt_matcher>(m));
		}

		ast::grammar_rule& normalize(ast::grammar_rule& r) {
			r.m->accept(this);
			r.m = rVal;
			return r;
		}

		ast::grammar& normalize(ast::grammar& g) {
			for (auto it = g.rs.begin(); it != g.rs.end(); ++it) {
				ast::grammar_rule_ptr& r = *it;
				*r = normalize(*r);
			}
			return g;
		}
	
	private:
		/** The matcher to return for the current visit */
		ast::matcher_ptr rVal;
	}; /* class visitor */
	
} /* namespace visitor */

