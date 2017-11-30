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
		case not_type:   out << "NOT";   break;
		case alt_type:   out << "ALT";   break;
		case opt_type:   out << "OPT";   break;
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
		return eps_expr::make(g);
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
		
		auto bt = b->type();
		// if second alternative fails or first alternative matches, use first
		if ( bt == fail_type || ! a->match().empty() ) return a;
		
		// if second alternative is empty, mark without node
		if ( bt == eps_type ) {
			gen_type g = std::static_pointer_cast<eps_expr>(b)->g;
			return expr::make_ptr<opt_expr>(a, g);
		}
		
		return expr::make_ptr<alt_expr>(a, b);
	}
	
	ptr<expr> alt_expr::make(const expr_list& es) {
		// Empty alternation list is an epsilon-rule; all failing list is a fail-rule
		if ( es.empty() ) return eps_expr::make(0);

		expr_list nes;
		gen_type ngl = no_gen;
		for (const ptr<expr>& e : es) {
			expr_type ety = e->type();
			// skip failing nodes
			if ( ety == fail_type ) continue;

			// mark and terminate on epsilon nodes
			if ( ety == eps_type ) {
				ngl = std::static_pointer_cast<eps_expr>(e)->g;
				break;
			}

			nes.push_back(e);
			
			// infinite loops and matching nodes end the alternation
			if ( ety == inf_type || ! e->match().empty() ) break;
		}

		// Eliminate alternation node if not enough nodes left
		switch ( nes.size() ) {
		case 0: return ngl == no_gen ? fail_expr::make() : eps_expr::make(ngl);
		case 1: return ngl == no_gen ? nes[0] : expr::make_ptr<opt_expr>(nes[0], ngl);
		default: return expr::make_ptr<alt_expr>(nes, ngl);
		}
	}
	
	ptr<expr> alt_expr::deriv(char x, gen_type i) const {
		expr_list des;
		gen_type dgl = gl;
		for (const ptr<expr>& e : es) {
			// Take derivative
			ptr<expr> de = e->d(x, i);
			
			expr_type dety = de->type();
			// skip failing nodes
			if ( dety == fail_type ) continue;
			
			// mark and terminate on epsilon nodes
			if ( dety == eps_type ) {
				dgl = std::static_pointer_cast<eps_expr>(de)->g;
				break;
			}
			
			des.push_back(de);

			// infinite loops and matching nodes end the alternation
			if ( dety == inf_type || ! de->match().empty() ) {
				dgl = no_gen;
				break;
			}
		}
		
		// Eliminate alternation node if not enough nodes left
		switch ( des.size() ) {
		case 0: return dgl == no_gen ? fail_expr::make() : eps_expr::make(dgl);
		case 1: return dgl == no_gen ? des[0] : expr::make_ptr<opt_expr>(des[0], dgl);
		default: return expr::make_ptr<alt_expr>(des, dgl);
		}
	}
	
	gen_set alt_expr::match_set() const {
		// invariant: gl is set, or only the last alternative may match
		if ( gl != no_gen ) { return gen_set{ gl }; }
		return es.back()->match();
	}
	
	gen_set alt_expr::back_set() const {
		gen_set x;
		if ( gl != no_gen ) { set_add( x, gl ); }
		for (const ptr<expr>& e : es) { set_union( x, e->back() ); }
		return x;
	}

	// opt_expr ////////////////////////////////////////////////////////////////////

	ptr<expr> opt_expr::make(ptr<expr> e, gen_type gl) {
		switch ( e->type() ) {
		case fail_type: return eps_expr::make(gl);  // return epsilon match on subexpression failure
		case eps_type: case inf_type:  return e;    // propegate epsilon, infinite loop
		default: break;
		}
		
		// return subexpression on match
		if ( ! e->match().empty() ) return e;
		
		return expr::make_ptr<opt_expr>(e, gl);
	}
	
	// Take subexpression derivative
	ptr<expr> opt_expr::deriv(char x, gen_type i) const {
		return opt_expr::make( e->d(x, i), gl );
	}
	
	gen_set opt_expr::match_set() const { return gen_set{gl}; }
	
	gen_set opt_expr::back_set() const {
		gen_set eb = e->back();
		set_add( eb, gl );
		return eb;
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
		case eps_type: return nrm(b, i);
		// failure or infinite loop propagates
		case fail_type: case inf_type: return a;
		default: break;  // do nothing
		}
		
		// return constructed expression
		return expr::make_ptr<seq_expr>(a, b, i);
	}

	/// Take derivative for lookahead gen g from list bs, fail for none-such
	ptr<expr> d_from( const seq_expr::look_list& bs, gen_type g, char x, gen_type i ) {
		for (const seq_expr::look_node& bi : bs) {
			if ( bi.g < g ) continue;
			if ( bi.g > g ) return fail_expr::make();  // generation list is sorted
			
			return bi.e->d(x, i);  // found element, take derivative
		}

		return fail_expr::make();  // no element, fail
	}
	
	ptr<expr> seq_expr::deriv(char x, gen_type i) const {
		ptr<expr> da = a->d(x, i);
		
		// Handle empty or failure results from predecessor derivative
		switch ( da->type() ) {
		case eps_type: {
			// generation of match
			auto dag = std::static_pointer_cast<eps_expr>(da)->g;

			// current- or last-gen match
			if ( dag == i || dag == i-1 ) {
				// Take follower (or follower's derivative on end-of-string or last-gen)
				ptr<expr> bn = nrm(b, dag);
				if ( x == '\0' || dag == i-1 ) { bn = bn->d(x, i); }
				return bn;
			}
		
			// old lookahead expression
			return d_from(bs, dag, x, i);
		} 
		// propegate fail, infinite loop
		case fail_type: case inf_type: return da;
		default: break;  // do nothing
		}
		
		gen_set dab = da->back();
		auto dabt = dab.begin();
		auto bit = bs.begin();

		look_list dbs;
		while ( dabt != dab.end() && bit != bs.end() ) {
			const look_node& bi = *bit;
			// skip generations not in backtrack list or without successful followers
			if ( bi.g < *dabt ) { ++bit; continue; }
			if ( bi.g > *dabt ) { ++dabt; continue; }
			
			ptr<expr> dbi = bi.e->d(x, i);

			++dabt; ++bit;

			// skip failures
			if ( dbi->type() == fail_type ) continue;

			// set new backtrack
			dbs.emplace_back(bi.g, dbi);
		}
		
		// Add new lookahead backtrack if needed
		while ( dabt != dab.end() ) {
			// skip generations without sucessful followers
			if ( *dabt < i-1 ) { ++dabt; continue; }

			// lazily generate current generation follower
			if ( *dabt == i ) break;

			// take derivative of previous-generation follower
			ptr<expr> nb = nrm(b, i-1)->d(x, i);

			// skip failures
			if ( nb->type() == fail_type ) break;

			// set new backtrack
			dbs.emplace_back(i-1, nb);
			break;
		}
		
		return expr::make_ptr<seq_expr>(da, b, std::move(dbs), i);
	}
	
	gen_set seq_expr::match_set() const {
		gen_set x;

		// Include matches from matching lookahead successors
		gen_set am = a->match();
		if ( am.empty() ) return x;

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
			
			++at; ++bit;
		}

		// account for lazy generation of current-gen lookahead
		if ( last( am ) == g && b->nbl() ) { set_add( x, g ); }
		
		return x;
	}
	
	gen_set seq_expr::back_set() const {
		gen_set x;
		
		// Include lookahead follower backtracks
		for (const look_node& bi : bs) {
			set_union( x, bi.e->back() );
		}

		// account for lazy generation of current-gen lookahead
		gen_set ab = a->back();
		if ( ! ab.empty() && last( ab ) == g && b->look() ) { set_add( x, g ); }
		
		return x;
	}
		
} // namespace derivs

