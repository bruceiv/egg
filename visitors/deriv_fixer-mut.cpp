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

#include "../derivs-mut.hpp"
#include "deriv_fixer-mut.hpp"

namespace derivs {
	
	void fixer::operator() (expr& x) {
		if ( fixed.count(&x) ) return;
		
		fix_match(x);
	}
	
	gen_set fixer::fix_match(expr& x) {
		// Fast path for expressions that can't have recursively-defined match set
		switch ( x.type() ) {
		case fail_type: case inf_type:   case eps_type: case look_type:
		case char_type: case range_type: case any_type: case str_type: 
		case not_type:
			fixed.insert(&x);
			return x.match();
		default: break;
		}
		
		// Fixed point calculation for recursive expressions
		bool changed = false;
		expr_set visited;
		
		running.insert(&x);
		
		// recalculate match expression until it doesn't change; 
		// by Kleene's thm this is a fixed point
		gen_set match_set{};
		while (true) {
			match_set = iter_match(x, changed, visited);
			if ( ! changed ) break;
			changed = false;
			visited.clear();
		}
		
		running.erase(&x);
		fixed.insert(&x);
		
		return match_set;
	}
	
	gen_set fixer::iter_match(expr& x, bool& changed, expr_set& visited) {
		if ( fixed.count(&x) ) return x.match();
		if ( ! running.count(&x) ) return fix_match(x);
		if ( visited.count(&x) ) return x.match(); else visited.insert(&x);
		
		gen_set old_match = x.match();
		gen_set new_match = calc_match(x, changed, visited);
		if ( new_match != old_match ) {
			changed = true;
		}
		
		return new_match;
	}
	
	gen_set fixer::calc_match(expr& x, bool& changed, expr_set& visited) {
		this->changed = &changed;
		this->visited = &visited;
		x.accept(this);
		return match;
	}
	
	void fixer::visit(fail_node&)   { match = gen_set{}; }
	void fixer::visit(inf_node&)    { match = gen_set{}; }
	void fixer::visit(eps_node&)    { match = gen_set{0}; }
	void fixer::visit(look_node& x) { match = gen_set{x.b}; }
	void fixer::visit(char_node&)   { match = gen_set{}; }
	void fixer::visit(range_node&)  { match = gen_set{}; }
	void fixer::visit(any_node&)    { match = gen_set{}; }
	void fixer::visit(str_node&)    { match = gen_set{}; }
	
	void fixer::visit(shared_node& x) {
		// Stop this from infinitely recursing
		if ( ! x.shared->crnt_cache.flags.match ) {
			x.shared->crnt_cache.set_match( gen_set{} );
		}
		
		// Calculate and cache match set
		match = x.shared->crnt_cache.match 
			  = iter_match(x.shared->e, *changed, *visited);
	}
	
	void fixer::visit(rule_node& x) {
		// Defer to contained shared_node and cache match set
		x.r.accept(this);
		x.cache.set_match(match);
	}
	
	void fixer::visit(not_node& x)  {
		// Make sure subexpressions are fixed
		iter_match(x.e, *changed, *visited);
		// Return static match value
		match = gen_set{1};
	}
	
	void fixer::visit(map_node& x)  {
		// Calculate and cache match set
		match = x.eg(iter_match(x.e, *changed, *visited));
		x.cache.set_match(match);
	}
	
	void fixer::visit(alt_node& x)  {
		// Save changed and visited references, in case first iter_match overwrites
		bool& changed = *(this->changed);
		expr_set& visited = *(this->visited);
		
		// Calculate and cache match set
		match = x.ag(iter_match(x.a, changed, visited)) 
		        | x.bg(iter_match(x.b, changed, visited));
		x.cache.set_match(match);
	}
	
	void fixer::visit(seq_node& x)    {
		// Save changed and visited references, in case iter_match overwrites
		bool& changed = *(this->changed);
		expr_set& visited = *(this->visited);
		
		gen_set am = iter_match(x.a, changed, visited);
		auto at = am.begin();
		// Make sure successor is fixed also
		iter_match(x.b, changed, visited);
		
		// initialize match-fail follower if uninitialized and needed
		if ( at != am.end() && *at == 0 ) {
			// Add backtrack node
			if ( x.c.type() == fail_type ) {
				x.c = x.b.clone();
				update_back_map(x.cg, 0, x.b, x.gm, 0);
			}
			++at;
		}
		// Include matches from match-fail follower
		gen_set m = x.cg(iter_match(x.c, changed, visited));
		
		// Include matches from matching lookahead successors
		auto bit = x.bs.begin();
		while ( at != am.end() && bit != x.bs.end() ) {
			auto& bi = *bit;
			auto  ai = *at;
		
			// Find matching generations
			if ( bi.g < ai ) {
				// Make sure generation is fixed
				iter_match(bi.e, changed, visited);
				// skip adding generation to match set
				++bit; continue;
			}
			else if ( bi.g > ai ) { ++at; continue; }
		
			// Add followers to match set as well as follower match-fail
			gen_set bm = bi.eg(iter_match(bi.e, changed, visited));
			m |= bm;
			
			if ( bi.gl > 0 ) m.add_back(bi.gl);
			else if ( ! bm.empty() && bm.min() == 0 ) {
				bi.gl = 1;
				m |= bi.gl;
			}
		
			++at; ++bit;
		}
		
		match = m;
		x.cache.set_match(match);
	}
	
} // namespace derivs
