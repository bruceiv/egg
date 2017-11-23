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

#include "derivs.hpp"
#include "norm.hpp"

#include <iostream>

#include "visitors/deriv_printer.hpp"

namespace derivs {

	/// Normalizing operator, defined by interpreter
	extern norm nrm;
	
	std::ostream& operator<< (std::ostream& out, expr_type t) {
		switch (t) {
		case fail_type:  out << "FAIL";  break;
		case inf_type:   out << "INF";   break;
		case eps_type:   out << "EPS";   break;
		case char_type:  out << "CHAR";  break;
		case except_type: out << "^CHAR"; break;
		case range_type: out << "RANGE"; break;
		case except_range_type: out << "^RANGE"; break;
		case any_type:   out << "ANY";   break;
		case none_type:  out << "NONE";  break;
		case str_type:   out << "STR";   break;
		// case rule_type:  out << "RULE";  break;
		case not_type:   out << "NOT";   break;
		case alt_type:   out << "ALT";   break;
		case or_type:  out << "OR";  break;
		case and_type: out << "AND"; break;
		case seq_type:   out << "SEQ";   break;
		}
		return out;
	}
	
	// memo_expr ///////////////////////////////////////////////////////////////////
	
	void memo_expr::reset_memo() {
		// memo_d.reset();
		// last_index = no_gen;
		memo_match = gen_set{};
		memo_back = gen_set{};
		flags = {false,false};
	}
	
	ptr<expr> memo_expr::d(char x, gen_type i) const {
		// return stored derivative
		if ( i == last_index ) {
			ptr<expr> dx = memo_d.lock();
			if ( dx ) return dx;
		}

		// no (matching) stored derivative; compute and store
		ptr<expr> dx = deriv(x, i);
		memo_d = dx;
		last_index = i;
		return dx;
	}
	
	gen_set memo_expr::match() const {
		if ( ! flags.match ) {
			memo_match = match_set();
			flags.match = true;
		}
		
		return memo_match;
	}
	
	gen_set memo_expr::back() const {
		if ( ! flags.back ) {
			memo_back = back_set();
			flags.back = true;
		}
		
		return memo_back;
	}
	
	// fail_expr ///////////////////////////////////////////////////////////////////
	
	//ptr<expr> fail_expr::make() { return expr::make_ptr<fail_expr>(); }
	ptr<expr> fail_expr::make() {
		static ptr<expr> singleton{new fail_expr};
		return singleton;
	}
	
	// A failure expression can't un-fail - no strings to match with any prefix
	ptr<expr> fail_expr::d(char, gen_type) const {
		return fail_expr::make();
	}
		
	gen_set fail_expr::match() const { return gen_set{}; }
	
	gen_set fail_expr::back()  const { return gen_set{}; }
	
	// inf_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> inf_expr::make() {
		static ptr<expr> singleton{new inf_expr};
		return singleton;	
	}
	
	// An infinite loop expression never breaks, ill defined with any prefix
	ptr<expr> inf_expr::d(char, gen_type) const {
		return inf_expr::make();
	}
	
	gen_set inf_expr::match() const { return gen_set{}; }
	
	gen_set inf_expr::back()  const { return gen_set{}; }
	
	// eps_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> eps_expr::make(gen_type g) {
		return expr::make_ptr<eps_expr>(g);
	}
		
	// Only succeeds if string is empty
	ptr<expr> eps_expr::d(char x, gen_type i) const {
		return ( x == '\0' ) ? eps_expr::make(g) : fail_expr::make();
	}
	
	gen_set eps_expr::match() const { return gen_set{g}; }
	
	gen_set eps_expr::back()  const { return gen_set{g}; }
	
	// char_expr ///////////////////////////////////////////////////////////////////
	
	ptr<expr> char_expr::make(char c) { return expr::make_ptr<char_expr>(c); }
	
	// Single-character expression either consumes matching character or fails
	ptr<expr> char_expr::d(char x, gen_type i) const {
		return ( c == x ) ? eps_expr::make(i) : fail_expr::make();
	}
	
	gen_set char_expr::match() const { return gen_set{}; }
	
	gen_set char_expr::back()  const { return gen_set{}; }

	// except_expr /////////////////////////////////////////////////////////////////
	
	ptr<expr> except_expr::make(char c) { return expr::make_ptr<except_expr>(c); }
	
	// Single-character expression either consumes non-matching character or fails
	ptr<expr> except_expr::d(char x, gen_type i) const {
		return ( c != x ) ? eps_expr::make(i) : fail_expr::make();
	}
	
	gen_set except_expr::match() const { return gen_set{}; }
	
	gen_set except_expr::back()  const { return gen_set{}; }
	
	// range_expr //////////////////////////////////////////////////////////////////
	
	ptr<expr> range_expr::make(char b, char e) { return expr::make_ptr<range_expr>(b, e); }
	
	// Character range expression either consumes matching character or fails
	ptr<expr> range_expr::d(char x, gen_type i) const {
		return ( b <= x && x <= e ) ? eps_expr::make(i) : fail_expr::make();
	}
	
	gen_set range_expr::match() const { return gen_set{}; }
	
	gen_set range_expr::back()  const { return gen_set{}; }

	// except_range_expr ///////////////////////////////////////////////////////////
	
	ptr<expr> except_range_expr::make(char b, char e) { 
		return expr::make_ptr<except_range_expr>(b, e);
	}
	
	// Character range expression either consumes non-matching character or fails
	ptr<expr> except_range_expr::d(char x, gen_type i) const {
		return ( b > x || x > e ) ? eps_expr::make(i) : fail_expr::make();
	}
	
	gen_set except_range_expr::match() const { return gen_set{}; }
	
	gen_set except_range_expr::back()  const { return gen_set{}; }
	
	// any_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> any_expr::make() { return expr::make_ptr<any_expr>(); }
	
	// Any-character expression consumes any character
	ptr<expr> any_expr::d(char x, gen_type i) const {
		return ( x == '\0' ) ? fail_expr::make() : eps_expr::make(i);
	}
	
	gen_set any_expr::match() const { return gen_set{}; }
	
	gen_set any_expr::back()  const { return gen_set{}; }

	// none_expr ///////////////////////////////////////////////////////////////////
	
	ptr<expr> none_expr::make() { return expr::make_ptr<none_expr>(); }
	
	// None-expression only matches at end-of-input
	ptr<expr> none_expr::d(char x, gen_type i) const {
		return ( x == '\0' ) ? eps_expr::make(i) : fail_expr::make();
	}
	
	gen_set none_expr::match() const { return gen_set{}; }
	
	gen_set none_expr::back()  const { return gen_set{}; }
	
	// str_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> str_expr::make(const std::string& s) {
		switch ( s.size() ) {
		case 0:  return eps_expr::make(no_gen); // FIXME
		case 1:  return char_expr::make(s[0]);
		default: return expr::make_ptr<str_expr>(s);
		}
	}
	
	ptr<expr> str_expr::d(char x, gen_type i) const {
		// Check that the first character matches
		if ( (*sp)[si] != x ) return fail_expr::make();
		
		// Otherwise return string or epsilon expression, as appropriate
		if ( sp->size() == si+1 ) return eps_expr::make(i);
		
		return ptr<expr>{new str_expr(sp, si+1)};
	}
	
	gen_set str_expr::match() const { return gen_set{}; }
	
	gen_set str_expr::back()  const { return gen_set{}; }
	
	// rule_expr ///////////////////////////////////////////////////////////////////
	
	// ptr<expr> rule_expr::make(ptr<expr> r) { return expr::make_ptr<rule_expr>(r); }
	
	// ptr<expr> rule_expr::deriv(char x, gen_type i) const {
	// 	// signal infinite loop if we try to take this derivative again
	// 	memo_d = inf_expr::make();
	// 	last_index = i;
	// 	// calculate derivative
	// 	return r->d(x, i);
	// }
	
	// gen_set rule_expr::match_set() const {
	// 	// Stop this from infinitely recursing
	// 	flags.match = true;
	// 	memo_match = gen_set{};
		
	// 	// Calculate match set
	// 	return r->match();
	// }
	
	// gen_set rule_expr::back_set() const {
	// 	// Stop this from infinitely recursing
	// 	flags.back = true;
	// 	memo_back = gen_set{};
		
	// 	// Calculate backtrack set
	// 	return r->back();
	// }
	
	// not_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> not_expr::make(ptr<expr> e, gen_type g) {
		switch ( e->type() ) {
		case fail_type: return eps_expr::make(g);  // return match on subexpression failure
		case inf_type:  return e;                  // propegate infinite loop
		case any_type:  return none_expr::make();  // !. becomes $ for end-of-input
		default:        break;
		}
		
		// return failure on subexpression success
		if ( ! e->match().empty() ) return fail_expr::make();
		
		return expr::make_ptr<not_expr>(e, g);
	}
	
	// Take negative lookahead of subexpression derivative
	ptr<expr> not_expr::deriv(char x, gen_type i) const {
		return not_expr::make(e->d(x, i), g);
	}
	
	gen_set not_expr::match_set() const { return gen_set{}; }
	
	gen_set not_expr::back_set() const { return gen_set{g}; }
	
	// alt_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> alt_expr::make(ptr<expr> a, ptr<expr> b) {
		switch ( a->type() ) {
		// if first alternative fails, use second
		case fail_type: return b;
		// if first alternative is infinite loop, propegate
		case inf_type:  return a; // an inf_expr
		default:        break; // do nothing
		}
		
		// if first alternative matches or second alternative fails, use first
		if ( b->type() == fail_type || ! a->match().empty() ) return a;
		
		return expr::make_ptr<alt_expr>(a, b);
	}
	
	ptr<expr> alt_expr::make(const expr_list& es) {
		// Empty alternation list is an epsilon-rule; all failing list is a fail-rule
		if ( es.empty() ) return eps_expr::make(0);

		expr_list nes;
		for (const ptr<expr>& e : es) {
			expr_type ety = e->type();
			// skip failing nodes
			if ( ety == fail_type ) continue;

			nes.push_back(e);
			
			// infinite loops and matching nodes end the alternation
			if ( ety == inf_type || ! e->match().empty() ) break;
		}

		// Eliminate alternation node if not enough nodes left
		switch ( nes.size() ) {
		case 0: return fail_expr::make();
		case 1: return nes[0];
		default: return expr::make_ptr<alt_expr>(nes);
		}
	}
	
	ptr<expr> alt_expr::deriv(char x, gen_type i) const {
		expr_list des;
		for (const ptr<expr>& e : es) {
			// Take derivative
			ptr<expr> de = e->d(x, i);
			
			expr_type dety = de->type();
			// skip failing nodes
			if ( dety == fail_type ) continue;
			
			des.push_back(de);

			// infinite loops and matching nodes end the alternation
			if ( dety == inf_type || ! de->match().empty() ) break;
		}
		
		// Eliminate alternation node if not enough nodes left
		switch ( des.size() ) {
		case 0: return fail_expr::make();
		case 1: return des[0];
		default: return expr::make_ptr<alt_expr>(des);
		}
	}
	
	gen_set alt_expr::match_set() const {
		gen_set x;
		for (const ptr<expr>& e : es) { set_union( x, e->match() ); }
		return x;
	}
	
	gen_set alt_expr::back_set() const {
		gen_set x;
		for (const ptr<expr>& e : es) { set_union( x, e->back() ); }
		return x;
	}

	// or_expr /////////////////////////////////////////////////////////////////////
	
	ptr<expr> or_expr::make(expr_list&& es) {
		switch ( es.size() ) {
		case 0:  return eps_expr::make(no_gen);  // FIXME
		case 1:  return es.front();
		default: return expr::make_ptr<or_expr>(std::move(es));
		}
	}
	
	ptr<expr> or_expr::deriv(char x, gen_type i) const {
		for ( const ptr<expr>& e : es ) {
			ptr<expr> de = e->d(x, i);
			if ( de->type() == eps_type ) return de;
		}
		return fail_expr::make();
	}
	
	gen_set or_expr::match_set() const { return gen_set{}; }
	
	gen_set or_expr::back_set() const { return gen_set{}; }

	// and_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> and_expr::make(expr_list&& es) {
		switch ( es.size() ) {
		case 0:  return fail_expr::make();
		case 1:  return es.front();
		default: return expr::make_ptr<and_expr>(std::move(es));
		}
	}
	
	ptr<expr> and_expr::deriv(char x, gen_type i) const {
		for ( const ptr<expr>& e : es ) {
			ptr<expr> de = e->d(x, i);
			if ( de->type() == fail_type ) return de;
		}
		return eps_expr::make(i);
	}
	
	gen_set and_expr::match_set() const { return gen_set{}; }
	
	gen_set and_expr::back_set() const { return gen_set{}; }
	
	// seq_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> seq_expr::make(ptr<expr> a, ast::matcher_ptr b, gen_type i) {
		switch ( b->type() ) {
		// empty second element just leaves first
		case ast::empty_type:  return a;
		// failing second element propagates
		case ast::fail_type: return fail_expr::make();
		default: break;  // do nothing
		}
		
		switch ( a->type() ) {
		// empty first element just leaves follower
		case eps_type:
			return nrm(b, i);
		// failure or infinite loop propagates
		case fail_type:
		case inf_type:
			return a;
		default: break;  // do nothing
		}
		
		// Set up match-fail follower
		gen_set am = a->match();
		gen_type gl = ( ! am.empty() && last(am) == i ) ? i : no_gen;
		
		// set up lookahead follower
		look_list bs;
		if ( ! a->back().empty() ) {
			gen_type bgl = b->nbl() ? i : no_gen;
			bs.emplace_back(i, nrm(b, i), bgl);
		}
		
		// return constructed expression
		return expr::make_ptr<seq_expr>(a, b, std::move(bs), gl);
	}

	/// Take derivative for lookahead gen g from list bs, null for none-such
	ptr<expr> d_from( const seq_expr::look_list& bs, gen_type g, char x, gen_type i ) {
		for (const seq_expr::look_node& bi : bs) {
			if ( bi.g < g ) continue;
			if ( bi.g > g ) return {};  // generation list is sorted
			
			ptr<expr> dbi = bi.e->d(x, i);  // found element, take derivative
			
			if ( dbi->type() == fail_type ) {  // straight path fails ...
				if ( bi.gl != no_gen ) {          // ... but matched in the past
					return eps_expr::make(bi.gl);   // return appropriate lookahead
				} else {                          // ... and no previous match
					return dbi;                      // return a failure expression
				}
			}

			// if there is no failure backtrack, or it is replaced by a new 
			// backtrack, just return the expression
			if ( bi.gl == no_gen ) return dbi;
			gen_set dbim = dbi->match();
			if ( ! dbim.empty() && last(dbim) == i ) return dbi;

			// otherwise return alt-expr of this lookahead and its failure 
			// backtrack

			// Otherwise return alt-expr of this lookahead and its failure backtrack
			return alt_expr::make(dbi, eps_expr::make(bi.gl));
		}

		return {};
	}
	
	ptr<expr> seq_expr::deriv(char x, gen_type i) const {
		bool did_inc = false;
		ptr<expr> da = a->d(x, i);
		
		// Handle empty or failure results from predecessor derivative
		switch ( da->type() ) {
		case eps_type: {
			// generation of match
			auto dag = std::static_pointer_cast<eps_expr>(da)->g;

			// current-gen match
			if ( dag == i ) {
				// Take follower (or follower's end-of-string derivative on end-of-string)
				ptr<expr> bn = nrm(b, i);
				if ( x == '\0' ) { bn = bn->d('\0', i); }
				return bn;
			}
		
			// old lookahead expression
			ptr<expr> dbi = d_from(bs, dag, x, i);
			if ( dbi ) return dbi;
			
			// end-of-string is only case where we can get a success for an unseen gen
			if ( x == '\0' ) {
				return nrm(b, i)->d('\0', i);
			}
			return fail_expr::make(); // if lookahead follower not found, fail
		} case fail_type: {
			// Return match-fail follower
			if ( gl == no_gen ) return fail_expr::make();
			ptr<expr> dc = d_from(bs, gl, x, i);
			return dc ? dc : fail_expr::make();
		} case inf_type: {
			// Propegate infinite loop
			return da;
		} default: {}  // do nothing
		}
		
		// check for new failure backtrack
		bool new_gen = false;
		gen_type dgl = gl;
		gen_set dam = da->match();
		if ( ! dam.empty() && last(dam) == i ) {
			dgl = i;
			new_gen = true;
		}
		
		gen_set dab = da->back();
		auto dabt = dab.begin();
		auto bit = bs.begin();

		look_list dbs;
		while ( dabt != dab.end() && bit != bs.end() ) {
			const look_node& bi = *bit;
			if ( bi.g < *dabt ) { ++bit; continue; }  // skip generations not in backtrack list
			assert(bi.g == *dabt && "no generations missing from backtrack list");
			
			ptr<expr> dbi = bi.e->d(x, i);
			gen_type dbgl = bi.gl;
			gen_set dbim = dbi->match();
			if ( ! dbim.empty() && last(dbim) == i ) {  // set new match-fail backtrack if needed
				dbgl = i;
			}
			dbs.emplace_back(bi.g, dbi, dbgl);
			
			++dabt; ++bit;
		}
		
		// Add new lookahead backtrack if needed
		if ( dabt != dab.end() ) {
			gen_type dabm = *dabt;
			assert(dabm == i && "leftover generation is new generation");
			assert(++dabt == dab.end() && "only one new lookahead backtrack");
			
			ptr<expr> nb = nrm(b, i);
			gen_type bgl = b->nbl() ? i : no_gen;
			dbs.emplace_back(dabm, nb, bgl);
		}
		
		return expr::make_ptr<seq_expr>(da, b, std::move(dbs), dgl);
	}
	
	gen_set seq_expr::match_set() const {
		gen_set x;

		// Include matches from matching lookahead successors
		gen_set am = a->match();

		auto at = am.begin();
		auto bit = bs.begin();
		while ( at != am.end() && bit != bs.end() ) {
			auto& bi = *bit;
			auto  ai = *at;
			
			// Find matching generations
			if ( bi.g < ai ) { ++bit; continue; }
			else if ( bi.g > ai ) { ++at; continue; }
			
			// Add followers to match set as well as follower match-fail
			set_union( x, bi.e->match() );
			if ( bi.gl != no_gen ) set_add( x, bi.gl );
			
			++at; ++bit;
		}
		
		return x;
	}
	
	gen_set seq_expr::back_set() const {
		gen_set x;
		
		// Include lookahead follower backtracks
		for (const look_node& bi : bs) {
			set_union( x, bi.e->back() );
			if ( bi.gl != no_gen ) set_add( x, bi.gl );
		}
		
		return x;
	}
		
} // namespace derivs

