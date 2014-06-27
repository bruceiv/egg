#include "derivs.hpp"

#include <iostream>

#include "visitors/deriv_printer.hpp"

namespace derivs {
	
	std::ostream& operator<< (std::ostream& out, expr_type t) {
		switch (t) {
		case fail_type:  out << "FAIL";  break;
		case inf_type:   out << "INF";   break;
		case eps_type:   out << "EPS";   break;
		case look_type:  out << "LOOK";  break;
		case char_type:  out << "CHAR";  break;
		case range_type: out << "RANGE"; break;
		case any_type:   out << "ANY";   break;
		case str_type:   out << "STR";   break;
		case rule_type:  out << "RULE";  break;
		case not_type:   out << "NOT";   break;
		case map_type:   out << "MAP";   break;
		case alt_type:   out << "ALT";   break;
		case seq_type:   out << "SEQ";   break;
		}
		return out;
	}
	
	// expr ////////////////////////////////////////////////////////////////////////
	
	gen_map expr::new_back_map(ptr<expr> e, gen_type gm, bool& did_inc) {
		assert(!e->back().empty() && "backtrack set never empty");
		
		gen_map eg{0};
		if ( e->back().max() > 0 ) {
			assert(e->back().max() == 1 && "static lookahead gen <= 1");
			did_inc = true;
			eg.add_back(1, gm + 1);
		}
		
		return eg;
	}
	
	gen_map expr::default_back_map(ptr<expr> e, bool& did_inc) {
		assert(!e->back().empty() && "backtrack set never empty");
		if ( e->back().max() > 0 ) {
			assert(e->back().max() == 1 && "static lookahead gen <= 1");
			did_inc = true;
			return gen_map{0,1};
		} else {
			return gen_map{0};
		}
	}
	
	gen_map expr::update_back_map(ptr<expr> e, ptr<expr> de, gen_map eg, 
	                               gen_type gm, bool& did_inc) {
		assert(!e->back().empty() && !de->back().empty() && "backtrack set never empty");
		
		gen_map deg;
		gen_set deb = de->back();
		auto debt = deb.begin();
		auto egt = eg.begin();
		
		// Only copy mappings still in backtrack set
		while ( debt != deb.end() && egt != eg.end() ) {
			gen_type debi = *debt;
			auto egi = *egt; 
			if ( egi.first < debi ) { ++egt; continue; }  // skip mappings not in backtrack set
			assert(egi.first == debi && "no missing backtrack mappings");
			deg.add_back(debi, egi.second);  // add mappings needed for backtracking
			++debt; ++egt;
		}
		
		// Check if new generation is needed
		if ( debt != deb.end() ) {
			gen_type debm = *debt;
			assert(debm > e->back().max() && "leftover generations are new");
			assert(++debt == deb.end() && "only one leftover generation");
			deg.add_back(debm, gm+1);
			did_inc = true;
		}
		
		return deg;
	}
	
	// memo_expr ///////////////////////////////////////////////////////////////////
	
	void memo_expr::reset_memo() {
		memo_match = gen_set{};
		memo_back = gen_set{};
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
	
	// fixer ///////////////////////////////////////////////////////////////////////
	
	void fixer::operator() (ptr<expr> x) {
		if ( ! x ) return;
		if ( fixed.count(x) ) return;
		
		fix_match(x);
	}
	
	gen_set fixer::fix_match(ptr<expr> x) {
		// Fast path for expressions that can't have recursively-defined match set
		switch ( x->type() ) {
		case fail_type: case inf_type:   case eps_type: case look_type:
		case char_type: case range_type: case any_type: case str_type: 
		case not_type:
			fixed.insert(x);
			return x->match();
		default: break;
		}
		
		// Fixed point calculation for recursive expressions
		bool changed = false;
		std::unordered_set<ptr<expr>> visited;
		
		running.insert(x);
		
		// recalculate match expression until it doesn't change; 
		// by Kleene's thm this is a fixed point
		gen_set match_set{};
		while (true) {
			match_set = iter_match(x, changed, visited);
			if ( ! changed ) break;
			changed = false;
			visited.clear();
		}
		
		running.erase(x);
		fixed.insert(x);
		
		return match_set;
	}
	
	gen_set fixer::iter_match(ptr<expr> x, bool& changed, 
	                          std::unordered_set<ptr<expr>>& visited) {
		if ( fixed.count(x) ) return x->match();
		if ( ! running.count(x) ) return fix_match(x);
		if ( visited.count(x) ) return x->match(); else visited.insert(x);
		
		gen_set old_match = x->match();
		gen_set new_match = calc_match(x, changed, visited);
		if ( new_match != old_match ) {
			changed = true;
		}
		
		return new_match;
	}
	
	gen_set fixer::calc_match(ptr<expr> x, bool& changed, 
	                          std::unordered_set<ptr<expr>>& visited) {
		this->changed = &changed;
		this->visited = &visited;
		x->accept(this);
		return match;
	}
	
	void fixer::visit(fail_expr&)   { match = gen_set{}; }
	void fixer::visit(inf_expr&)    { match = gen_set{}; }
	void fixer::visit(eps_expr&)    { match = gen_set{0}; }
	void fixer::visit(look_expr& x) { match = gen_set{x.b}; }
	void fixer::visit(char_expr&)   { match = gen_set{}; }
	void fixer::visit(range_expr&)  { match = gen_set{}; }
	void fixer::visit(any_expr&)    { match = gen_set{}; }
	void fixer::visit(str_expr& x)  { match = gen_set{}; }
	
	void fixer::visit(rule_expr& x) {
		// Stop this from infinitely recursing
		if ( ! x.flags.match ) {
			x.flags.match = true;
			x.memo_match = gen_set{};
		}
		
		// Calculate and cache match set
		match = x.memo_match = iter_match(x.r, *changed, *visited);
	}
	
	void fixer::visit(not_expr& x)  {
		// Make sure subexpressions are fixed
		iter_match(x.e, *changed, *visited);
		// Return static match value
		match = gen_set{1};
	}
	
	void fixer::visit(map_expr& x)  {
		// Calculate and cache match set
		match = x.memo_match = x.eg(iter_match(x.e, *changed, *visited));
		x.flags.match = true;
	}
	
	void fixer::visit(alt_expr& x)  {
		// Save changed and visited references, in case first iter_match overwrites
		bool& changed = *(this->changed);
		std::unordered_set<ptr<expr>>& visited = *(this->visited);
		
		// Calculate and cache match set
		match = x.memo_match = x.ag(iter_match(x.a, changed, visited)) 
		                       | x.bg(iter_match(x.b, changed, visited));
		x.flags.match = true;
	}
	
	void fixer::visit(seq_expr& x)    {
		// Save changed and visited references, in case iter_match overwrites
		bool& changed = *(this->changed);
		std::unordered_set<ptr<expr>>& visited = *(this->visited);
		
		gen_set am = iter_match(x.a, changed, visited);
		auto at = am.begin();
		// Make sure successor is fixed also
		iter_match(x.b, changed, visited);
		
		// initialize match-fail follower if uninitialized and needed
		if ( at != am.end() && *at == 0 ) {
			// Add backtrack node
			if ( x.c->type() == fail_type ) {
				bool did_inc = false;
				x.c = x.b;
				x.cg = expr::default_back_map(x.c, did_inc);
				if ( did_inc ) x.gm = 1;
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
		
		match = x.memo_match = m;
		x.flags.match = true;
	}
	
	// fail_expr ///////////////////////////////////////////////////////////////////
	
	ptr<expr> fail_expr::make() { return expr::make_ptr<fail_expr>(); }
	
	// A failure expression can't un-fail - no strings to match with any prefix
	ptr<expr> fail_expr::d(char) const { return fail_expr::make(); }
		
	gen_set fail_expr::match() const { return gen_set{}; }
	
	gen_set fail_expr::back()  const { return gen_set{0}; }
	
	// inf_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> inf_expr::make() { return expr::make_ptr<inf_expr>(); }
	
	// An infinite loop expression never breaks, ill defined with any prefix
	ptr<expr> inf_expr::d(char) const { return inf_expr::make(); }
	
	gen_set inf_expr::match() const { return gen_set{}; }
	
	gen_set inf_expr::back()  const { return gen_set{0}; }
	
	// eps_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> eps_expr::make() { return expr::make_ptr<eps_expr>(); }
	
	// Only succeeds if string is empty
	ptr<expr> eps_expr::d(char x) const {
		return ( x == '\0' ) ? eps_expr::make() : fail_expr::make();
	}
	
	gen_set eps_expr::match() const { return gen_set{0}; }
	
	gen_set eps_expr::back()  const { return gen_set{0}; }
	
	// look_expr ///////////////////////////////////////////////////////////////////
	
	ptr<expr> look_expr::make(gen_type g) {
		return (g == 0) ? expr::make_ptr<eps_expr>() : expr::make_ptr<look_expr>(g);
	}
	
	// No prefixes to remove from language containing the empty string; all fail
	ptr<expr> look_expr::d(char x) const { return look_expr::make(b); }

	gen_set look_expr::match() const { return gen_set{b}; }
	
	gen_set look_expr::back()  const { return gen_set{b}; }
	
	// char_expr ///////////////////////////////////////////////////////////////////
	
	ptr<expr> char_expr::make(char c) { return expr::make_ptr<char_expr>(c); }
	
	// Single-character expression either consumes matching character or fails
	ptr<expr> char_expr::d(char x) const {
		return ( c == x ) ? eps_expr::make() : fail_expr::make();
	}
	
	gen_set char_expr::match() const { return gen_set{}; }
	
	gen_set char_expr::back()  const { return gen_set{0}; }
	
	// range_expr //////////////////////////////////////////////////////////////////
	
	ptr<expr> range_expr::make(char b, char e) { return expr::make_ptr<range_expr>(b, e); }
	
	// Character range expression either consumes matching character or fails
	ptr<expr> range_expr::d(char x) const {
		return ( b <= x && x <= e ) ? eps_expr::make() : fail_expr::make();
	}
	
	gen_set range_expr::match() const { return gen_set{}; }
	
	gen_set range_expr::back()  const { return gen_set{0}; }
	
	// any_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> any_expr::make() { return expr::make_ptr<any_expr>(); }
	
	// Any-character expression consumes any character
	ptr<expr> any_expr::d(char x) const {
		return ( x == '\0' ) ? fail_expr::make() : eps_expr::make();
	}
	
	gen_set any_expr::match() const { return gen_set{}; }
	
	gen_set any_expr::back()  const { return gen_set{0}; }
	
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
	
	gen_set str_expr::match() const { return gen_set{}; }
	
	gen_set str_expr::back()  const { return gen_set{0}; }
	
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
		memo_match = gen_set{};
		
		// Calculate match set
		return r->match();
	}
	
	gen_set rule_expr::back_set() const {
		// Stop this from infinitely recursing
		flags.back = true;
		memo_back = gen_set{0};
		
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
	
	gen_set not_expr::match_set() const { return gen_set{}; }
	
	gen_set not_expr::back_set() const { return gen_set{1}; }
	
	// map_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> map_expr::make(memo_expr::table& memo, ptr<expr> e, gen_type gm, gen_map eg) {
		// account for unmapped generations
		assert(!eg.empty() && "non-empty generation map");
		assert(e->back().max() <= eg.max_key() && "no unmapped generations");
		assert(eg.max() <= gm && "max is actually max");
		
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
		if ( gm == eg.max_key() ) return e;
		
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
		gen_map deg = expr::update_back_map(e, de, eg, gm, did_inc);
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
		gen_map ag = expr::default_back_map(a, did_inc);
		gen_map bg = expr::default_back_map(b, did_inc);
		return expr::make_ptr<alt_expr>(memo, a, b, ag, bg, 0 + did_inc);
	}
	
	ptr<expr> alt_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, 
	                         gen_map ag, gen_map bg, gen_type gm) {
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
			gen_map dbg = expr::update_back_map(b, db, bg, gm, did_inc);
			return map_expr::make(memo, db, gm + did_inc, dbg);
		}
		case inf_type:  return da; // an inf_expr
		default:        break; // do nothing
		}
		
		// Map in new lookahead generations for derivative
		gen_map dag = expr::update_back_map(a, da, ag, gm, did_inc);
		
		if ( ! da->match().empty() ) {
			return map_expr::make(memo, da, gm + did_inc, dag);
		}
		
		// Calculate other derivative and map in new lookahead generations
		ptr<expr> db = b->d(x);
		if ( db->type() == fail_type ) return map_expr::make(memo, da, gm + did_inc, dag);
		gen_map dbg = expr::update_back_map(b, db, bg, gm, did_inc);
				
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
		gen_map   cg;
		gen_set am = a->match();
		if ( ! am.empty() && am.min() == 0 ) {
			c = b;
			cg = expr::default_back_map(c, did_inc);
		} else {
			c = fail_expr::make();
			cg = gen_map{0};
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
			gen_map bng = expr::new_back_map(bn, gm, did_inc);
			return map_expr::make(memo, bn, gm + did_inc, bng);
		} case look_type: {
			// Lookahead follower (or lookahead follower match-fail)
			auto i = std::static_pointer_cast<look_expr>(da)->b;
			for (const look_node& bi : bs) {
				if ( bi.g < i ) continue;
				if ( bi.g > i ) return fail_expr::make();  // generation list is sorted
				
				ptr<expr> dbi = bi.e->d(x);  // found element, take derivative
				
				if ( dbi->type() == fail_type ) {  // straight path fails ...
					if ( bi.gl > 0 ) {                // ... but matched in the past
						return look_expr::make(bi.gl);   // return appropriate lookahead
					} else {                          // ... and no previous match
						return dbi;                      // return a failure expression
					}
				}
				
				gen_map dbig = expr::update_back_map(bi.e, dbi, bi.eg, gm, did_inc);
				
				// if there is no failure backtrack (or this generation is it) 
				// we don't have to track it
				gen_set dbim = dbi->match();
				if ( bi.gl == 0 || (! dbim.empty() && dbim.min() == 0) ) {
					return map_expr::make(memo, dbi, gm + did_inc, dbig);
				}
				
				// Otherwise return alt-expr of this lookahead and its failure backtrack
				return alt_expr::make(memo, dbi, look_expr::make(),
				                      dbig, gen_map{0,bi.gl}, gm + did_inc);
			}
			// end-of-string is only case where we can get a lookahead success for an unseen gen
			if ( x == '\0' ) {
				ptr<expr> bn = b->d('\0');
				gen_map bng = expr::new_back_map(bn, gm, did_inc);
				return map_expr::make(memo, bn, gm + did_inc, bng);
			}
			return fail_expr::make(); // if lookahead follower not found, fail
		} case fail_type: {
			// Return match-fail follower
			ptr<expr> dc = c->d(x);
			gen_map dcg = expr::update_back_map(c, dc, cg, gm, did_inc);
			return map_expr::make(memo, dc, gm + did_inc, dcg);
		} case inf_type: {
			// Propegate infinite loop
			return da;
		} default: {}  // do nothing
		}
		
		// Construct new match-fail follower
		ptr<expr> dc;
		gen_map dcg;
		
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
		gen_set dab = da->back();
		auto dabt = dab.begin();
		assert(dabt != dab.end() && "backtrack set non-empty");
		if ( *dabt == 0 ) { ++dabt; }  // skip backtrack generation 0
		auto bit = bs.begin();

		look_list dbs;
		while ( dabt != dab.end() && bit != bs.end() ) {
			const look_node& bi = *bit;
			if ( bi.g < *dabt ) { ++bit; continue; }  // skip generations not in backtrack list
			assert(bi.g == *dabt && "no generations missing from backtrack list");
			
			ptr<expr> dbi = bi.e->d(x);
			gen_map dbig = expr::update_back_map(bi.e, dbi, bi.eg, gm, did_inc);
			gen_type dgl = bi.gl;
			gen_set dbim = dbi->match();
			if ( ! dbim.empty() && dbim.min() == 0 ) {  // set new match-fail backtrack if needed
				dgl = gm+1;
				did_inc = true;
			}
			dbs.emplace_back(bi.g, dbig, dbi, dgl);
			
			++dabt; ++bit;
		}
		
		// Add new lookahead backtrack if needed
		if ( dabt != dab.end() ) {
			gen_type dabm = *dabt;
			assert(dabm > a->back().max() && "leftover generation greater than previous");
			assert(++dabt == dab.end() && "only one new lookahead backtrack");
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
		
		// Check for gen-zero backtrack from predecessor (successors included earlier)
		if ( a->back().min() == 0 ) {
			x |= 0;
		}
		
		// Include lookahead follower backtracks
		for (const look_node& bi : bs) {
			x |= bi.eg(bi.e->back());
			if ( bi.gl > 0 ) x |= bi.gl;
		}
		
		return x;
	}
		
} // namespace derivs

