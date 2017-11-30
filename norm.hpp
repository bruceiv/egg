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

#include <istream>
#include <string>
#include <unordered_map>

#include "ast.hpp"
#include "derivs.hpp"
#include "fixer.hpp"
#include "visitors/instrumenter.hpp"

namespace derivs {
	class norm : public ast::visitor {
		/// Original grammar
		const ast::grammar* gram;
		/// List of rules in current normalization
		std::unordered_map<std::string, ptr<expr>>  rs;
		/// The AST pointer being processed
		const ast::matcher_ptr* pPtr;
		/// The derivative expression to return for the current visit
		ptr<expr> rVal;
		/// The current generation
		gen_type g;

		/// Normalizes a matcher
		ptr<expr> process(const ast::matcher_ptr& p) {
			pPtr = &p;
			p->accept(this);
			return rVal;
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

		/// Gets the unique rule expression corresponding to the given name
		ptr<expr> get_rule(const std::string& s) {
			auto it = rs.find( s );
			if ( it == rs.end() ) {
				// set in temporary infinite recursion
				it = rs.emplace_hint( it, s, inf_expr::make() );
				// find appropriate grammar rule
				auto rit = gram->names.find( s );
				if ( rit == gram->names.end() ) {
					// no matching rule, fail
					rVal = fail_expr::make();
				} else {
					// normalize expression
					process( rit->second->m );
				}
				// place in buffer
				it->second = rVal;
			}

			return it->second;
		}

		/// Resets the normalizer for a new generation
		void reset( gen_type g ) {
			rVal.reset();
			if ( norm::g == g ) return;
			norm::g = g;
			rs.clear();
		}

	public:
		norm() : gram(nullptr), rs(), pPtr(nullptr), rVal(), g(0) {}

		norm(const ast::grammar& gram)
			: gram(&gram), rs(), pPtr(nullptr), rVal(), g(0) {}

		void visit(ast::char_matcher& m) {
			rVal = expr::make_ptr<char_expr>( m.c );
		}

		void visit(ast::str_matcher& m) {
			rVal = expr::make_ptr<str_expr>( m.s );
		}

		void visit(ast::range_matcher& m) {
			// Empty alternation is a success or failure, depending on sense
			if ( m.rs.size() == 0 ) {
				rVal = m.neg ? fail_expr::make() : expr::make_ptr<eps_expr>(g);
				return;
			}
			
			// Transform first option
			auto it = m.rs.begin();
			rVal = make_char_range(*it, m.neg);
			if ( m.rs.size() == 1 ) return;
			
			// Transform remaining options
			expr_list rs{ rVal };
			rs.reserve( m.rs.size() );
			while ( ++it != m.rs.end() ) {
				rs.push_back( make_char_range(*it, m.neg) );
			}
			rVal = m.neg ? expr::make_ptr<and_expr>( std::move(rs) ) :
					expr::make_ptr<or_expr>( std::move(rs) );
		}

		void visit(ast::rule_matcher& m) {
			rVal = get_rule(m.rule);
		}

		void visit(ast::any_matcher& m) {
			rVal = expr::make_ptr<any_expr>();
		}
		
		void visit(ast::empty_matcher& m) {
			rVal = expr::make_ptr<eps_expr>(g);
		}
		
		void visit(ast::none_matcher& m) {
			rVal = expr::make_ptr<none_expr>();
		}

		void visit(ast::action_matcher& m) {
			//TODO unimplemented; treat as success
			rVal = expr::make_ptr<eps_expr>(g);
		}
		
		void visit(ast::opt_matcher& m) {
			// match subexpression or epsilon
			rVal = expr::make_ptr<opt_expr>( process(m.m), g );
		}

		void visit(ast::many_matcher& m) {
			// save current matcher pointer
			const ast::matcher_ptr& om = *pPtr;
			// split out one repetition
			rVal = expr::make_ptr<opt_expr>(
				expr::make_ptr<seq_expr>( process(m.m), om, g ), g );
		}

		void visit(ast::some_matcher& m) {
			// transmute into m.m m.m*
			rVal = expr::make_ptr<seq_expr>(
				process(m.m), ast::make_ptr<ast::many_matcher>(m.m), g );
		}

		void visit(ast::seq_matcher& m) {
			switch ( m.ms.size() ) {
			case 0:
				// empty sequence is a success
				rVal = expr::make_ptr<eps_expr>(g);
				return;
			case 1:
				// singleton expression drawn out
				process(m.ms.front());
				return;
			case 2:
				// draw singleton expression out and sequence
				process(m.ms.front());
				rVal = seq_expr::make(rVal, m.ms.back(), g);
				return;
			default:
				// draw initial expression out and sequence rest
				auto it = m.ms.begin();
				process(*it);
				ast::seq_matcher_ptr p = ast::make_ptr<ast::seq_matcher>();
				while ( ++it != m.ms.end() ) {
					*p += *it;
				}
				rVal = seq_expr::make(rVal, ast::as_ptr<ast::matcher>(p), g);
				return;
			}
		}
		
		void visit(ast::alt_matcher& m) {
			// Empty sequence is a success
			if ( m.ms.size() == 0 ) {
				rVal = expr::make_ptr<eps_expr>(g);
				return;
			}
			
			// Transform options to expression list
			expr_list es;
			for (auto& mi : m.ms) {
				process(mi);
				es.emplace_back(rVal);
			}
			rVal = alt_expr::make(es);
		}
		
		void visit(ast::until_matcher& m) {
			// save current matcher pointer
			const ast::matcher_ptr& om = *pPtr;
			// split out one repetition
			rVal = expr::make_ptr<alt_expr>(
				process(m.t),
				expr::make_ptr<seq_expr>( process(m.r), om, g ) );
		}

		void visit(ast::look_matcher& m) {
			rVal = expr::make_ptr<not_expr>(
				expr::make_ptr<not_expr>( process(m.m), g), g );
		}

		void visit(ast::not_matcher& m) {
			rVal = expr::make_ptr<not_expr>( process(m.m), g );
		}

		void visit(ast::capt_matcher& m) {
			// TODO implement; for now, ignore capture
			process(m.m);
		}
		
		void visit(ast::named_matcher& m) {
			// TODO implement; for now, ignore error message
			process(m.m);
		}
		
		void visit(ast::fail_matcher& m) {
			// TODO implement; for now, ignore error message
			rVal = fail_expr::make();
		}

		/// Normalize an AST node at a specified generation [default 0]
		ptr<expr> operator() (const ast::matcher_ptr& p, gen_type g = 0) {
			reset( g );
			return process(p);
		}

		/// Normalize the rule with the given name at a specified generation [default 0]
		ptr<expr> operator() (const std::string& r, gen_type g = 0) {
			reset( g );
			return get_rule(r);
		}

		/// Resets the normalizer for a new parse
		void reset( const ast::grammar& gram ) {
			norm::gram = &gram;
			rVal.reset();
			g = 0;
			rs.clear();
		}
	};
}
