#include "derivs.hpp"

#include <iostream>
#include "visitors/deriv_printer.hpp"

namespace derivs {
	
	// expr ////////////////////////////////////////////////////////////////////////
	
	gen_set expr::new_back_map(ptr<expr> e, gen_type gm, bool& did_inc) {
		assert(!e->back().empty() && "backtrack set never empty");
		
		gen_set eg = zero_set;
		if ( e->back().max() > 0 ) {
			assert(e->back().max() == 1 && "static lookahead gen <= 1");
			did_inc = true;
			eg |= gm + 1;
		}
		
		return eg;
	}
	
	gen_set expr::default_back_map(ptr<expr> e, bool& did_inc) {
		assert(!e->back().empty() && "backtrack set never empty");
		if ( e->back().max() > 0 ) {
			assert(e->back().max() == 1 && "static lookahead gen <= 1");
			did_inc = true;
			return zero_one_set;
		} else {
			return zero_set;
		}
	}
	
	gen_set expr::update_back_map(ptr<expr> e, ptr<expr> de, gen_set eg, 
	                               gen_type gm, bool& did_inc) {
		assert(!e->back().empty() && !de->back().empty() && "backtrack set never empty");
		
		gen_set deg = eg;
		if ( de->back().max() > e->back().max() ) {
//			assert(de->back().max() == e->back().max() + 1 && "gen only grows by 1");
			deg |= gm+1;
			did_inc = true;
		}
		
		return deg;
	}
	
	const gen_set expr::empty_set{};
	const gen_set expr::zero_set{0};
	const gen_set expr::one_set{1};
	const gen_set expr::zero_one_set{0,1};
	
	// memo_expr ///////////////////////////////////////////////////////////////////
	
	void memo_expr::reset_memo() {
		memo_match = expr::empty_set;
		memo_back = expr::empty_set;
		flags = {false,false};
	}
	
	ptr<expr> memo_expr::d(char x) const {
		auto ix = memo.find(const_cast<memo_expr* const>(this));
		if ( ix == memo.end() ) {
			// no such item; compute and store
			ptr<expr> dx = deriv(x);
			memo.insert(ix, std::make_pair(const_cast<memo_expr* const>(this), dx));
			return dx;
		} else {
			// derivative found, return
			return ix->second;
		}
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
	
	ptr<expr> fail_expr::make() { return expr::make_ptr<fail_expr>(); }
	
	// A failure expression can't un-fail - no strings to match with any prefix
	ptr<expr> fail_expr::d(char) const { return fail_expr::make(); }
		
	gen_set fail_expr::match() const { return expr::empty_set; }
	
	gen_set fail_expr::back()  const { return expr::zero_set; }
	
	// inf_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> inf_expr::make() { return expr::make_ptr<inf_expr>(); }
	
	// An infinite loop expression never breaks, ill defined with any prefix
	ptr<expr> inf_expr::d(char) const { return inf_expr::make(); }
	
	gen_set inf_expr::match() const { return expr::empty_set; }
	
	gen_set inf_expr::back()  const { return expr::zero_set; }
	
	// eps_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> eps_expr::make() { return expr::make_ptr<eps_expr>(); }
	
	// Only succeeds if string is empty
	ptr<expr> eps_expr::d(char x) const {
		return ( x == '\0' ) ? eps_expr::make() : fail_expr::make();
	}
	
	gen_set eps_expr::match() const { return expr::zero_set; }
	
	gen_set eps_expr::back()  const { return expr::zero_set; }
	
	// look_expr ///////////////////////////////////////////////////////////////////
	
	ptr<expr> look_expr::make(gen_type g) {
		return (g == 0) ? expr::make_ptr<eps_expr>() : expr::make_ptr<look_expr>(g);
	}
	
	// No prefixes to remove from language containing the empty string; all fail
	ptr<expr> look_expr::d(char x) const { return look_expr::make(b); }
/*	ptr<expr> look_expr::d(char x) const {  
		return ( x == '\0' ) ? look_expr::make(b) : fail_expr::make();
	}
*/	
	gen_set look_expr::match() const { return gen_set{b}; }
	
	gen_set look_expr::back()  const { return gen_set{b}; }
	
	// char_expr ///////////////////////////////////////////////////////////////////
	
	ptr<expr> char_expr::make(char c) { return expr::make_ptr<char_expr>(c); }
	
	// Single-character expression either consumes matching character or fails
	ptr<expr> char_expr::d(char x) const {
		return ( c == x ) ? eps_expr::make() : fail_expr::make();
	}
	
	gen_set char_expr::match() const { return expr::empty_set; }
	
	gen_set char_expr::back()  const { return expr::zero_set; }
	
	// range_expr //////////////////////////////////////////////////////////////////
	
	ptr<expr> range_expr::make(char b, char e) { return expr::make_ptr<range_expr>(b, e); }
	
	// Character range expression either consumes matching character or fails
	ptr<expr> range_expr::d(char x) const {
		return ( b <= x && x <= e ) ? eps_expr::make() : fail_expr::make();
	}
	
	gen_set range_expr::match() const { return expr::empty_set; }
	
	gen_set range_expr::back()  const { return expr::zero_set; }
	
	// any_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> any_expr::make() { return expr::make_ptr<any_expr>(); }
	
	// Any-character expression consumes any character
	ptr<expr> any_expr::d(char x) const {
		return ( x == '\0' ) ? fail_expr::make() : eps_expr::make();
	}
	
	gen_set any_expr::match() const { return expr::empty_set; }
	
	gen_set any_expr::back()  const { return expr::zero_set; }
	
	// str_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> str_expr::make(std::string s) {
		switch ( s.size() ) {
		case 0:  return eps_expr::make();
		case 1:  return char_expr::make(s[0]);
		default: return expr::make_ptr<str_expr>(s);
		}
	}
	
	ptr<expr> str_expr::d(char x) const {
		// Check that the first character matches
		if ( s[0] != x ) return fail_expr::make();
		
		// Otherwise return a character or string expression, as appropriate
		if ( s.size() == 2 ) return char_expr::make(s[1]);
		
		std::string t(s, 1);
		return expr::make_ptr<str_expr>(t);
	}
	
	gen_set str_expr::match() const { return expr::empty_set; }
	
	gen_set str_expr::back()  const { return expr::zero_set; }
	
	// rule_expr ///////////////////////////////////////////////////////////////////
	
	ptr<expr> rule_expr::make(memo_expr::table& memo, ptr<expr> r) {
		return expr::make_ptr<rule_expr>(memo, r);
	}
	
	ptr<expr> rule_expr::deriv(char x) const {
		// signal infinite loop if we try to take this derivative again
		memo[const_cast<rule_expr* const>(this)] = inf_expr::make();
		// calculate derivative
		ptr<expr> rVal = r->d(x);
		// clear infinite loop signal and return
		memo.erase(const_cast<rule_expr* const>(this));
		return rVal;
	}
	
	gen_set rule_expr::match_set() const {
		// Stop this from infinitely recursing
		flags.match = true;
		memo_match = expr::empty_set;
		
		// Calculate match set
		return r->match();
	}
	
	gen_set rule_expr::back_set() const {
		// Stop this from infinitely recursing
		flags.back = true;
		memo_back = expr::zero_set;
		
		// Calculate backtrack set
		return r->back();
	}
	
	// not_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> not_expr::make(memo_expr::table& memo, ptr<expr> e) {
		switch ( e->type() ) {
		case fail_type: return look_expr::make(1);  // return match on subexpression failure
		case inf_type:  return e;                   // propegate infinite loop
		default:        break;
		}
		
		// return failure on subexpression success
		if ( ! e->match().empty() ) return fail_expr::make();
		
		return expr::make_ptr<not_expr>(memo, e);
	}
	
	// Take negative lookahead of subexpression derivative
	ptr<expr> not_expr::deriv(char x) const { return not_expr::make(memo, e->d(x)); }
	
	gen_set not_expr::match_set() const { return expr::empty_set; }
	
	gen_set not_expr::back_set() const { return expr::one_set; }
	
	// map_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> map_expr::make(memo_expr::table& memo, ptr<expr> e, gen_type gm, gen_set eg) {
		// account for unmapped generations
		assert(e->back().max() < eg.count() && "no unmapped generations");
		assert(eg.max() <= gm && "max is actually max");
//		auto n = eg.count();
//		for (auto i = e->back().max() + 1; i < n; ++i) eg |= ++gm;
		
		switch ( e->type() ) {
		// Map expression match generation into exit generation
		case eps_type:  return look_expr::make(eg(0));
		case look_type: return look_expr::make(eg(e->match().max()));
		// Propegate fail and infinity errors
		case fail_type: return e; // a fail_expr
		case inf_type:  return e; // an inf_expr
		default:        break; // do nothing
		}
		
		// check if map isn't needed (identity map)
		if ( gm + 1 == eg.count() ) return e;
		
		return expr::make_ptr<map_expr>(memo, e, gm, eg);
	}
	
	ptr<expr> map_expr::deriv(char x) const {
		ptr<expr> de = e->d(x);
		
		// Check conditions on de [same as make]
		switch ( de->type() ) {
		// Map expression match generation into exit generation
		case eps_type:  return look_expr::make(eg(0));
		case look_type: return look_expr::make(eg(de->match().max()));
		// Propegate fail and infinity errors
		case fail_type: return de; // a fail_expr
		case inf_type:  return de; // an inf_expr
		default:        break; // do nothing
		}
		
		// Calculate generations of new subexpressions
		// - If we've added a lookahead generation that wasn't there before, map it into the 
		//   generation space of the derived alternation
		bool did_inc = false;
		gen_set deg = expr::update_back_map(e, de, eg, gm, did_inc);
		return expr::make_ptr<map_expr>(memo, de, gm + did_inc, deg);
	}
	
	gen_set map_expr::match_set() const { return eg(e->match()); }
	
	gen_set map_expr::back_set() const { return eg(e->back()); }
	
	// alt_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> alt_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b) {
		switch ( a->type() ) {
		// if first alternative fails, use second
		case fail_type: return b;
		// if first alternative is infinite loop, propegate
		case inf_type:  return a; // an inf_expr
		default:        break; // do nothing
		}
		
		// if first alternative matches or second alternative fails, use first
		if ( b->type() == fail_type || ! a->match().empty() ) return a;
		
		bool did_inc = false;
		gen_set ag = expr::default_back_map(a, did_inc);
		gen_set bg = expr::default_back_map(b, did_inc);
		return expr::make_ptr<alt_expr>(memo, a, b, ag, bg, 0 + did_inc);
	}
	
	ptr<expr> alt_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, 
	                         gen_set ag, gen_set bg, gen_type gm) {
	    assert(!(ag.empty() || bg.empty()) && "backtrack maps non-empty");
	    assert(gm >= ag.max() && gm >= bg.max() && "gm is actual maximum");
	    
		switch ( a->type() ) {
		// if first alternative fails, use second
		case fail_type: return map_expr::make(memo, b, gm, bg);
		// if first alternative is infinite loop, propegate
		case inf_type:  return a; // an inf_expr
		default:        break; // do nothing
		}
		
		// if first alternative matches or second alternative fails, use first
		if ( b->type() == fail_type || ! a->match().empty() ) {
			return map_expr::make(memo, a, gm, ag);
		}
		
		return expr::make_ptr<alt_expr>(memo, a, b, ag, bg, gm);
	}
	
	ptr<expr> alt_expr::deriv(char x) const {
		bool did_inc = false;
		
		// Calculate derivative and map in new lookahead generations
		ptr<expr> da = a->d(x);
		
		// Check conditions on a before we calculate dx(b) [same as make()]
		switch ( da->type() ) {
		case fail_type: {
			ptr<expr> db = b->d(x);
			gen_set dbg = expr::update_back_map(b, db, bg, gm, did_inc);
			return map_expr::make(memo, db, gm + did_inc, dbg);
		}
		case inf_type:  return da; // an inf_expr
		default:        break; // do nothing
		}
		
		// Map in new lookahead generations for derivative
		gen_set dag = expr::update_back_map(a, da, ag, gm, did_inc);
		
		if ( ! da->match().empty() ) {
			return map_expr::make(memo, da, gm + did_inc, dag);
		}
		
		// Calculate other derivative and map in new lookahead generations
		ptr<expr> db = b->d(x);
		if ( db->type() == fail_type ) return map_expr::make(memo, da, gm + did_inc, dag);
		gen_set dbg = expr::update_back_map(b, db, bg, gm, did_inc);
				
		return expr::make_ptr<alt_expr>(memo, da, db, dag, dbg, gm + did_inc);
	}
	
	gen_set alt_expr::match_set() const { return ag(a->match()) | bg(b->match()); }
	
	gen_set alt_expr::back_set() const { return ag(a->back()) | bg(b->back()); }
	
	// seq_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> seq_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b) {
		switch ( b->type() ) {
		// empty second element just leaves first
		case eps_type:  return a;
		// failing second element propegates
		case fail_type: return b;
		default: break;  // do nothing
		}
		
		switch ( a->type() ) {
		// empty first element or lookahead success just leaves follower
		case eps_type:
		case look_type:
			return b;
		// failure or infinite loop propegates
		case fail_type:
		case inf_type:
			return a;
		default: break;  // do nothing
		}
		
		bool did_inc = false;
		
		// Set up match-fail follower
		ptr<expr> c;
		gen_set   cg;
		gen_set am = a->match();
		if ( ! am.empty() && am.min() == 0 ) {
			c = b;
			cg = expr::default_back_map(c, did_inc);
		} else {
			c = fail_expr::make();
			cg = expr::zero_set;
		}
		
		// set up lookahead follower
		look_list bs;
		assert(!a->back().empty() && "backtrack set is always non-empty");
		if ( a->back().max() > 0 ) {
			assert(a->back().max() == 1 && "static backtrack gen <= 1");
			
			gen_type gl = 0;
			gen_set bm = b->match();
			if ( ! bm.empty() && bm.min() == 0 ) {
				gl = 1;
				did_inc = true;
			}
			
			bs.emplace_back(1, expr::default_back_map(b, did_inc), b, gl);
		}
		
		// return constructed expression
		return expr::make_ptr<seq_expr>(memo, a, b, bs, c, cg, 0 + did_inc);
	}
	
	ptr<expr> seq_expr::deriv(char x) const {
		bool did_inc = false;
		ptr<expr> da = a->d(x);
		
		// Handle empty or failure results from predecessor derivative
		switch ( da->type() ) {
		case eps_type: {
			// Take follower (or follower's end-of-string derivative on end-of-string)
			ptr<expr> bn = ( x == '\0' ) ? b->d('\0') : b;
			gen_set bng = expr::new_back_map(bn, gm, did_inc);
			return map_expr::make(memo, bn, gm + did_inc, bng);
/*			// If end of string, return how the follower handles end-of-string
			if ( x == '\0' ) {
				return b->d('\0');
			}
			
			// Return follower, but with any lookahead gens mapped to their proper generation
			gen_set bg = expr::new_back_map(b, gm, did_inc);
			return map_expr::make(memo, b, gm + did_inc, bg);
*/		} case look_type: {
			// Lookahead follower (or lookahead follower match-fail)
			auto i = std::static_pointer_cast<look_expr>(da)->b;
			for (const look_node& bi : bs) {
				if ( bi.g < i ) continue;
//				if ( bi.g > i ) break;  // generation list is sorted
				if ( bi.g > i ) return fail_expr::make();  // generation list is sorted
				
				ptr<expr> dbi = bi.e->d(x);  // found element, take derivative
				
				if ( dbi->type() == fail_type ) {  // straight path fails ...
					if ( bi.gl > 0 ) {                // ... but matched in the past
						return look_expr::make(bi.gl);   // return appropriate lookahead
					} else {                          // ... and no previous match
						return dbi;                      // return a failure expression
					}
				}
				
				gen_set dbig = expr::update_back_map(bi.e, dbi, bi.eg, gm, did_inc);
				
				// if there is no failure backtrack (or this generation is it) 
				// we don't have to track it
				gen_set dbim = dbi->match();
				if ( bi.gl == 0 || (! dbim.empty() && dbim.min() == 0) ) {
					return map_expr::make(memo, dbi, gm + did_inc, dbig);
				}
				
				// Otherwise return alt-expr of this lookahead and its failure backtrack
				return alt_expr::make(memo, dbi, look_expr::make(),
				                      dbig, gen_set{0,bi.gl}, gm + did_inc);
/*				return alt_expr::make(memo,
				                      dbi, 
				                      seq_expr::make(memo, 
				                                     not_expr::make(memo, dbi), 
				                                     look_expr::make()), 
	                                  dbig, gen_set{0,bi.gl});
*/			}
			// end-of-string is only case where we can get a lookahead success for an unseen gen
			if ( x == '\0' ) {
				ptr<expr> bn = b->d('\0');
				gen_set bng = expr::new_back_map(bn, gm, did_inc);
				return map_expr::make(memo, bn, gm + did_inc, bng);
			}
			return fail_expr::make(); // if lookahead follower not found, fail
		} case fail_type: {
			// Return match-fail follower
			ptr<expr> dc = c->d(x);
			gen_set dcg = expr::update_back_map(c, dc, cg, gm, did_inc);
			return map_expr::make(memo, dc, gm + did_inc, dcg);
		} case inf_type: {
			// Propegate infinite loop
			return da;
		} default: {}  // do nothing
		}
		
		// Construct new match-fail follower
		ptr<expr> dc;
		gen_set dcg;
		
		gen_set dam = da->match();
		if ( ! dam.empty() && dam.min() == 0 ) {
			// new failure backtrack
			dc = b;
			dcg = expr::new_back_map(b, gm, did_inc);
		} else {
			// continue previous failure backtrack
			dc = c->d(x);
			dcg = expr::update_back_map(c, dc, cg, gm, did_inc);
		}
		
		// Build derivatives of lookahead backtracks
		look_list dbs;
		for (const look_node& bi : bs) {
			ptr<expr> dbi = bi.e->d(x);
			gen_set dbig = expr::update_back_map(bi.e, dbi, bi.eg, gm, did_inc);
			gen_type dgl = bi.gl;
			gen_set dbim = dbi->match();
			if ( ! dbim.empty() && dbim.min() == 0 ) {  // set new match-fail backtrack if needed
				dgl = gm+1;
				did_inc = true;
			}
			dbs.emplace_back(bi.g, dbig, dbi, dgl);
		}
		
		// Add new lookahead backtrack if needed
		gen_type dabm = da->back().max();
		if ( dabm > a->back().max() ) {
//			assert(dabm == a->back().max() + 1 && "backtrack gen only grows by 1");
			gen_type gl = 0;
			gen_set bm = b->match();
			if ( ! bm.empty() && bm.min() == 0 ) {  // set new match-fail backtrack if needed
				gl = gm+1;
				did_inc = true;
			}
			dbs.emplace_back(dabm, expr::new_back_map(b, gm, did_inc), b, gl);
		}
		
		return expr::make_ptr<seq_expr>(memo, da, b, dbs, dc, dcg, gm + did_inc);
	}
	
	gen_set seq_expr::match_set() const {
		// Include matches from match-fail follower
		gen_set x = cg(c->match());
		
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
			x |= bi.eg(bi.e->match());
			if ( bi.gl > 0 ) x |= bi.gl;
			
			++at; ++bit;
		}
		
		return x;
	}
		
	gen_set seq_expr::back_set() const {
		// Include backtrack from match-fail follower
		gen_set x = cg(c->back());
		
		// Include lookahead follower backtracks
		for (const look_node& bi : bs) {
			x |= bi.eg(bi.e->back());
			if ( bi.gl > 0 ) x |= bi.gl;
		}
		
		// Check for gen-zero backtrack from predecessor (successors included earlier)
		if ( a->back().min() == 0 ) {
			x |= 0;
		}
		
		return x;
	}
		
} // namespace derivs

