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

#include "deriv_printer.hpp"

namespace derivs {
	
	/// Rebuilds a set of derivative expressions using their smart constructors
	class normalizer : derivs::visitor {
	public:
		normalizer(memo_expr::table& memo) : rVal(nullptr), memo(memo), rs() {}
		
		/// Converts any expression
		ptr<expr> normalize(ptr<expr> e) {
			e->accept(this);
			return rVal;
		}
		
		/// Conversion specialized for rule expressions
		ptr<rule_expr> normalize(ptr<rule_expr> r) {
			r->accept(this);
			return std::static_pointer_cast<rule_expr>(rVal);
		}
		
		void visit(fail_expr& e)  { rVal = fail_expr::make(); }
		void visit(inf_expr& e)   { rVal = inf_expr::make(); }
		void visit(eps_expr& e)   { rVal = eps_expr::make(); }
		void visit(look_expr& e)  { rVal = look_expr::make(e.g); }
		void visit(char_expr& e)  { rVal = char_expr::make(e.c); }
		void visit(range_expr& e) { rVal = range_expr::make(e.b, e.e); }
		void visit(any_expr& e)   { rVal = any_expr::make(); }
		void visit(str_expr& e)   { rVal = str_expr::make(e.str()); }
		
		void visit(rule_expr& e) {
			auto it = rs.find(&e);
			if ( it == rs.end() ) {
				// no stored rule; store one, then update its pointed to rule
				ptr<rule_expr> r = std::make_shared<rule_expr>(memo, inf_expr::make());
				rs./*emplace(&e, r)*/insert(std::make_pair(&e, r));
				e.r->accept(this);
				r->r = rVal;
				r->reset_memo();
				rVal = std::static_pointer_cast<expr>(r);
			} else {
				rVal = std::static_pointer_cast<expr>(it->second);
			}
		}
		
		void visit(not_expr& e) {
			e.e->accept(this);
			rVal = not_expr::make(memo, rVal);
		}
		
		void visit(and_expr& e) {
			e.e->accept(this);
			rVal = and_expr::make(memo, rVal);
		}
		
		void visit(map_expr& e) {
			// won't appear in un-normalized expression anyway
			e.e->accept(this);
			rVal = map_expr::make(memo, rVal, e.eg);
		}
		
		void visit(alt_expr& e) {
			e.a->accept(this);
			ptr<expr> a = rVal;
			e.b->accept(this);
			rVal = alt_expr::make(memo, a, rVal);
		}
		
		void visit(seq_expr& e) {
			e.a->accept(this);
			ptr<expr> a = rVal;
			e.b->accept(this);
			rVal = seq_expr::make(memo, a, rVal);
		}
		
		void visit(back_expr& e) {
			// won't appear in un-normalized expression anyway
			e.a->accept(this);
			ptr<expr> a = rVal;
			e.b->accept(this);
			ptr<expr> b = rVal;
			
			back_expr::look_list bs;
			for (auto bi : e.bs) {
				bi.e->accept(this);
				/*bs.emplace_back(bi.g, bi.eg, rVal);*/
				back_expr::look_node bn(bi.g, bi.eg, rVal);
				bs.push_back(bn);
			}
			
			rVal = expr::make_ptr<back_expr>(memo, a, b, bs);
		}
	
	private:
		ptr<expr>                            rVal;  ///< result of last read
		memo_expr::table&                    memo;  ///< Memoization table
		std::map<rule_expr*, ptr<rule_expr>> rs;    ///< Unique transformation of rule expressions
	};  // class normalizer
	
	/// Loads a set of derivatives from the grammar AST
	class loader : ast::visitor {
		
		/// Gets the unique rule expression correspoinding to the given name
		ptr<rule_expr> get_rule(const std::string& s) {
			if ( rs.count(s) == 0 ) {
				ptr<rule_expr> r = std::make_shared<rule_expr>(memo, expr::make_ptr<fail_expr>());
				rs./*emplace(s, r)*/insert(std::make_pair(s, r));
				return r;
			} else {
				return rs.at(s);
			}
		}
		
		/// Converts an AST char range into a derivative expr. char_range
		ptr<expr> make_char_range(const ast::char_range& r) const {
			return ( r.from == r.to ) ? 
				expr::make_ptr<char_expr>(r.from) : 
				expr::make_ptr<range_expr>(r.from, r.to);
		}
		
		/// Makes a new anonymous nonterminal for a many-expression
		ptr<expr> make_many(ptr<expr> e) {
			// make anonymous non-terminal R
			ptr<expr> r = expr::make_ptr<rule_expr>(memo, expr::make_ptr<fail_expr>());
			// set non-terminal rule to e R / eps
			std::static_pointer_cast<rule_expr>(r)->r = 
				expr::make_ptr<alt_expr>(memo, 
				                         expr::make_ptr<seq_expr>(memo, e, r),
				                         expr::make_ptr<eps_expr>());
			return r;
		}
		
	public:
		/// Default constructor; builds a derivative parser graph from the given PEG grammar
		loader(ast::grammar& g, bool dbg = false) {
			// Read in rules
			for (ptr<ast::grammar_rule> r : g.rs) {
				rVal = nullptr;
				r->m->accept(this);
				get_rule(r->name)->r = rVal;
				
				if ( dbg ) {
					std::cout << "LOADED RULE `" << r->name << "`" << std::endl;
					derivs::printer::print(std::cout, rVal);
					std::cout << std::endl;
				}
			}
			
			if ( dbg ) { std::cout << "***** DONE LOADING RULES  *****" << std::endl; }
			rVal = nullptr;
			
			// Normalize rules
			normalizer n(memo);
			std::map<std::string, ptr<rule_expr>> nrs;
			for (auto rp : rs) {
				ptr<rule_expr> nr = n.normalize(rp.second);
				nrs.insert(std::make_pair(rp.first, nr));
				
				if ( dbg ) {
					std::cout << "NORMED RULE `" << rp.first << "`" << std::endl;
					derivs::printer::print(std::cout, nr);
					std::cout << std::endl;
				}
			}
			rs.swap(nrs);
			if ( dbg ) { std::cout << "***** DONE NORMING RULES  *****" << std::endl; }
		}
		
		/// Gets the rules from a grammar
		std::map<std::string, ptr<rule_expr>>& get_rules() { return rs; }
		
		/// Gets the memoization table that goes along with them
		memo_expr::table& get_memo() { return memo; }
		
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
			rVal = expr::make_ptr<and_expr>(memo, rVal);
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
		std::map<std::string, ptr<rule_expr>>  rs;    ///< List of rules
		memo_expr::table                       memo;  ///< Memoization table
		ptr<expr>                              rVal;  ///< Return value of node visits
	};  // class loader
	
	/// Recognizes the input
	/// @param l		Loaded derivatives
	/// @param ps		Parser state
	/// @param rule		Start rule
	/// @param dbg		Print debug output? (default false)
	/// @return true for match, false for failure
	bool match(loader& l, parser::state& ps, std::string rule, bool dbg = false) {
		auto& rs = l.get_rules();
		auto& memo = l.get_memo();
		
		// fail on no such rule
		if ( rs.count(rule) == 0 ) return false;
		
		ptr<expr> e = rs[rule]->r;
		
		// Take derivatives until failure, match, or end of input
		while ( true ) {
			if ( dbg ) { derivs::printer::print(std::cout, e); }
			
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
			if ( dbg ) { std::cout << "d(\'" << x << "\') =====>" << std::endl; }
			
			e = e->d(x);
			memo.clear(); // clear memoization table after every character
			++ps;
		}
		
		// Match if final expression does not demand more characters
		return ( e->nbl() != SHFT );
	}
	
	/// Recognizes the input
	/// @param g		Source grammar
	/// @param ps		Parser state
	/// @param rule		Start rule
	/// @param dbg		Print debug output? (default false)
	/// @return true for match, false for failure
	bool match(ast::grammar& g, parser::state& ps, std::string rule, bool dbg = false) {
		loader l(g, dbg);
		return match(l, ps, rule, dbg);
	}
	
} // namespace derivs
