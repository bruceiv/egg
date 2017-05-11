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
#include <utility>

#include "../ast.hpp"
#include "../derivs.hpp"
#include "../parser.hpp"
#include "../utils/strings.hpp"
#include "../utils/uint_set.hpp"

#include "deriv_printer.hpp"

namespace derivs {
	
	/// Rebuilds a set of derivative expressions using their smart constructors
	class normalizer : derivs::visitor {
	public:
		normalizer() = default;
		
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
		void visit(look_expr& e)  { rVal = look_expr::make(e.b); }
		void visit(char_expr& e)  { rVal = char_expr::make(e.c); }
		void visit(except_expr& e) { rVal = except_expr::make(e.c); }
		void visit(range_expr& e) { rVal = range_expr::make(e.b, e.e); }
		void visit(except_range_expr& e) { rVal = except_range_expr::make(e.b, e.e); }
		void visit(any_expr& e)   { rVal = any_expr::make(); }
		void visit(none_expr& e)  { rVal = none_expr::make(); }
		void visit(str_expr& e)   { rVal = str_expr::make(e.str()); }
		
		void visit(rule_expr& e) {
			auto it = rs.find(&e);
			if ( it == rs.end() ) {
				// no stored rule; store one, then update its pointed to rule
				ptr<rule_expr> r = std::make_shared<rule_expr>(inf_expr::make());
				rs.emplace(&e, r)/*insert(std::make_pair(&e, r))*/;
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
			rVal = not_expr::make(rVal);
		}
		
		void visit(map_expr& e) {
			// won't appear in un-normalized expression anyway
			e.e->accept(this);
			rVal = map_expr::make(rVal, e.gm, e.eg);
		}
		
		void visit(alt_expr& e) {
			expr_list es;
			for (auto& x : e.es) {
				x.e->accept(this);
				es.emplace_back(rVal);
			}
			rVal = alt_expr::make(es);
		}

		void visit(or_expr& e) {
			expr_list es;
			for (auto& x : e.es) {
				x->accept(this);
				es.emplace_back(rVal);
			}
			rVal = or_expr::make(std::move(es));
		}

		void visit(and_expr& e) {
			expr_list es;
			for (auto& x : e.es) {
				x->accept(this);
				es.emplace_back(rVal);
			}
			rVal = and_expr::make(std::move(es));
		}
		
		void visit(seq_expr& e) {
			e.a->accept(this);
			ptr<expr> a = rVal;
			e.b->accept(this);
			rVal = seq_expr::make(a, rVal);
		}
	
	private:
		ptr<expr>                            rVal;  ///< result of last read
		std::map<rule_expr*, ptr<rule_expr>> rs;    ///< Unique transformation of rule expressions
	};  // class normalizer
	
	/// Loads a set of derivatives from the grammar AST
	class loader : ast::visitor {
		
		/// Gets the unique rule expression correspoinding to the given name
		ptr<rule_expr> get_rule(const std::string& s) {
			if ( rs.count(s) == 0 ) {
				ptr<rule_expr> r = std::make_shared<rule_expr>(fail_expr::make());
				rs.emplace(s, r)/*insert(std::make_pair(s, r))*/;
				return r;
			} else {
				return rs.at(s);
			}
		}
		
		/// Converts an AST char range into a derivative expr char_range
		ptr<expr> make_char_range(const ast::char_range& r, bool neg) const {
			return ( r.from == r.to ) ? 
				neg ? 
					expr::make_ptr<except_expr>(r.from) :
					expr::make_ptr<char_expr>(r.from) : 
				neg ?
					expr::make_ptr<except_range_expr>(r.from, r.to) :
					expr::make_ptr<range_expr>(r.from, r.to);
		}
		
		/// Makes a new anonymous nonterminal for a many-expression
		ptr<expr> make_many(ptr<expr> e) {
			// make anonymous non-terminal R
			ptr<expr> r = expr::make_ptr<rule_expr>(fail_expr::make());
			// set non-terminal rule to e R / eps
			std::static_pointer_cast<rule_expr>(r)->r = 
				expr::make_ptr<alt_expr>(expr::make_ptr<seq_expr>(e, r),
				                         eps_expr::make());
			return r;
		}

		/// Makes a new anonymous nonterminal for an until-expression
		ptr<expr> make_until(ptr<expr> e, ptr<expr> t) {
			// make anonymous nonterminal R
			ptr<expr> r = expr::make_ptr<rule_expr>(fail_expr::make());
			// set non-terminal rule to t / e R
			std::static_pointer_cast<rule_expr>(r)->r =
				expr::make_ptr<alt_expr>(t, expr::make_ptr<seq_expr>(e, r));
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
			}
			
			rVal = nullptr;
			
			// Normalize rules
			normalizer n;
			std::map<std::string, ptr<rule_expr>> nrs;
			for (auto rp : rs) {
				ptr<rule_expr> nr = n.normalize(rp.second);
				nrs.insert(std::make_pair(rp.first, nr));
				names.insert(std::make_pair(nr->r.get(), rp.first));
			}
			rs.swap(nrs);
			
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
		
		/// Gets the rule names for this grammar
		std::map<expr*, std::string>& get_names() { return names; }
		
		virtual void visit(ast::char_matcher& m) { rVal = expr::make_ptr<char_expr>(m.c); }
		
		virtual void visit(ast::str_matcher& m) { rVal = expr::make_ptr<str_expr>(m.s); }
		
		virtual void visit(ast::range_matcher& m) {
			// Empty alternation is a success or failure, depending on sense
			if ( m.rs.size() == 0 ) {
				rVal = m.neg ? fail_expr::make() :  eps_expr::make();
				return;
			}
			
			// Transform last option
			auto it = m.rs.rbegin();
			rVal = make_char_range(*it, m.neg);
			if ( m.rs.size() == 1 ) return;
			
			// Transform remaining options
			expr_list rs{ rVal };
			while ( ++it != m.rs.rend() ) {
				rs.push_back( make_char_range(*it, m.neg) );
			}
			rVal = m.neg ? expr::make_ptr<and_expr>( std::move(rs) ) :
					expr::make_ptr<or_expr>( std::move(rs) );
		}
		
		virtual void visit(ast::rule_matcher& m) { rVal = get_rule(m.rule); }
		
		virtual void visit(ast::any_matcher& m) { rVal = expr::make_ptr<any_expr>(); }

		virtual void visit(ast::none_matcher& m) { rVal = expr::make_ptr<none_expr>(); }
		
		virtual void visit(ast::empty_matcher& m) { rVal = eps_expr::make(); }
		
		virtual void visit(ast::action_matcher& m) {
			//TODO actually implement this; for the moment, just return success
			rVal = eps_expr::make();
		}
		
		virtual void visit(ast::opt_matcher& m) {
			// match subexpression or epsilon
			m.m->accept(this);
			rVal = expr::make_ptr<alt_expr>(rVal, eps_expr::make());
		}
		
		virtual void visit(ast::many_matcher& m) {
			m.m->accept(this);
			rVal = make_many(rVal);
		}
		
		virtual void visit(ast::some_matcher& m) {
			m.m->accept(this);
			rVal = expr::make_ptr<seq_expr>(rVal, make_many(rVal));
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
				rVal = expr::make_ptr<seq_expr>(rVal, tVal);
			}
		}
		
		virtual void visit(ast::alt_matcher& m) {
			// Empty sequence is a success
			if ( m.ms.size() == 0 ) { rVal = eps_expr::make(); return; }
			
			// Transform options to expression list
			expr_list es;
			for (auto& mi : m.ms) {
				mi->accept(this);
				es.emplace_back(rVal);
			}
			rVal = alt_expr::make(es);
		}

		virtual void visit(ast::until_matcher& m) {
			m.r->accept(this);
			ptr<expr> r = rVal;
			m.t->accept(this);
			rVal = make_until(r, rVal);
		}
		
		virtual void visit(ast::look_matcher& m) {
			m.m->accept(this);
			rVal = expr::make_ptr<not_expr>(expr::make_ptr<not_expr>(rVal));
		}
		
		virtual void visit(ast::not_matcher& m) {
			m.m->accept(this);
			rVal = expr::make_ptr<not_expr>(rVal);
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
		std::map<std::string, ptr<rule_expr>>  rs;     ///< List of rules
		std::map<expr*, std::string>           names;  ///< Names of rule expressions
		ptr<expr>                              rVal;   ///< Return value of node visits
	};  // class loader
	
	/// Recognizes the input
	/// @param l		Loaded derivatives
	/// @param in		Input stream
	/// @param rule		Start rule
	/// @param dbg		Print debug output? (default false)
	/// @return true for match, false for failure
	bool match(loader& l, std::istream& in, std::string rule, bool dbg = false) {
		auto& rs = l.get_rules();
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

