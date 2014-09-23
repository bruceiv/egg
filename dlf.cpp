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

#include <algorithm>

#include "dlf.hpp"

namespace dlf {
	// restriction_mgr /////////////////////////////////////////////////////////
	
	bool restriction_mgr::check_enforced() {
		bool new_enforced = false;
		it = pending.begin();
		while ( it != pending.end() ) {
			flags::index j = it->first;
			blocker& j_blocker = it->second;
			
			j_blocker.blocking -= unenforceable;
			if ( j_blocker.blocking.empty() ) {
				// no remaining restrictions on enforcing j
				auto jt = it++;
				enforced |= j;
				pending.erase(jt);
				new_enforced = true;
				continue;
			}
			
			++it;
		}
		return new_enforced;
	}
	
	bool restriction_mgr::check_unenforceable() {
		bool new_unenforceable = false;
		it = pending.begin();
		while ( it != pending.end() ) {
			flags::index j = it->first;
			blocker& j_blocker = it->second;
	
			if ( j_blocker.released && j_blocker.intersects(enforced) ) {
				// this newly enforced restrictions mean that no j-restriction will ever fire
				auto jt = it++;
				unenforceable |= j;
				pending.erase(jt);
				new_unenforceable = true;
				continue;
			}
	
			++it;
		}
		return new_unenforceable;
	}
	
	void restriction_mgr::reserve(flags::index n) {
		flags::index t = next;
		next += n;
		return t;
	}
	
	void restriction_mgr::enforce_unless(flags::index i, const flags::vector& blocking) {
		// check if the restriction has already been enforced
		if ( forbidden(i) ) return;
		
		// check if there are already restrictions blocking enforcement
		auto it = pending.find(i);
		if ( it == pending.end() ) {
			// new restriction
			if ( ! blocking.empty() ) {
				// wait on enforcement decision if blocking restrictions
				pending.emplace_hint(it, blocking);
				return;
			}
		} else {
			blocker& i_blocker = it->second;
			// refine existing restriction
			i_blocker.blocking &= blocking;
			// continue to wait on enforcement decision if blocking restrictions remain
			if ( ! i_blocker.blocking.empty() ) return;
			// otherwise remove restriction to enforce from wait list
			pending.erase(it);
		}
		
		// if we reach here then we are enforcing the restriction, and it is not a member 
		// of the pending set
		enforced |= i;
		
		// Check if this blocks any other restrictions from being enforced
		bool new_unenforceable = false;
		it = pending.begin();
		while ( it != pending.end() ) {
			flags::index j = it->first;
			blocker& j_blocker = it->second;
			
			if ( j_blocker.released && j_blocker.blocking(i) ) {
				// this newly enforced restriction means that no j-restriction will ever fire
				auto jt = it++;
				unenforceable |= j;
				pending.erase(jt);
				new_unenforceable = true;
				continue;
			}
			
			++it;
		}
		
		while ( new_unenforceable ) {
			// Check if the newly-unenforceable restrictions fire any others
			bool new_enforced = check_enforced();
					
			// Check if any newly-enforced restrictions block any others
			new_unenforceable = new_enforced ? check_unenforceable() : false;
		}
		
		++update;
	}
	
	void restriction_mgr::release(flags::index i) {
		// check if the restriction has already been enforced
		if ( forbidden(i) ) return;
		
		// check if there is a pending enforcement decision for the restriction
		auto it = pending.find(i);
		if ( it != pending.end() ) {
			blocker& i_blocker = it->second;
			i_blocker.released = true;
			// continue to wait on this decision if releasing the restriction doesn't block it
			if ( ! i_blockier.blocking.intersects(enforced) ) return;
			// otherwise remove restriction from pending list
			pending.erase(it);
		}
		
		// if we reach here then the restriction is unenforceable
		unenforceable |= i;
		
		// Check if the newly-unenforceable restriction fires any others
		bool new_enforced = false;
		it = pending.begin();
		while ( it != pending.end() ) {
			flags::index j = it->first;
			blocker& j_blocker = it->second;
			
			j_blocker.blocking -= i;
			if ( j_blocker.blocking.empty() ) {
				// no remaining restrictions on enforcing j
				auto jt = it++;
				enforced |= j;
				pending.erase(jt);
				new_enforced = true;
				continue;
			}
			
			++it;
		}
		
		while ( new_enforced ) {
			// Check if any newly-enforced restrictions block any others
			bool new_unenforceable = check_unenforceable();
			
			// Check if the newly-unenforceable restrictions fire any others
			new_enforced = new_unenforceable ? check_enforced() : false;
		}
		
		++update;
	}
	
	// restriction_ck //////////////////////////////////////////////////////////
	
	restriction restriction_ck::check() {
		// Shortcuts for known state or no change
		if ( state != unknown || update == mgr.update ) return state;
		
		// Remove restrictions that have been lifted
		restricted -= mgr.unenforceable;
		// Check for lack of restrictions
		if ( restricted.empty() ) { state = allowed; }
		// Check for newly enforced restrictions
		else if ( restricted.intersects(mgr.enforced) ) { state = forbidden; }
		
		update = mgr.update;
		return state;
	}
	
	void restriction_ck::join(const restriction_ck& o) {
		// add restriction set
		restricted |= o.restricted;
		
		// update knowledge state of restriction set
		update = min(update, o.update);
		if ( state == allowed && (o.state != allowed || update < mgr.update) ) { state = unknown; }
	}
	
	// node_type ///////////////////////////////////////////////////////////////
	
	std::ostream& operator<< (std::ostream& out, node_type t) {
		switch ( t ) {
		case match_type: out << "MATCH"; break;
		case fail_type:  out << "FAIL";  break;
		case inf_type:   out << "INF";   break;
		case char_type:  out << "CHAR";  break;
		case range_type: out << "RANGE"; break;
		case any_type:   out << "ANY";   break;
		case str_type:   out << "STR";   break;
		case rule_type:  out << "RULE";  break;
		case alt_type:   out << "ALT";   break;
		case cut_type:   out << "CUT";   break;
		case end_type:   out << "END";   break;
		}
		return out;
	}
	
	// arc /////////////////////////////////////////////////////////////////////
	
	bool arc::blocked() {
		if ( blocking.check() == forbidden ) { fail(); return true }
		return false;
	}
	
	bool arc::join(arc& out) {
		succ = out.succ;
		blocking.join(out.blocking);
		return succ->type() == match_type && blocking.check() == allowed;
	}
	
	bool arc::fail() {
		succ = fail_node::make();
		return false;
	}
	
	// match_node //////////////////////////////////////////////////////////////
	
	ptr<node> match_node::make(bool& reachable) { return node::make<match_node>(reachable); }
	
	bool match_node::d(char, arc& in) {
		switch ( in.blocking.check() ) {
		case allowed:   return true;
		case forbidden: in.fail(); return false;
		case unknown:   return false;
		}
	}
	
	std::size_t match_node::hash() const { return tag_with(match_type); }
	
	bool match_node::equiv(ptr<node> o) const { return o->type() == match_type; }
	
	// fail_node ///////////////////////////////////////////////////////////////
	
	ptr<node> fail_node::make() {
		static ptr<node> singleton = as_ptr<node>(ptr<fail_node>{new fail_node});
		return singleton;
	}
	
	bool fail_node::d(char, arc&) { return false; }
	
	std::size_t fail_node::hash() const { return tag_with(fail_type); }
	
	bool fail_node::equiv(ptr<node> o) const { return o->type() == fail_type; }
	
	// inf_node ////////////////////////////////////////////////////////////////
	
	ptr<node> inf_node::make() {
		static ptr<node> singleton = as_ptr<node>(ptr<inf_node>{new inf_node});
		return singleton;
	}
	
	bool inf_node::d(char, arc& in) {
		if ( in.blocking.check() == forbidden ) in.fail();
		return false;
	}
	
	std::size_t fail_node::hash() const { return tag_with(inf_type); }
	
	bool inf_node::equiv(ptr<node> o) const { return o->type() == inf_type; }
	
	// end_node ///////////////////////////////////////////////////////////////
	
	ptr<node> end_node::make() { return node::make<end_node>(); }
	
	bool end_node::d(char, arc&) { return false; }
	
	std::size_t end_node::hash() const { return tag_with(end_type); }
	
	bool end_node::equiv(ptr<node> o) const { return o->type() == end_type; }
	
	// char_node //////////////////////////////////////////////////////////////
	
	ptr<node> char_node::make(const arc& out, char c) { return node::make<char_node>(out, c); }
	
	bool char_node::d(char x, arc& in) {
		if ( in.blocked() ) return false;
		return x == c ? in.join(out) : in.fail();
	}
	
	std::size_t char_node::hash() const { return tag_with(char_type, c); }
	
	bool char_node::equiv(ptr<node> o) const {
		return o->type() == char_type && as_ptr<char_node>(o)->c == c;
	}
	
	// range_node /////////////////////////////////////////////////////////////
	
	ptr<node> range_node::make(const arc& out, char b, char e) {
		if ( b > e ) return fail_node::make();
		else if ( b == e ) return char_node::make(out, b);
		else return node::make<range_node>(out, b, e);
	}
	
	bool range_node::d(char x, arc& in) {
		if ( in.blocked() ) return false;
		return b <= x && x <= e ? in.join(out) : in.fail();
	}
	
	std::size_t range_node::hash() const { return tag_with(range_type, (b << 8) | e); }
	
	bool range_node::equiv(ptr<node> o) const {
		if ( o->type() != end_type ) return false;
		ptr<range_node> oc = as_ptr<range_node>(o);
		return oc->b == b && oc->e == e;
	}
	
	// any_node ///////////////////////////////////////////////////////////////
	
	ptr<node> any_node::make(const arc& out) { return node::make<any_node>(out); }
	
	bool any_node::d(char, arc& in) { return in.blocked() ? false : in.join(out); }
	
	std::size_t any_node::hash() const { return tag_with(any_type); }
	
	bool char_node::equiv(ptr<node> o) const { return o->type() == any_type }
	
	// str_node ///////////////////////////////////////////////////////////////
	
	ptr<node> str_node::make(const arc& out, const std::string& s) {
		switch ( s.size() ) {
		case 0:  return out.succ;
		case 1:  return char_node::make(out, s[0]);
		default: return node::make<str_node>(out, s);
		}
	}
	
	bool str_node::d(char x, arc& in) {
		if ( in.blocked() )      return false;                // fail on node blocked
		if ( (*sp)[i] != x )     return in.fail();            // fail on non-match
		if ( sp->size() == i+1 ) return in.join(out);         // join with successor if consumed
		in.succ = as_ptr<node>(ptr<str_node>{out, sp, i+1});  // new node with incremented index
		return false;
	}
	
	std::size_t str_node::hash() const { return tag_with(str_type, sp.get() ^ i); }
	
	bool str_node::equiv(ptr<node> o) const {
		if ( o->type() != str_type ) return false;
		ptr<str_node> oc = as_ptr<str_node>(o);
		return oc->sp == sp && oc->i == i;
	}
	
	// rule_node //////////////////////////////////////////////////////////////
	
	// alt_node ///////////////////////////////////////////////////////////////
	
	std::pair<arc_set::iterator, bool> alt_node::merge(arc& add) {
		node_type ty = add.succ->type();
		
		// don't bother inserting failing nodes
		if ( ty == fail_type ) return std::make_pair(out.end(), false);
		
		auto ins = out.emplace(add);
		if ( ! ins->second ) { // element was not inserted
			arc& ex = *(ins->first);  // existing arc
			
			switch ( ty ) {
			case match_type: {
				/// Match can only be prevented by restrictions that block both matches
				ex.blocking.refine(add.blocking);
				if ( ex.blocking.check() == allowed ) return std::make_pair(ins, true);
			} case inf_type: {
			
			} case end_type: {
			
			} case char_type: {
			
			} case range_type: {
			
			} case any_type: {
			
			} case str_type: {
			
			} case rule_type: {
			
			} case alt_type: {
			
			} case cut_type: {
			
			} case fail_type: { assert(false && "shouldn't reach this branch"); } 
			}
		}
		
		return std::make_pair(ins, false);
	}
	
	template<typename It>
	ptr<node> alt_node::make(It first, It last) {
		if ( first == last ) return fail_node::make();
		// TODO account for empty/singleton alternations
		// TODO also, lets make all the constructors private, and have make() be the only way to build nodes
		return node::make<alt_node>(first, last);
	}
	
	bool alt_node::d(char x, arc& in) {
		if ( in.blocked() ) return false;
		
		alt_node n;
		bool is_match = false;
		for (arc& o : out) {
			// Take derivative and merge in (short-circuiting for unrestricted match)
			o.succ->d(x, o);
			if ( n.merge(o) ) return in.join(*n.out.begin());
		}
		
		switch ( n.out.size() ) {
		case 0:  return in.fail();
		case 1:  return in.join(*n.out.begin());
		default: in.succ = node::make<alt_node>(std::move(n)); return false;
		}
	}
	
	std::size_t alt_node::hash() const { return tag_with(alt_type); }
	
	bool alt_node::equiv(ptr<node> o) const { return o->type() == alt_type; }
	
	// cut_node ///////////////////////////////////////////////////////////////
	
	ptr<node> cut_node::make(const arc& out, flags::index i, restriction_mgr& mgr) {
		return node::make<cut_node>(out, i, mgr);
	}
	
	bool cut_node::d(char x, arc& in) {
		// Check blocked
		if ( in.blocked() ) return false;
		// Enforce restriction
		mgr.enforce_unless(i, in.blocking);
		// Pass derivative along
		in.join(out);
		return in.succ.d(x, in);
	}
	
	std::size_t cut_node::hash() const { return tag_with(cut_type, i); }
	
	bool cut_node::equiv(ptr<node> o) const {
		if ( o->type() != cut_type ) return false;
		return as_ptr<cut_node>(o)->i == i;
	}
	
}
