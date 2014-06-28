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

#include <istream>
#include <map>
#include <string>
#include <unordered_set>
#include <utility>

#include "../ast.hpp"
#include "../derivs-mut.hpp"
#include "../utils/strings.hpp"
#include "../utils/uint_set.hpp"

#include "deriv_printer-mut.hpp"

namespace derivs {
	
	/// Loads a set of derivatives from the grammar AST
	class loader : ast::visitor {
		
		/// Gets the shared rule node correspoinding to the given name
		shared_node get_rule(const std::string& s) {
			if ( rs.count(s) == 0 ) {
				shared_node r{ std::move(expr::make<fail_node>()) };
				rs.emplace(s, r);
				return r;
			} else {
				return rs.at(s);
			}
		}
		
		/// Converts an AST char range into a derivative expr. char_range
		expr make_char_range(const ast::char_range& r) const {
			return ( r.from == r.to ) ? expr::make<char_node>(r.from)
			                          : expr::make<range_node>(r.from, r.to);
		}
		
		/// Makes a new anonymous nonterminal for a many-expression
		expr make_many(expr&& e) {
			// Make anonymous non-terminal R
			rule_node r{ std::move(expr{}) };
			// Build rule e R / eps for R
			shared_node s{std::move(
			                  expr::make<alt_node>(
			                      expr::make<seq_node>(std::move(e), r),
			                      expr::make<eps_node>()))};
			// switch new rule into r
			r.r = s;
			return r;
		}
		
	public:
		/// Default constructor; builds a derivative parser graph from the given PEG grammar
		loader(ast::grammar& g, bool dbg = false) {
			// Read in rules
			for (ptr<ast::grammar_rule> r : g.rs) {
				r->m->accept(this);
				get_rule(r->name).shared->e = std::move(rVal);
			}
			
			// Normalize rules
			expr_set normed;
			for (auto rp : rs) {
				rp.second.normalize(normed);
				names.emplace(rp.second.get(), rp.first);
			}
			normed.clear();
			
			// TODO down to here
			
			// Calculate fixed point of match() for all expressions
			derivs::fixer fix;
			for (auto rp : rs) {
				fix(rp.second);
			}
			
			if ( dbg ) {
				derivs::printer p(std::cout, names);
				for (auto rp : rs) {
					p.print(std::static_pointer_cast<derivs::expr>(rp.second));
				}
				std::cout << "\n***** DONE LOADING RULES  *****\n" << std::endl;
				
			}
		}
		
		/// Gets the rules from a grammar
		std::map<std::string, ptr<rule_expr>>& get_rules() { return rs; }
		
		/// Gets the memoization table that goes along with them
		memo_expr::table& get_memo() { return memo; }
		
		/// Gets the rule names for this grammar
		std::map<expr*, std::string>& get_names() { return names; }
		
		virtual void visit(ast::char_matcher& m) { rVal = expr::make_ptr<char_expr>(m.c); }
		
		virtual void visit(ast::str_matcher& m) { rVal = expr::make_ptr<str_expr>(m.s); }
		
		virtual void visit(ast::range_matcher& m) {
			// Empty alternation is a success
			if ( m.rs.size() == 0 ) { rVal = expr::make_ptr<eps_expr>(); return; }
			
			// Transform last option
			auto it = m.rs.rbegin();
			rVal = make_char_range(*it);
			
			// Transform remaining options
			while ( ++it != m.rs.rend() ) {
				auto tVal = make_char_range(*it);
				rVal = expr::make_ptr<alt_expr>(memo, tVal, rVal);
			}
		}
		
		virtual void visit(ast::rule_matcher& m) { rVal = get_rule(m.rule); }
		
		virtual void visit(ast::any_matcher& m) { rVal = expr::make_ptr<any_expr>(); }
		
		virtual void visit(ast::empty_matcher& m) { rVal = expr::make_ptr<eps_expr>(); }
		
		virtual void visit(ast::action_matcher& m) {
			//TODO actually implement this; for the moment, just return success
			rVal = expr::make_ptr<eps_expr>();
		}
		
		virtual void visit(ast::opt_matcher& m) {
			// match subexpression or epsilon
			m.m->accept(this);
			rVal = expr::make_ptr<alt_expr>(memo, rVal, expr::make_ptr<eps_expr>());
		}
		
		virtual void visit(ast::many_matcher& m) {
			m.m->accept(this);
			rVal = make_many(rVal);
		}
		
		virtual void visit(ast::some_matcher& m) {
			m.m->accept(this);
			rVal = expr::make_ptr<seq_expr>(memo, rVal, make_many(rVal));
		}
		
		virtual void visit(ast::seq_matcher& m) {
			// Convert vector in seq_matcher to cons-list in seq_expr
			
			// Empty sequence is a success
			if ( m.ms.size() == 0 ) { rVal = expr::make_ptr<eps_expr>(); return; }
			
			// Transform last option
			auto it = m.ms.rbegin();
			(*it)->accept(this);
			
			// Transform remaining options
			while ( ++it != m.ms.rend() ) {
				ptr<expr> tVal = rVal;
				(*it)->accept(this);
				rVal = expr::make_ptr<seq_expr>(memo, rVal, tVal);
			}
		}
		
		virtual void visit(ast::alt_matcher& m) {
			// Convert vector in alt_matcher to cons-list in alt_expr
			
			// Empty sequence is a success
			if ( m.ms.size() == 0 ) { rVal = expr::make_ptr<eps_expr>(); return; }
			
			// Transform last option
			auto it = m.ms.rbegin();
			(*it)->accept(this);
			
			// Transform remaining options
			while ( ++it != m.ms.rend() ) {
				ptr<expr> tVal = rVal;
				(*it)->accept(this);
				rVal = expr::make_ptr<alt_expr>(memo, rVal, tVal);
			}
		}
		
		virtual void visit(ast::look_matcher& m) {
			m.m->accept(this);
			rVal = expr::make_ptr<not_expr>(memo, expr::make_ptr<not_expr>(memo, rVal));
		}
		
		virtual void visit(ast::not_matcher& m) {
			m.m->accept(this);
			rVal = expr::make_ptr<not_expr>(memo, rVal);
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
			rVal = expr::make_ptr<fail_expr>();
		}
		
	private:
		std::map<std::string, shared_node> rs;     ///< List of rules
		std::map<expr*, std::string>       names;  ///< Names of rule expressions
		expr                               rVal;   ///< Return value of node visits
	};  // class loader
	
	/// Recognizes the input
	/// @param l		Loaded derivatives
	/// @param in		Input stream
	/// @param rule		Start rule
	/// @param dbg		Print debug output? (default false)
	/// @return true for match, false for failure
	bool match(loader& l, std::istream& in, std::string rule, bool dbg = false) {
		auto& rs = l.get_rules();
		auto& memo = l.get_memo();
		derivs::printer p(std::cout, l.get_names());
		
		// fail on no such rule
		if ( rs.count(rule) == 0 ) return false;
		
		ptr<expr> e = rs[rule]->r;
		
		// Take derivatives until failure, match, or end of input
		while ( true ) {
			if ( dbg ) { p.print(e); }
			
			switch ( e->type() ) {
			case fail_type: return false;
			case inf_type:  return false;
			case eps_type:  return true;
			case look_type: return true;
			default:        break; // do nothing
			}
			
			// Break on a match
			if ( ! e->match().empty() ) return true;
			
			char x;
			if ( ! in.get(x) ) { x = '\0'; }  // read character, \0 for EOF
			
			if ( dbg ) {
				std::cout << "d(\'" << (x == '\0' ? "\\0" : strings::escape(x)) << "\') =====>" 
				          << std::endl;
			}
			
			e = e->d(x);
			memo.clear(); // clear memoization table after every character
			
			if ( x == '\0' ) break;
		}
		if ( dbg ) { p.print(e); }
		
		// Match if final expression matched on terminator char
		return ! e->match().empty();
	}
	
	/// Recognizes the input
	/// @param g		Source grammar
	/// @param in		Input stream
	/// @param rule		Start rule
	/// @param dbg		Print debug output? (default false)
	/// @return true for match, false for failure
	bool match(ast::grammar& g, std::istream& in, std::string rule, bool dbg = false) {
		loader l(g, dbg);
		return match(l, in, rule, dbg);
	}
	
} // namespace derivs
