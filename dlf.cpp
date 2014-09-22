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
		if ( state != forbidden && update < mgr.update ) { state = unknown; }
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
}
