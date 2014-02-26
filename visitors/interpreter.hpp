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

#include <map>
#include <string>
#include <utility>

#include "../ast.hpp"
#include "../derivs.hpp"
#include "../parser.hpp"

namespace derivs {
	
	/// Loads a set of derivatives from the grammar AST
	class loader : ast::visitor {
		
		/// Converts an AST char range into a derivative expr. char_range
		ptr<expr> make_char_range(const ast::char_range& r) const {
			return ( r.from == r.to ) ? 
				char_expr::make(r.from) : 
				range_expr::make(r.from, r.to);
		}
		
		/// Makes a new anonymous nonterminal for a many-expression
		ptr<expr> make_many(ptr<expr> e) {
			// make anonymous non-terminal R
			ptr<expr> r = rule_expr::make(memo, ptr<expr>(nullptr));
			// set non-terminal rule to e R / eps
			std::static_pointer_cast<rule_expr>(r)->r = 
				alt_expr::make(memo, 
				               seq_expr::make(memo, e, r),
				               eps_expr::make());
			return r;
		}
		
	public:
		/// Default constructor; builds a derivative parser graph from the given PEG grammar
		loader(ast::grammar& g) {
			// Read in rules
			for (ptr<ast::grammar_rule> r : g.rs) {
				rVal = nullptr;
				r->m->accept(this);
				rs[r->name] = rVal;
			}
			rVal = nullptr;
			
			// Update rule expressions to have single rule expression
			for (std::pair<std::string, ptr<rule_expr>> rn : rns) rn.second->r = rs[rn.first];
			rns.clear();
		}
		
		/// Gets the rules from a grammar
		std::map<std::string, ptr<expr>>& get_rules() { return rs; }
		
		/// Gets the memoization table that goes along with them
		memo_expr::table&                 get_memo()  { return memo; }
		
		virtual void visit(ast::char_matcher& m)  { rVal = char_expr::make(m.c); }
		
		virtual void visit(ast::str_matcher& m)   { rVal = str_expr::make(m.s); }
		
		virtual void visit(ast::range_matcher& m) {
			// Empty alternation is a success
			if ( m.rs.size() == 0 ) { rVal = eps_expr::make(); return; }
			
			// Transform last option
			auto it = m.rs.rbegin();
			rVal = make_char_range(*it);
			
			// Transform remaining options
			while ( ++it != m.rs.rend() ) {
				auto tVal = make_char_range(*it);
				rVal = alt_expr::make(memo, tVal, rVal);
			}
		}
		
		virtual void visit(ast::rule_matcher& m) {
			// Make sure all instances of the same rule matcher are actually the same object
			if ( rns.count(m.rule) == 0 ) {
				// Insert a new rule with a placeholder subexpression
				rVal = rule_expr::make(memo, ptr<expr>(nullptr));
				rns/*.emplace(m.rule, rVal)*/
					.insert(std::make_pair(m.rule, 
					                       std::static_pointer_cast<rule_expr>(rVal)));
			} else {
				rVal = rns.at(m.rule);
			}
		}
		
		virtual void visit(ast::any_matcher& m) { rVal = any_expr::make(); }
		
		virtual void visit(ast::empty_matcher& m) { rVal = eps_expr::make(); }
		
		virtual void visit(ast::action_matcher& m) {
			//TODO actually implement this; for the moment, just return success
			rVal = eps_expr::make();		
		}
		
		virtual void visit(ast::opt_matcher& m) {
			// match subexpression or epsilon
			m.m->accept(this);
			rVal = alt_expr::make(memo, rVal, eps_expr::make());
		}
		
		virtual void visit(ast::many_matcher& m) {
			m.m->accept(this);
			rVal = make_many(rVal);
		}
		
		virtual void visit(ast::some_matcher& m) {
			m.m->accept(this);
			rVal = seq_expr::make(memo, rVal, make_many(rVal));
		}
		
		virtual void visit(ast::seq_matcher& m) {
			// Convert vector in seq_matcher to cons-list in seq_expr
			
			// Empty sequence is a success
			if ( m.ms.size() == 0 ) { rVal = eps_expr::make(); return; }
			
			// Transform last option
			auto it = m.ms.rbegin();
			(*it)->accept(this);
			
			// Transform remaining options
			while ( ++it != m.ms.rend() ) {
				ptr<expr> tVal = rVal;
				(*it)->accept(this);
				rVal = seq_expr::make(memo, rVal, tVal);
			}
		}
		
		virtual void visit(ast::alt_matcher& m) {
			// Convert vector in alt_matcher to cons-list in alt_expr
			
			// Empty sequence is a success
			if ( m.ms.size() == 0 ) { rVal = eps_expr::make(); return; }
			
			// Transform last option
			auto it = m.ms.rbegin();
			(*it)->accept(this);
			
			// Transform remaining options
			while ( ++it != m.ms.rend() ) {
				ptr<expr> tVal = rVal;
				(*it)->accept(this);
				rVal = alt_expr::make(memo, rVal, tVal);
			}
		}
		
		virtual void visit(ast::look_matcher& m) {
			m.m->accept(this);
			rVal = and_expr::make(memo, rVal);
		}
		
		virtual void visit(ast::not_matcher& m) {
			m.m->accept(this);
			rVal = not_expr::make(memo, rVal);
		}
		
		virtual void visit(ast::capt_matcher& m) {
			// TODO implement this; for the moment, just ignore the capture
			m.m->accept(this);
		}
		
		virtual void visit(ast::named_matcher& m) {
			// TODO implment this; for the moment, just ignore the error message
			m.m->accept(this);
		}
		
		virtual void visit(ast::fail_matcher& m) {
			// TODO complete implementation; for the moment ignore the error message
			rVal = fail_expr::make();
		}
		
	private:
		std::map<std::string, ptr<expr>>           rs;    ///< List of rules
		memo_expr::table                           memo;  ///< Memoization table
		std::map<std::string, ptr<rule_expr>>      rns;   ///< Rule nodes for subexpression update
		ptr<expr>                                  rVal;  ///< Return value of node visits
	};  // class loader
	
	/// Derivative-based interpreter for parsing expression grammars
	class interpreter {
	public:
		/// Sets up an interpreter for the given rules
		interpreter(std::map<std::string, ptr<expr>>& rs, memo_expr::table& memo) 
			: rs(rs), memo(memo) { memo.clear(); }
		
		/// Loads the given grammar into the interpreter
		static interpreter from_ast(ast::grammar& g) {
			loader l(g);
			interpreter i(l.get_rules(), l.get_memo());
			return i;
		}
		
		/// Recognizes the input, returns true for match, false for failure
		bool match(parser::state& ps, std::string rule) {
			// fail on no such rule
			if ( rs.count(rule) == 0 ) return false;
			
			ptr<expr> e = rs[rule];
			
			// Take derivatives until failure, match, or end of input
			while ( true ) {
				switch ( e->type() ) {
				case fail_type: return false;
				case inf_type:  return false;
				case eps_type:  return true;
				case look_type: return true;
				default:        break; // do nothing
				}
				if ( e->nbl() == NBL ) return true;
				
				char x = ps();
				if ( x == '\0' ) break;
				
				e = e->d(x);
				memo.clear(); // clear memoization table after every character
			}
			
			// Match if final expression does not demand more characters
			return ( e->nbl() != SHFT );
		}
		
	private:
		std::map<std::string, ptr<expr>>& rs;    ///< List of rules
		memo_expr::table&                 memo;  ///< Memoization table
	};  // class interpreter
	
} // namespace derivs

