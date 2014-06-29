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
#include "deriv_fixer-mut.hpp"

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
			// Build the outer rule e R / eps
			rule_node r{std::move(
			                expr::make<alt_node>(
			                    expr::make<seq_node>(
			                        std::move(e),
			                        std::move(expr::make<rule_node>(std::move(expr{})))),
			                    std::move(expr::make<eps_node>())))};
			// Rebind inner rule to outer rule
			*static_cast<rule_node*>(
					static_cast<seq_node*>(
							static_cast<alt_node*>(
									r.r.shared->e.get()
								)->a.get()
						)->b.get()
				) = r;
			// Return now-recursive outer rule
			return expr::make<rule_node>(r);
		}
		
	public:
		/// Default constructor; builds a derivative parser graph from the given PEG grammar
		loader(ast::grammar& g, bool dbg = false) {
			// Read in rules
			for (ast::grammar_rule_ptr r : g.rs) {
				r->m->accept(this);                             // Convert to derivs::expr
				get_rule(r->name).shared->e = std::move(rVal);  // Bind to shared rule
			}
			
			// Normalize rules
			expr_set normed;
			for (auto rp : rs) {
				rp.second.normalize(normed);
				names.emplace(rp.second.get(), rp.first);
			}
			normed.clear();
			
			// Calculate fixed point of match() for all expressions
//			derivs::fixer fix;
//			for (auto rp : rs) {
//				fix(rp.second);
//			}
			
			if ( dbg ) {
				derivs::printer p(std::cout, names);
				for (auto rp : rs) {
					p.print(rp.second);
				}
				std::cout << "\n***** DONE LOADING RULES  *****\n" << std::endl;
				
			}
		}
		
		/// Gets the rules from a grammar
		std::map<std::string, shared_node>& get_rules() { return rs; }
		
		/// Gets the rule names for this grammar
		std::map<expr*, std::string>& get_names() { return names; }
		
		virtual void visit(ast::char_matcher& m) { rVal = expr::make<char_node>(m.c); }
		
		virtual void visit(ast::str_matcher& m) { rVal = expr::make<str_node>(m.s); }
		
		virtual void visit(ast::range_matcher& m) {
			// Empty alternation is a success
			if ( m.rs.size() == 0 ) { rVal = expr::make<eps_node>(); return; }
			
			// Transform last option
			auto it = m.rs.rbegin();
			rVal = make_char_range(*it);
			
			// Transform remaining options
			while ( ++it != m.rs.rend() ) {
				auto tVal = make_char_range(*it);
				rVal = expr::make<alt_node>(std::move(tVal), std::move(rVal));
			}
		}
		
		virtual void visit(ast::rule_matcher& m) { rVal = expr::make<rule_node>(get_rule(m.rule)); }
		
		virtual void visit(ast::any_matcher& m) { rVal = expr::make<any_node>(); }
		
		virtual void visit(ast::empty_matcher& m) { rVal = expr::make<eps_node>(); }
		
		virtual void visit(ast::action_matcher& m) {
			//TODO actually implement this; for the moment, just return success
			rVal = expr::make<eps_node>();
		}
		
		virtual void visit(ast::opt_matcher& m) {
			// match subexpression or epsilon
			m.m->accept(this);
			rVal = expr::make<alt_node>(std::move(rVal), 
			                            std::move(expr::make<eps_node>()));
		}
		
		virtual void visit(ast::many_matcher& m) {
			m.m->accept(this);
			rVal = make_many(std::move(rVal));
		}
		
		virtual void visit(ast::some_matcher& m) {
			m.m->accept(this);
			expr tVal = std::move(rVal.clone());
			rVal = expr::make<seq_node>(std::move(tVal), 
			                            std::move(make_many(std::move(rVal))));
		}
		
		virtual void visit(ast::seq_matcher& m) {
			// Convert vector in seq_matcher to cons-list in seq_expr
			
			// Empty sequence is a success
			if ( m.ms.size() == 0 ) { rVal = expr::make<eps_node>(); return; }
			
			// Transform last option
			auto it = m.ms.rbegin();
			(*it)->accept(this);
			
			// Transform remaining options
			while ( ++it != m.ms.rend() ) {
				expr tVal = std::move(rVal);
				(*it)->accept(this);
				rVal = expr::make<seq_node>(std::move(rVal), std::move(tVal));
			}
		}
		
		virtual void visit(ast::alt_matcher& m) {
			// Convert vector in alt_matcher to cons-list in alt_expr
			
			// Empty sequence is a success
			if ( m.ms.size() == 0 ) { rVal = expr::make<eps_node>(); return; }
			
			// Transform last option
			auto it = m.ms.rbegin();
			(*it)->accept(this);
			
			// Transform remaining options
			while ( ++it != m.ms.rend() ) {
				expr tVal = std::move(rVal);
				(*it)->accept(this);
				rVal = expr::make<alt_node>(std::move(rVal), std::move(tVal));
			}
		}
		
		virtual void visit(ast::look_matcher& m) {
			m.m->accept(this);
			rVal = expr::make<not_node>(
				std::move(expr::make<not_node>(std::move(rVal))));
		}
		
		virtual void visit(ast::not_matcher& m) {
			m.m->accept(this);
			rVal = expr::make<not_node>(std::move(rVal));
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
			rVal = expr::make<fail_node>();
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
		derivs::printer p(std::cout, l.get_names());
		
		// Fail on no such rule
		if ( rs.count(rule) == 0 ) return false;
		
		// Clone the rule so we don't break the shared definition
		expr e = rs.at(rule).clone(0);
		
		// Take derivatives until failure, match, or end of input
		ind i = 0;
		while ( true ) {
			if ( dbg ) { p.print(e); }
			
			switch ( e.type() ) {
			case fail_type: return false;
			case inf_type:  return false;
			case eps_type:  return true;
			case look_type: return true;
			default:        break; // do nothing
			}
			
			// Break on a match
			if ( ! e.match(i).empty() ) return true;
			
			char x;
			if ( ! in.get(x) ) { x = '\0'; }  // read character, \0 for EOF
			
			if ( dbg ) {
				std::cout << "d(\'" << (x == '\0' ? "\\0" : strings::escape(x)) << "\') =====>" 
				          << std::endl;
			}
			
			// Take derivative and increment input index
			e.d(x, i++);
			
			if ( x == '\0' ) break;
		}
		if ( dbg ) { p.print(e); }
		
		// Match if final expression matched on terminator char
		return ! e.match(i).empty();
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
