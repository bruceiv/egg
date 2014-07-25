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

//uncomment to disable asserts
//#define NDEBUG
#include <cassert>
#include <utility>

#include "derivs-mut.hpp"

namespace derivs {
	
	// UTILITY /////////////////////////////////////////////////////////////////////
	
	gen_map new_back_map(const expr& e, gen_type gm, bool& did_inc, ind i) {
		if ( e.back(i).max() > 0 ) {
			assert(e.back(i).max() == 1 && "static lookahead gen <= 1");
			did_inc = true;
			return gen_map{0, gm+1};
		} else {
			return gen_map{0};
		}
	}
	
	gen_map new_back_map(const expr& e, gen_type& gm, ind i) {
		if ( e.back(i).max() > 0 ) {
			assert(e.back(i).max() == 1 && "static lookahead gen <= 1");
			return gen_map{0, ++gm};
		} else {
			return gen_map{0};
		}
	}
	
	gen_map default_back_map(const expr& e, bool& did_inc, ind i) {
		return new_back_map(e, 0, did_inc, i);
	}
	
	void update_back_map(gen_map& eg, gen_type ebm, const expr& de, gen_type& gm, ind i) {
		gen_type debm = de.back(i).max();
		if ( debm > ebm ) { eg.add_back(debm, ++gm); }
	}
	
	void update_back_map(gen_map& eg, gen_type ebm, const expr& de, 
	                     gen_type gm, bool& did_inc, ind i) {
		gen_type debm = de.back(i).max();
		if ( debm > ebm ) {
			did_inc = true;
			eg.add_back(debm, gm+1);
		}
	}
	
	// fail_node ///////////////////////////////////////////////////////////////////
	
	expr fail_node::make() { return expr::make<fail_node>(); }
	
	expr fail_node::clone(ind) const { return expr::make<fail_node>(); }
	
	void fail_node::normalize(expr&, expr_set&) {}
	
	void fail_node::d(expr&, char, ind) { /* invariant */ }
		
	gen_set fail_node::match(ind) const { return gen_set{}; }
	
	gen_set fail_node::back(ind)  const { return gen_set{0}; }
	
	// inf_node ////////////////////////////////////////////////////////////////////
	
	expr inf_node::make() { return expr::make<inf_node>(); }
	
	expr inf_node::clone(ind) const { return expr::make<inf_node>(); }
	
	void inf_node::normalize(expr&, expr_set&) {}
	
	void inf_node::d(expr&, char, ind) { /* invariant */ }
	
	gen_set inf_node::match(ind) const { return gen_set{}; }
	
	gen_set inf_node::back(ind)  const { return gen_set{0}; }
	
	// eps_node ////////////////////////////////////////////////////////////////////
	
	expr eps_node::make() { return expr::make<eps_node>(); }
	
	expr eps_node::clone(ind) const { return expr::make<eps_node>(); }
	
	void eps_node::normalize(expr&, expr_set&) {}
	
	void eps_node::d(expr& self, char x, ind) {
		if ( x != '\0' ) { self.remake<fail_node>(); } // Only match on empty string
	}
	
	gen_set eps_node::match(ind) const { return gen_set{0}; }
	
	gen_set eps_node::back(ind)  const { return gen_set{0}; }
	
	// look_node ///////////////////////////////////////////////////////////////////
	
	expr look_node::make(gen_type g) {
		return (g == 0) ? expr::make<eps_node>() : expr::make<look_node>(g);
	}
	
	expr look_node::clone(ind) const { return expr::make<look_node>(b); }
	
	void look_node::normalize(expr& self, expr_set&) {
		if ( b == 0 ) {
			self.remake<eps_node>();
			return;
		}
	}
	
	void look_node::d(expr&, char, ind) { /* invariant (unparsed suffixes okay) */ }

	gen_set look_node::match(ind) const { return gen_set{b}; }
	
	gen_set look_node::back(ind)  const { return gen_set{b}; }
	
	// char_node ///////////////////////////////////////////////////////////////////
	
	expr char_node::make(char c) { return expr::make<char_node>(c); }
	
	expr char_node::clone(ind) const { return expr::make<char_node>(c); }
	
	void char_node::normalize(expr&, expr_set&) {}
	
	void char_node::d(expr& self, char x, ind) {
		if ( c == x ) { self.remake<eps_node>(); }  // Match on character
		else { self.remake<fail_node>(); }          // Fail otherwise
	}
	
	gen_set char_node::match(ind) const { return gen_set{}; }
	
	gen_set char_node::back(ind)  const { return gen_set{0}; }
	
	// range_node //////////////////////////////////////////////////////////////////
	
	expr range_node::make(char b, char e) { return expr::make<range_node>(b, e); }
	
	expr range_node::clone(ind) const { return expr::make<range_node>(b, e); }
	
	void range_node::normalize(expr&, expr_set&) {}
	
	void range_node::d(expr& self, char x, ind) {
		if ( b <= x && x <= e ) { self.remake<eps_node>(); }  // Match if in range
		else { self.remake<fail_node>(); }                    // Fail otherwise
	}
	
	gen_set range_node::match(ind) const { return gen_set{}; }
	
	gen_set range_node::back(ind)  const { return gen_set{0}; }
	
	// any_node ////////////////////////////////////////////////////////////////////
	
	expr any_node::make() { return expr::make<any_node>(); }
	
	expr any_node::clone(ind) const { return expr::make<any_node>(); }
	
	void any_node::normalize(expr&, expr_set&) {}
	
	void any_node::d(expr& self, char x, ind) {
		if ( x == '\0' ) { self.remake<fail_node>(); }  // Fail on no character
		else { self.remake<eps_node>(); }               // Match otherwise
	}
	
	gen_set any_node::match(ind) const { return gen_set{}; }
	
	gen_set any_node::back(ind)  const { return gen_set{0}; }
	
	// str_node ////////////////////////////////////////////////////////////////////
	
	expr str_node::make(const std::string& s) {
		switch ( s.size() ) {
		case 0:  return eps_node::make();
		case 1:  return char_node::make(s[0]);
		default: return expr::make<str_node>(s);
		}
	}
	
	expr str_node::clone(ind) const { return expr::make<str_node>(*this); }
	
	void str_node::normalize(expr& self, expr_set&) {
		switch ( s.size() ) {
		case 0:  self.remake<eps_node>();      return;
		case 1:  self.remake<char_node>(s[0]); return;
		default:                               return;
		}
	}
	
	void str_node::d(expr& self, char x, ind) {
		// REMEMBER CHARS IN s ARE IN REVERSE ORDER
		
		// Check that the first character matches
		if ( s.back() != x ) {
			self.remake<fail_node>();
			return;
		}
				
		// Switch to character if this derivative consumes the penultimate
		if ( s.size() == 2 ) {
			self.remake<char_node>(s[0]);
			return;
		}
		
		// Mutate string node otherwise
		s.pop_back();
	}
	
	gen_set str_node::match(ind) const { return gen_set{}; }
	
	gen_set str_node::back(ind)  const { return gen_set{0}; }
	
	// shared_node ///////////////////////////////////////////////////////////////////
	
	expr shared_node::make(expr&& e, ind crnt) {
		return expr::make<shared_node>(std::move(e), crnt);
	}
	
	expr shared_node::clone(ind i) const {
		switch ( i - shared->crnt ) {
		case 0: { // crnt == i
			return expr::make<shared_node>(std::move(shared->e.clone(i)), i, 
			                               shared->crnt_cache, shared->prev_cache);
		} case ind(-1): { // crnt - 1 == i
			return expr::make<shared_node>(std::move(shared->e.clone(i)), i, 
			                               shared->prev_cache);
		} case 1: { // crnt + 1 == i
			// Cache existing back and match
			node_cache& prev_cache = shared->crnt_cache;
			if ( ! prev_cache.flags.back ) { prev_cache.set_back(shared->e.back(i-1)); }
			if ( ! prev_cache.flags.match ) { prev_cache.set_match(shared->e.match(i-1)); }
			
			return expr::make<shared_node>(std::move(shared->e.clone(i)), i, 
			                               node_cache{}, prev_cache);
		} default: {
			return expr::make<shared_node>(std::move(shared->e.clone(i)), i);
		}
		}
	}
	
	void shared_node::normalize(expr_set& normed) {
		// Ensure can't enter infinite normalization loop
		if ( shared->dirty || normed.count(get()) ) return;
		
		shared->dirty = true;         // break infinite loop
		shared->e.normalize(normed);  // normalize subexpression
		shared->dirty = false;        // lower dirty flag
		normed.emplace(get());        // mark as normalized
	}
	
	void shared_node::d(expr&, char x, ind i) {
		if ( i == shared->crnt ) {  // Computing current derivative
			// Step cache one forward [will not call back(i-1) or match(i-1) again]
			shared->prev_cache.set_back( back(i) );
			shared->prev_cache.set_match( match(i) );
			
			// Increment current and invalidate old current cache
			++(shared->crnt);
			shared->crnt_cache.invalidate();
					
			// Update with derivative; if calls self.d() recursively will not break assertion below
			shared->e.d(x, i);
		} else assert(i == shared->crnt - 1 && "shared node only keeps two generations");
		
		// if we reach here than we know that the previously-computed derivative 
		// was requested, and it's already stored
	}
		
	gen_set shared_node::match(ind i) const {
		if ( i == shared->crnt ) {
			// Current generation, cached calculation
			if ( ! shared->crnt_cache.flags.match ) {
				shared->crnt_cache.set_match( shared->e.match(i) );
			}
			return shared->crnt_cache.match;
		} else if ( i == shared->crnt - 1 ) {
			// Previous generation, read from cache
			assert(shared->prev_cache.flags.match && "match cached for previous generation");
			return shared->prev_cache.match;
		} else {
			assert(false && "shared node only keeps two generations");
			return gen_set{};
		}
	}
	
	gen_set shared_node::back(ind i)  const {
		if ( i == shared->crnt ) {
			// Current generation, cached calculation
			if ( ! shared->crnt_cache.flags.back ) {
				shared->crnt_cache.set_back( shared->e.back(i) );
			}
			return shared->crnt_cache.back;
		} else if ( i == shared->crnt - 1 ) {
			// Previous generation, read from cache
			assert(shared->prev_cache.flags.back && "back cached for previous generation");
			return shared->prev_cache.back;
		} else if ( i == shared->crnt + 1 ) {
			assert(false && "requested not-yet-computed backtrack generation");
			return gen_set{};
		} else {
			assert(false && "shared node only keeps two generations");
			return gen_set{};
		}
	}
	
	// rule_node ////////////////////////////////////////////////////////////////////
	
	// Unlike the usual semantics, we want to reuse the shared rule node and cached functions
	expr rule_node::clone(ind) const { return expr::make<rule_node>(*this); }
	
	void rule_node::normalize(expr& self, expr_set& normed) { r.normalize(normed); }
	
	void rule_node::d(expr& self, char x, ind i) {
		// Break left recursion by returning an inf node
		if ( r.shared->dirty ) {
			self.remake<inf_node>();
			return;
		}
		
		r.shared->dirty = true;  // flag derivative calculations
		expr e = r.clone(i);     // clone rule into new expression
		e.d(x, i);               // take derivative of new expression
		r.shared->dirty = false; // lower calculation flag
		
		// Overwrite self with new derivative temporary (deletes this)
		self = std::move(e); return;
	}
	
	gen_set rule_node::match(ind i) const {
		if ( ! cache.flags.match ) {
			// Ensure no infinite recursion
			cache.set_match( gen_set{} );
			// Calculate match on contained gen-0 rule
			cache.set_match( r.match(0) );
		}
		return cache.match;
	}
	
	gen_set rule_node::back(ind i)  const {
		if ( ! cache.flags.back ) {
			// Ensure no infinite recursion
			cache.set_back( gen_set{0} );
			// Calculate back on contained gen-0 rule
			cache.set_back( r.back(0) );
		}
		return cache.back;
	}
	
	// not_node ////////////////////////////////////////////////////////////////////
	
	expr not_node::make(expr&& e, ind i) {
		switch ( e.type() ) {
		case fail_type: return look_node::make(1);  // Match on subexpression failure
		case inf_type:  return std::move(e);        // Propegate infinite loop
		default:        break;
		}
		
		// return failure on subexpression success
		if ( ! e.match(i).empty() ) return fail_node::make();
		
		return expr::make<not_node>(std::move(e));
	}
	
	expr not_node::clone(ind i) const { return expr::make<not_node>(std::move(e.clone(i))); }
	
	void not_node::normalize(expr& self, expr_set& normed) {
		e.normalize(normed);
		
		switch ( e.type() ) {
		case fail_type: self.remake<look_node>(1); return;
		case inf_type:  self.remake<inf_node>();   return;
		default:                                   break;
		}
		
		if ( ! e.match().empty() ) {
			self.remake<fail_node>();
			return;
		}
	}
	
	// Take negative lookahead of subexpression derivative
	void not_node::d(expr& self, char x, ind i) {
		e.d(x, i);  // TAKE DERIVATIVE OF e
		
		// normalize
		switch ( e.type() ) {
		case fail_type: self.remake<look_node>(1); return;
		case inf_type:  self.remake<inf_node>();   return;
		default:                                   break;
		}
		
		if ( ! e.match(i+1).empty() ) {
			self.remake<fail_node>();
			return;
		}
	}
	
	gen_set not_node::match(ind) const { return gen_set{}; }
	
	gen_set not_node::back(ind) const { return gen_set{1}; }
	
	// map_node ////////////////////////////////////////////////////////////////////
	
	expr map_node::make(expr&& e, const gen_map& eg, gen_type gm, ind i) {
		// account for unmapped generations
		assert(!eg.empty() && "non-empty generation map");
		assert(e.back(i).max() <= eg.max_key() && "no unmapped generations");
		assert(eg.max() <= gm && "max is actually max");
		
		switch ( e.type() ) {
		// Map expression match generation into exit generation
		case eps_type:  return look_node::make(eg(0));
		case look_type: return look_node::make(eg(e.match(i).max()));
		// Propegate fail and infinity errors
		case fail_type: return std::move(e); // a fail node
		case inf_type:  return std::move(e); // an inf node
		default:        break; // do nothing
		}
		
		// check if map isn't needed (identity map)
		if ( gm == eg.max_key() ) return std::move(e);
		
		return expr::make<map_node>(std::move(e), eg, gm);
	}
	
	expr map_node::clone(ind i) const {
		return expr::make<map_node>(std::move(e.clone(i)), eg, gm, cache);
	}
	
	void map_node::normalize(expr& self, expr_set& normed) {
		e.normalize(normed);
		
		switch ( e.type() ) {
		// Map expression match generation into exit generation
		case eps_type:  self.remake<look_node>(eg(0));               return;
		case look_type: self.remake<look_node>(eg(e.match().max())); return;
		// Propegate fail and infinity errors
		case fail_type: self.remake<fail_node>();                    return;
		case inf_type:  self.remake<inf_node>();                     return;
		default:                                                     break;
		}
	}
	
	void map_node::d(expr& self, char x, ind i) {
		cache.invalidate();
		gen_type ebm = e.back(i).max();
		e.d(x, i);  // TAKE DERIV OF e
		
		// Normalize
		switch ( e.type() ) {
		// Map subexpression match into exit generation
		case eps_type:  self = look_node::make(eg(0));                  return;
		case look_type: self = look_node::make(eg(e.match(i+1).max())); return;
		// Propegate fail and infinity errors
		case fail_type: self.remake<fail_node>();                       return;
		case inf_type:  self.remake<inf_node>();                        return;
		default:                                                        break;
		}
		
		// Add new mapping if needed
		update_back_map(eg, ebm, e, gm, i+1);
	}
	
	gen_set map_node::match(ind i) const {
		if ( ! cache.flags.match ) { cache.set_match( eg(e.match(i)) ); }
		return cache.match;
	}
	
	gen_set map_node::back(ind i) const {
		if ( ! cache.flags.back ) { cache.set_back( eg(e.back(i)) ); }
		return cache.back;
	}
	
	// alt_node ////////////////////////////////////////////////////////////////////
	
	expr alt_node::make(expr&& a, expr&& b) {
		switch ( a.type() ) {
		// if first alternative fails, use second
		case fail_type: return std::move(b);
		// if first alternative is infinite loop, propegate
		case inf_type:  return std::move(a);
		default:        break;
		}
		
		// if first alternative matches or second alternative fails, use first
		if ( b.type() == fail_type || ! a.match().empty() ) return std::move(a);
		
		bool did_inc = false;
		gen_map ag = default_back_map(a, did_inc);
		gen_map bg = default_back_map(b, did_inc);
		return expr::make<alt_node>(std::move(a), std::move(b), ag, bg, did_inc ? 1 : 0);
	}
	
	expr alt_node::make(expr&& a, expr&& b, const gen_map& ag, const gen_map& bg, 
	                    gen_type gm, ind i) {
		assert(gm >= ag.max() && gm >= bg.max() && "gm is actual maximum");
	    
		switch ( a.type() ) {
		// if first alternative fails, use second
		case fail_type: return map_node::make(std::move(b), bg, gm, i);
		// if first alternative is infinite loop, propegate
		case inf_type:  return std::move(a);
		default:        break;
		}
		
		// if first alternative matches or second alternative fails, use first
		if ( b.type() == fail_type || ! a.match(i).empty() ) {
			return map_node::make(std::move(a), ag, gm, i);
		}
		
		return expr::make<alt_node>(std::move(a), std::move(b), ag, bg, gm);
	}
	
	expr alt_node::clone(ind i) const {
		return expr::make<alt_node>(std::move(a.clone(i)), std::move(b.clone(i)),
		                            ag, bg, gm, cache);
	}
	
	void alt_node::normalize(expr& self, expr_set& normed) {
		a.normalize(normed);
		b.normalize(normed);
		
		switch ( a.type() ) {
		// if first alternative fails, use second
		case fail_type: self = std::move(b);     return;
		// if first alternative is infinite loop, propegate
		case inf_type:  self.remake<inf_node>(); return;
		default:                                 break;
		}
		
		// if first alternative matches or second alternative fails, use first
		if ( b.type() == fail_type || ! a.match().empty() ) {
			self = std::move(a);
			return;
		}
	}
	
	void alt_node::d(expr& self, char x, ind i) {
		cache.invalidate();
		gen_type abm = a.back(i).max();
		a.d(x, i);  // TAKE DERIV OF a
		
		// Check conditions on a before we calculate dx(b) [same as make()]
		switch ( a.type() ) {
		case fail_type: {
			// Return map of b
			gen_type bbm = b.back(i).max(); 
			b.d(x, i);  // TAKE DERIV OF b
			update_back_map(bg, bbm, b, gm, i+1);
			self = map_node::make(std::move(b), bg, gm, i+1);
			return;
		}
		case inf_type:  self.remake<inf_node>(); return;
		default:                                 break;
		}
		
		// Map in new lookahead generations for derivative
		bool did_inc = false;
		update_back_map(ag, abm, a, gm, did_inc, i+1);
		
		// Eliminate second alternative if first matches
		if ( ! a.match(i+1).empty() ) {
			self = map_node::make(std::move(a), ag, did_inc ? gm+1 : gm, i+1);
			return;
		}
		
		// Calculate other derivative and map in new lookahead generations
		gen_type bbm = b.back(i).max();
		b.d(x, i);  // TAKE DERIV OF b
		
		// Eliminate second alternative if it fails
		if ( b.type() == fail_type ) {
			self = map_node::make(std::move(a), ag, did_inc ? gm+1 : gm, i+1);
			return;
		}
		update_back_map(bg, bbm, b, gm, did_inc, i+1);
		
		if ( did_inc ) { ++gm; }
	}
	
	gen_set alt_node::match(ind i) const {
		if ( ! cache.flags.match ) {
			cache.set_match( ag(a.match(i)) | bg(b.match(i)) );
		}
		return cache.match;
	}
	
	gen_set alt_node::back(ind i) const {
		if ( ! cache.flags.back ) {
			cache.set_back( ag(a.back(i)) | bg(b.match(i)) );
		}
		return cache.back;
	}
	
	// seq_node ////////////////////////////////////////////////////////////////////
	
	expr seq_node::make(expr&& a, expr&& b) {
		switch ( b.type() ) {
		// empty second element just leaves first
		case eps_type:  return std::move(a);
		// failing second element propegates
		case fail_type: return std::move(b);
		default: break;  // do nothing
		}
		
		switch ( a.type() ) {
		// empty first element or lookahead success just leaves follower
		case eps_type: case look_type:
			return std::move(b);
		// failure or infinite loop propegates
		case fail_type: case inf_type:
			return std::move(a);
		default: break;  // do nothing
		}
		
		bool did_inc = false;
		
		// Set up match-fail follower
		expr c = fail_node::make();
		gen_map cg{0};
		
		gen_set am = a.match();
		if ( ! am.empty() && am.min() == 0 ) {
			c = b.clone();
			update_back_map(cg, 0, b, 0, did_inc, 0);
		}
		
		// set up lookahead follower
		look_list bs;
		if ( a.back().max() > 0 ) {
			assert(a.back().max() == 1 && "static backtrack gen <= 1");
			
			gen_type gl = 0;
			gen_set bm = b.match();
			if ( ! bm.empty() && bm.min() == 0 ) {
				gl = 1;
				did_inc = true;
			}
			
			bs.emplace_front(1, b.clone(), default_back_map(b, did_inc), gl);
		}
		
		return expr::make<seq_node>(std::move(a), std::move(b), std::move(bs), 
		                            std::move(c), cg, did_inc ? 1 : 0);
	}
	
	expr seq_node::clone(ind i) const {
		// clone lookahead list
		look_list cbs;
		auto cbt = cbs.before_begin();
		for (const look_back& bi : bs) {
			cbt = cbs.emplace_after(cbt, bi.g, bi.e.clone(i), bi.eg, bi.gl);
		}
		
		return expr::make<seq_node>(std::move(a.clone(i)), std::move(b.clone(i)), 
		                            std::move(cbs), std::move(c.clone(i)), cg, gm, cache);
	}
	
	void seq_node::normalize(expr& self, expr_set& normed) {
		a.normalize(normed);
		b.normalize(normed);
		
		switch ( b.type() ) {
		// empty second element just leaves first
		case eps_type:  self = std::move(a); return;
		// failing second element propegates
		case fail_type: self = std::move(b); return;
		default:                             break;
		}
		
		switch ( a.type() ) {
		// empty first element or lookahead success just leaves follower
		case eps_type: case look_type:
			self = std::move(b);
			return;
		// failure or infinite loop propegates
		case fail_type: case inf_type:
			self = std::move(a);
			return;
		default: break;
		}
		
		gm = 0;
		bool did_inc = false;
		
		// Set up match-fail follower
		c.remake<fail_node>();
		cg = gen_map{0};
		
		gen_set am = a.match();
		if ( ! am.empty() && am.min() == 0 ) {
			c = b.clone();
			update_back_map(cg, 0, b, 0, did_inc, 0);
		}
		
		// Set up lookahead follower
		bs.clear();
		if ( a.back().max() > 0 ) {
			assert(a.back().max() == 1 && "static backtrack gen <= 1");
			
			gen_type gl = 0;
			gen_set bm = b.match();
			if ( ! bm.empty() && bm.min() == 0 ) {
				gl = 1;
				did_inc = true;
			}
			
			bs.emplace_front(1, b.clone(), default_back_map(b, did_inc), gl);
		}
		
		// Fix gm
		if ( did_inc ) ++gm;
	}
	
	void seq_node::d(expr& self, char x, ind i) {
		cache.invalidate();
		gen_type abm = a.back(i).max();
		a.d(x, i);  // TAKE DERIV OF a
		
		// Handle empty or failure results from predecessor derivative
		switch ( a.type() ) {
		case eps_type: {
			// Take follower (or follower's end-of-string derivative on end-of-string)
			if ( x == '\0' ) {
				b.d('\0', i);  // TAKE DERIV OF b
				gen_map bg = new_back_map(b, gm, i+1);
				self = map_node::make(std::move(b), bg, gm, i+1);
				return;
			} else {
				expr bn = b.clone(i+1);
				gen_map bg = new_back_map(bn, gm, i+1);
				self = map_node::make(std::move(bn), bg, gm, i+1);
				return;
			}
		} case look_type: {
			// Take lookahead follower (or lookahead follower match-fail)
			gen_type g = a.match(i+1).max();
			for (look_back& bi : bs) {
				// find node in (sorted) generation list
				if ( bi.g < g ) continue;
				if ( bi.g > g ) {
					self.remake<fail_node>();
					return;
				}
				
				gen_type bibm = bi.e.back(i).max();
				bi.e.d(x, i);  // TAKE DERIV OF bi
				
				if ( bi.e.type() == fail_type ) {  // straight path fails ...
					if ( bi.gl > 0 ) {                // ... but matched in the past
						self.remake<look_node>(bi.gl);   // return appropriate lookahead
						return;
					} else {                          // ... and no previous match
						self.remake<fail_node>();        // return a failure expression
						return;
					}
				}
				
				update_back_map(bi.eg, bibm, bi.e, gm, i+1);
				
				// if there is no failure backtrack (or this generation is it) 
				// we don't have to track it
				gen_set dbim = bi.e.match(i+1);
				if ( bi.gl == 0 || (! dbim.empty() && dbim.min() == 0) ) {
					self = map_node::make(std::move(bi.e), bi.eg, gm, i+1);
					return;
				}
				
				// Otherwise return alt-expr of this lookahead and its failure backtrack
				self = alt_node::make(std::move(bi.e), look_node::make(), 
				                      bi.eg, gen_map{0,bi.gl}, gm);
				return;
			}
			// end-of-string is only case where we can get a lookahead success for an unseen gen
			if ( x == '\0' ) {
				b.d('\0', i);  // TAKE DERIV OF b
				gen_map bg = new_back_map(b, gm, i+1);
				self = map_node::make(std::move(b), bg, gm, i+1);
				return;
			}
			self.remake<fail_node>(); // if lookahead follower not found, fail
			return;
		} case fail_type: {
			// Return derivative of match-fail follower
			gen_type cbm = c.back(i).max();
			c.d(x, i);  // TAKE DERIV OF c
			update_back_map(cg, cbm, c, gm, i+1);
			self = map_node::make(std::move(c), cg, gm, i+1);
			return;
		} case inf_type: {
			// Propegate infinite loop
			self.remake<inf_node>();
			return;
		} default: {}  // do nothing
		}
		
		bool did_inc = false;
		
		// Update match-fail follower
		gen_set dam = a.match(i+1);
		if ( ! dam.empty() && dam.min() == 0 ) {
			// new failure backtrack
			c = b.clone(i+1);
			cg = new_back_map(b, gm, did_inc, i+1);
		} else {
			// continue previous failure backtrack
			gen_type cbm = c.back(i).max();
			c.d(x, i);  // TAKE DERIV OF c
			update_back_map(cg, cbm, c, gm, did_inc, i+1);
		}
		
		// Build derivatives of lookahead backtracks
		gen_set dab = a.back(i+1);
		auto dabt = dab.begin();
		assert(dabt != dab.end() && "backtrack set non-empty");
		if ( *dabt == 0 ) { ++dabt; }  // skip backtrack generation 0
		auto bit = bs.begin();
		auto bilt = bs.before_begin();

		while ( dabt != dab.end() && bit != bs.end() ) {
			look_back& bi = *bit;
			
			// erase generations not in the backtrack list
			if ( bi.g < *dabt ) {
				bit = bs.erase_after(bilt);
				continue;
			}
			assert(bi.g == *dabt && "no generations missing from backtrack list");
			
			// take derivative of lookahead
			gen_type bibm = bi.e.back(i).max();
			bi.e.d(x, i);  // TAKE DERIV OF bi
			update_back_map(bi.eg, bibm, bi.e, gm, did_inc, i+1);
			
			gen_set dbim = bi.e.match(i+1);
			if ( ! dbim.empty() && dbim.min() == 0 ) {  // set new match-fail backtrack if needed
				++bi.gl;
				did_inc = true;
			}
			
			++dabt; bilt = bit; ++bit;
		}
		
		// Add new lookahead backtrack if needed
		if ( dabt != dab.end() ) {
			gen_type dabm = *dabt;
			assert(dabm > abm && "leftover generation greater than previous");
			assert(++dabt == dab.end() && "only one new lookahead backtrack");
			
			gen_type gl = 0;
			gen_set bm = b.match(i);
			if ( ! bm.empty() && bm.min() == 0 ) {  // set new match-fail backtrack if needed
				gl = gm+1;
				did_inc = true;
			}
			bs.emplace_after(bilt, 
			                 dabm, b.clone(i+1), new_back_map(b, gm, did_inc, i+1), gl);
		}
		
		if ( did_inc ) ++gm;
	}
	
	gen_set seq_node::match(ind i) const {
		if ( cache.flags.match ) return cache.match;
		
		// Include matches from match-fail follower
		gen_set x = cg(c.match(i));
		
		// Include matches from matching lookahead successors
		gen_set am = a.match(i);
		
		auto at = am.begin();
		auto bit = bs.begin();
		while ( at != am.end() && bit != bs.end() ) {
			const look_back& bi = *bit;
			gen_type         ai = *at;
			
			// Find matching generations
			if ( bi.g < ai ) { ++bit; continue; }
			else if ( bi.g > ai ) { ++at; continue; }
			
			// Add followers to match set as well as follower match-fail
			x |= bi.eg(bi.e.match(i));
			if ( bi.gl > 0 ) x |= bi.gl;
			
			++at; ++bit;
		}
		
		cache.set_match(x);
		return cache.match;
	}
	
	gen_set seq_node::back(ind i) const {
		if ( cache.flags.back ) return cache.back;
		
		// Check for gen-zero backtrack from predecessor
		gen_set x = ( a.back(i).min() == 0 ) ? gen_set{0} : gen_set{};
		
		// Include backtrack from match-fail follower
		x |= cg(c.back(i));
		
		// Include lookahead follower backtracks
		for (const look_back& bi : bs) {
			x |= bi.eg(bi.e.back(i));
			if ( bi.gl > 0 ) x |= bi.gl;
		}
		
		cache.set_back(x);
		return cache.back;
	}
	
} // namespace derivs
