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

//uncomment to disable asserts
//#define NDEBUG
#include <cassert>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../ast.hpp"
#include "../dlf.hpp"

#include "../utils/bag.hpp"

#include "dlf-loader.hpp"
#include "dlf-printer.hpp"

//#include "../utils/DBG_MSG.hpp"

namespace dlf {
/*	/// Gets all the cuts in an expression
	class cuts_in : public iterator {
	public:
		cuts_in(ptr<node> np) : cuts{} { if ( np ) iterator::visit(np); }
		operator cutset () { return cuts; }
		void visit(const cut_node& n) { cuts |= n.cut; iterator::visit(n); }
	private:
		cutset cuts;  ///< Cuts included in this expression
	}; // cuts_in
*/	
	/// Gets the maximum index cut in an expression (returns 1 more than the maximum cut, 0 for no cuts)
	class cut_end : public iterator {
	public:
		cut_end(ptr<node> np) : maxcut(0) { if ( np ) iterator::visit(np); }
		operator cutind () { return maxcut; }
		void visit(const cut_node& n) {
			if ( n.cut >= maxcut ) maxcut = n.cut + 1;
			iterator::visit(n);
		}
	private:
		cutind maxcut;
	}; // cut_end
	
	/// Disarms all cuts in an expression
	class disarm_all : public iterator {
	public:
		disarm_all(ptr<node> np) { if ( np ) iterator::visit(np); }
		void visit(const cut_node& n) { const_cast<cut_node&>(n).disarm(); iterator::visit(n); }
	}; // disarm_all

	/// Clones a nonterminal, repointing its out-arcs to the given node
	class clone : public visitor {
		/// Common code for visiting an arc
		arc clone_of(const arc& a) {
			// translate blockset
			cutset blocking;
			for (cut_node* cn : a.blocking) {
				blocking.insert(as_ptr<cut_node>(cuts[cn->cut]).get());
			}
			
			// either merge into out-arc or clone successor
			if ( a.succ->type() == end_type ) {
				arc r{out.succ, std::move(blocking)};
				r.block_all(out.blocking);
				return r;
			} else return arc{clone_of(a.succ), std::move(blocking)};
//			return ( a.succ->type() == end_type ) ?
//				arc{out.succ, out.blocking | (a.blocking >> nShift)} :
//			 	arc{clone_of(a.succ), a.blocking >> nShift};
		}

		/// Functional interface for visitor pattern
		ptr<node> clone_of(const ptr<node> np) {
			auto it = visited.find(np);
			if ( it != visited.end() ) return rVal = it->second;
			np->accept(this);
			visited.emplace(np, rVal);
			return rVal;
		}
	public:
		clone(nonterminal& nt, const arc& out, cutind nShift = 0)
			: rVal(out.succ), out(out), nShift(nShift), visited(), cuts(nShift) {
			// initialize cut nodes
			for (cutind i = 0; i < nShift; ++i) {
				cuts[i] = node::make<cut_node>(arc{fail_node::make()}, i + nShift);
			}
			// perform clone
			rVal = clone_of(nt.sub);
		}
//		clone(nonterminal& nt, const arc& out,
//		      cut_listener* listener = nullptr, cutind nShift = 0)
//		: rVal{out.succ}, out{out}, listener{listener}, nShift{nShift}, visited{} {
//			rVal = clone_of(nt.sub);
//		}
		operator ptr<node> () { return rVal; }

		void visit(const match_node&)   { rVal = match_node::make(); }
		void visit(const fail_node&)    { rVal = fail_node::make(); }
		void visit(const inf_node&)     { rVal = inf_node::make(); }
		void visit(const end_node&)     { rVal = out.succ; }
		void visit(const char_node& n)  { rVal = node::make<char_node>(clone_of(n.out), n.c); }
		void visit(const range_node& n) { rVal = node::make<range_node>(clone_of(n.out), n.b, n.e); }
		void visit(const any_node& n)   { rVal = node::make<any_node>(clone_of(n.out)); }
		void visit(const str_node& n)   { rVal = node::make<str_node>(clone_of(n.out), n.sp, n.i); }
		void visit(const rule_node& n)  { rVal = node::make<rule_node>(clone_of(n.out), n.r); }
		void visit(const cut_node& n)   {
			// Rewrite appropriate cut
			cut_node& cn = *as_ptr<cut_node>(cuts[n.cut]);
			cn.out = clone_of(n.out);
			rVal = cuts[n.cut];
//			rVal = node::make<cut_node>(clone_of(n.out), n.cut + nShift, listener);
		}
		void visit(const alt_node& n)   {
			arc_set rs;
			for (const arc& a : n.out) { rs.emplace(clone_of(a)); }
			rVal = node::make<alt_node>(std::move(rs));
		}

	private:
		ptr<node> rVal;          ///< Return value of last visit
		const arc out;           ///< Replacement for end nodes
//		cut_listener* listener;  ///< Cut listener to release cloned cuts to
		cutind nShift;           ///< Amount to shift restrictions by

		/// Memoizes visited nodes to maintain the structure of the DAG
		std::unordered_map<ptr<node>, ptr<node>> visited;
		/// List of cloned cut nodes (need to be built before outgoing arcs)
		std::vector<ptr<node>> cuts;
	};  // clone

	/// Implements the derivative computation over a DLF DAG
	class derivative {
//	class derivative : public cut_listener {
/*		/// Pending cut application, blocked according to some outstanding indices
		struct cut_info {
//			cut_info() : blocking{}, freed{false} {}
			cut_info(arc&& a) : a(std::move(a)), freed(false) {}
//			cut_info(const cutset& blocking) : blocking{blocking}, freed{false} {}

			arc a;       ///< Arc with restrictions blocking this cut; points to a cut node
//			cutset blocking;  ///< Restrictions that can block the cut
			bool freed;  ///< Have all references to this cut been freed?
		};  // cut_info
*/		
		/// Information about a nonterminal
		struct nt_info {
			nt_info(ptr<nonterminal> nt) : nCuts(cut_end(nt->sub)), inDeriv(false) {}
//			nt_info(const cutset& cuts)
//			: cuts{cuts}, nCuts{cuts.empty() ? 0 : cuts.last() + 1}, inDeriv{false} {}

//			cutset cuts;   ///< Cuts used by non-terminal
			cutind nCuts;  ///< Number of cuts used by non-terminal
			bool inDeriv;  ///< Flag used when currently in derivative
		}; // nt_info

		/// Gets the info block for a nonterminal
		nt_info& get_info(ptr<nonterminal> nt) {
			auto it = nt_state.find(nt);
			if ( it == nt_state.end() ) {
				nt_info info(nt);
//				nt_info info(cuts_in(nt->sub));
				return nt_state.emplace_hint(it, nt, std::move(info))->second;
			} else return it->second;
		}

		/// Expands a nonterminal
		ptr<node> expand(ptr<nonterminal> nt, const arc& out, const nt_info& info) {
			cutind nShift = next_restrict;
			next_restrict += info.nCuts;
			return clone(*nt, out, nShift);
//			return clone(*nt, out, this, nShift);
		}

		inline ptr<node> expand(ptr<nonterminal> nt, const arc& out) {
			return expand(nt, out, get_info(nt));
		}

/*		/// Mark all cut nodes left in the DAG as unfired for end-of-input
		void release_unfired() {
			new_released = cuts_in(root.succ);
//DBG("marking ["; for(auto ii : new_released) std::cout << " " << ii; std::cout << " ] released by unfired" << std::endl);
			if ( ! new_released.empty() ) release_new();
		}
*/
/*		/// Called when a set of cuts is applied
		void block_new() {
//DBG("marking ["; for (auto ii : new_blocked) std::cout << " " << ii; std::cout << " ] blocked" << std::endl);
			// apply new blocks to root node
			if ( root.blocking.intersects(new_blocked) ) {
				root.succ = fail_node::make();
			} else {
				// check if root node is alt_node, if so check its successors
				if ( root.succ->type() == alt_type ) {
					alt_node& an = *as_ptr<alt_node>(root.succ);
					auto it = an.out.begin();
					while ( it != an.out.end() ) {
						if ( it->blocking.intersects(new_blocked) ) {
							it = an.out.erase(it);
							continue;
						}
						
						++it;
					}
					if ( an.out.empty() ) { root.succ = fail_node::make(); }
					else if ( an.out.size() == 1 ) {
						const arc& aa = *an.out.begin();
						root.blocking |= aa.blocking;
						root.succ = aa.succ;
					}
				}
			}

			// apply new blocks to pending list
			auto it = pending.begin();
			while ( it != pending.end() ) {
				cutind cut = it->first;
				cut_info& info = it->second;

				// remove newly blocked cuts from pending list
				if ( new_blocked(cut) ) {
					it = pending.erase(it);
					continue;
				}

				// release any pending cuts that are now blocked
				if ( info.blocking.intersects(new_blocked) ) {
					if ( info.freed && pending.count(cut) == 1 ) {
						new_released |= cut;
//DBG("releasing " << cut << " by new blocked" << std::endl);
					}
//else DBG("removing pending " << cut << " by new blocked" << std::endl);
					it = pending.erase(it);
					continue;
				}
				
				++it;
			}

			// apply new blocks to pending match list
			auto jt = pending_matches.begin();
			while ( jt != pending_matches.end() ) {
				if ( jt->second.intersects(new_blocked) ) {
//DBG("removing match at " << jt->first << std::endl);
					jt = pending_matches.erase(jt);
					continue;
				}
				++jt;
			}

			// clear the new_blocked list
			blocked |= new_blocked;
			new_blocked.clear();

			// recurse with newly released nodes
			if ( ! new_released.empty() ) { release_new(); }
		}
*/
/*		/// Called when a set of cuts will never match
		void release_new() {
//DBG("marking ["; for (auto ii : new_released) std::cout << " " << ii; std::cout << " ] released" << std::endl);
			// apply new releases to root node
			root.blocking -= new_released;
			// Also apply to arc successors if alt_node
			if ( root.succ->type() == alt_type ) {
				alt_node& an = *as_ptr<alt_node>(root.succ);
				for (const arc& a : an.out) {
					const_cast<cutset&>(a.blocking) -= new_released;
				}
			}

			// apply new releases to pending list
			auto it = pending.begin();
			while ( it != pending.end() ) {
				cutind cut = it->first;
				cut_info& info = it->second;

				// block any cuts that have been fired pending some newly released blocks
				info.blocking -= new_released;
				if ( info.blocking.empty() ) {
					new_blocked |= cut;
//DBG("firing " << cut << " by new released" << std::endl);
					it = pending.erase(it);
					continue;
				}

				++it;
			}

			// apply new releases to pending match list
			auto jt = pending_matches.begin();
			while ( jt != pending_matches.end() ) {
				jt->second -= new_released;
				++jt;
			}

			// clear the new_released list
			released |= new_released;
			new_released.clear();

			// recurse with newly blocked nodes
			if ( ! new_blocked.empty() ) { block_new(); }
		}
*/
		/// Functionalal interface to actually take the derivative
		class deriv : public visitor {
			static inline arc merge(const arc& a, const cutset& blocking) {
				arc r{a};
				r.block_all(blocking);
				return r;
//				return arc{a.succ, a.blocking | blocking};
			}

			static arc traverse(arc&& a, derivative& st) {
//				// Clear released cuts
//				a.blocking -= st.released;

				// Fail on blocked arc
				if ( a.blocked() ) return a;
//				if ( a.blocking.intersects(st.blocked) ) {
//					a.succ = fail_node::make();
//					return a;
//				}

				switch ( a.succ->type() ) {
				// Conditionally apply cut node
				case cut_type: { 
					cut_node& cn = *as_ptr<cut_node>(a.succ);
					
					if ( a.blocking.empty() ) {
						cn.fire();
						a = cn.out;
						return traverse(std::move(a), st);
//						st.new_blocked |= cn.cut;
////DBG("new " << cn.cut << " blocked by traversal" << std::endl);
					} else {
						// save copy of self on pending list, traverse successor
						arc c{cut_node::make(arc{}, cn.cut, cn.blocked), a.blocking};
						st.pending.insert(std::move(c));
						a = merge(cn.out, a.blocking);
						return traverse(std::move(a), st);
//						st.pending.emplace(cn.cut, a.blocking);
////DBG("new " << cn.cut << " blocked pending ["; for (auto ii : a.blocking) std::cout << " " << ii; std::cout << " ] by traversal" << std::endl);
					}
//					
//					// Merge block-set into successor and traverse
//					a = merge(cn.out, a.blocking);
//					return traverse(std::move(a), st);
				}
				// Conditionally apply match node
				case match_type: {
					st.pending_matches.emplace(st.input_index, a);
					return arc{};
//					st.pending_matches.emplace_back(st.input_index, a.blocking);
////DBG("new match at " << st.input_index << " blocked pending["; for (auto ii : a.blocking) std::cout << " " << ii; std::cout << " ]" << std::endl);
//					return arc{fail_node::make()};
				}
				// Traverse all branches of an alternation
				case alt_type: {
					const alt_node& an = *as_ptr<alt_node>(a.succ);
					arc_set as;
					for (const arc& aa : an.out) {
						as.emplace(traverse(merge(aa, a.blocking), st));
					}
					switch ( as.size() ) {
					case 0:  return arc{};
//					case 0:  return arc{fail_node::make()};
					case 1:  a = *as.begin(); return a;
//					case 1:  return merge(*as.begin(), a.blocking);
					default: a.succ = alt_node::make(std::move(as)); return a;
					}
				}
				// Return non-mutator node
				default: return a;
				}
			}
			inline arc traverse(arc&& a) { return traverse(std::move(a), st); }

			/// Sets a failure arc into rVal
			inline void fail() { rVal.block(); }
//			inline void fail() { rVal.succ = fail_node::make(); }

			/// Merges the given arc into rVal
			inline void follow(const arc& a) { rVal = traverse(merge(a, rVal.blocking)); }

			/// Resets rVal to the given node
			inline void reset(ptr<node> np) { rVal.succ = np; }

			/// Does nothing to rVal
			inline void pass() {}

			/// Takes the derivative of another arc
			inline arc d(arc&& a) { return deriv(std::move(a), x, st); }
			inline arc d(const arc& a) { return d(arc{a}); }
		
		public:	
			deriv(arc&& a, char x, derivative& st)
			: st(st), rVal(traverse(std::move(a), st)), x(x) {
//PRE_DBG_ARC("take deriv of ", rVal);
				rVal.succ->accept(this);
			}
			operator arc() { return rVal; }
//operator arc() { POST_DBG_ARC("deriv result is ", rVal); return rVal; }
			
			void visit(const match_node&)   {
				assert(false && "Should never take derivative of match node");
			}
			void visit(const fail_node&)    { pass(); }
			void visit(const inf_node&)     { pass(); }
			void visit(const end_node&)     {
				assert(false && "Should never take derivative of end node");
			}

			void visit(const char_node& n)  { x == n.c ? follow(n.out) : fail(); }

			void visit(const range_node& n) { n.b <= x && x <= n.e ? follow(n.out) : fail(); }

			void visit(const any_node& n)   { x != '\0' ? follow(n.out) : fail(); }

			void visit(const str_node& n) {
				if ( x == (*n.sp)[n.i] ) {
					n.size() == 1 ?
						follow(n.out) :
						reset(node::make<str_node>(arc{n.out}, n.sp, n.i+1));
				} else fail();
			}

			void visit(const rule_node& n) {
				nt_info& info = st.get_info(n.r);
				// return infinite loop on left-recursion
				if ( info.inDeriv ) {
					reset(inf_node::make());
					return;
				}

				// take derivative of expanded rule under left-recursion flag
				info.inDeriv = true;
				reset(st.expand(n.r, n.out, info));
				rVal = d(std::move(rVal));
				info.inDeriv = false;
			}

			void visit(const cut_node& n) {
				assert(false && "should never take derivative of cut node");
			}

			void visit(const alt_node& n) {
				arc_set as;
				for (arc o : n.out) {
					o.block_all(rVal.blocking);
//					o.blocking |= rVal.blocking;
					as.emplace(d(std::move(o)));
				}

				switch ( as.size() ) {
				case 0: fail(); return;  // no following node
				case 1: rVal = *as.begin(); return;  // replace with single node
				default: reset(node::make<alt_node>(std::move(as))); return; // replace alt
				}
			}

		private:
			derivative& st;  ///< Derivative state
			arc rVal;        ///< Return value
			char x;          ///< Character to take derivative with respect to
		};  // deriv

		inline arc d(arc&& a, char x) { return deriv(std::move(a), x, *this); }
		inline arc d(const arc& a, char x) { return d(arc{a}, x); }

	public:
		/// Sets up derivative computation for a nonterminal
		derivative(ptr<nonterminal> nt)
			: nt_state(), pending(), input_index(0), pending_matches(),
			  next_restrict(0), match_ptr(), root(ptr<node>{}) {
//		: nt_state{}, blocked{}, released{}, pending{}, new_blocked{}, new_released{},
//		  input_index{0}, pending_matches{}, 
//		  next_restrict{0}, match_ptr{}, root{ptr<node>{}} {
			ptr<node> mp = match_node::make();
			match_ptr = mp;
			root.succ = expand(nt, arc{mp});
		}

//		~derivative() {
//			root.succ.reset();  // delete all the arcs before destroying their cut listener
//		}

		/// Checks if the current expression is an unrestricted match
		bool matched() const {
			for (const auto& p : pending_matches) {
				if ( p.second.blocking.empty() ) return true;
//				if ( p.second.empty() ) return true;
			}
			return false;
		}

		/// Checks if the current expression cannot match
		bool failed() const { return pending_matches.empty() && match_ptr.expired(); }

/*		/// Called when new cuts are added
		void acquire_cut(cutind cut) {}

		/// Called when cuts can no longer be applied
		void release_cut(cutind cut) {
			// can't release cut that's already blocked
			
			auto rg = pending.equal_range(cut);
			if ( rg.first == rg.second ) {
				// release cut that is freed and not pending or blocked
				if ( new_blocked(cut) || blocked(cut) ) return;
//if ( new_blocked(cut) || blocked(cut) ) { DBG("blocked " << cut << " freed" << std::endl); return; }
				new_released |= cut;
//DBG("new " << cut << " released due to free" << std::endl);
			} else do {
				// mark all pending instances of this cut freed
				// can't be new instances, because the cut won't occur again
				rg.first->second.freed = true;
//DBG("pending " << cut << " freed" << std::endl);
				++rg.first;
			} while ( rg.first != rg.second );
		}
*/
		/// Takes the derviative of the current root node
		void operator() (char x) {
			root = d(std::move(root), x);  // take derivative
			
			// traverse the graph disarming all remaining cuts on end-of-input
			if ( x == '\0' ) { disarm_all(root.succ); }
			
			// check cuts on pending list for blocked or ready to fire; repeat until fixed point
			bool fired_cut;
			do {
				fired_cut = false;
				auto it = pending.begin();
				while ( it != pending.end() ) {
					arc& a = *it;
					if ( a.blocked() ) {  // erase blocked cut from pending list
						it = pending.erase(it);
					} else if ( a.blocking.empty() ) {  // fire unblocked cut
						as_ptr<cut_node>(a.succ)->fire();
						fired_cut = true;
						it = pending.erase(it);
					} else {  // skip other cuts
						++it;
					}
				}
			} while ( fired_cut );
			
			// remove blocked matches from match pending list
			auto jt = pending_matches.begin();
			while ( jt != pending_matches.end() ) {
				if ( jt->second.blocked() ) { jt = pending_matches.erase(jt); } 
				else { ++jt; }
			}
			
			++input_index;
////if ( !( new_blocked.empty() && new_released.empty() && x != '\0' ) ) { PRE_DBG("applying block-set changes" << std::endl); 
//			if ( ! new_blocked.empty() ) { block_new(); }
//			else if ( ! new_released.empty() ) { release_new(); }
//			if ( x == '\0' ) { release_unfired(); }
//			++input_index;
////DBG("new blocked:["; for (auto ii : blocked) std::cout << " " << ii; std::cout << " ]" << std::endl);
////DBG("new released:["; for (auto ii : released) std::cout << " " << ii; std::cout << " ]" << std::endl);
////POST_DBG_ARC("changes applied; now ", root); }
		}

	private:
		/// Stored state for each nonterminal
		std::unordered_map<ptr<nonterminal>, nt_info> nt_state;
	public:
//		cutset blocked;                 ///< Blocked indices
//		cutset released;                ///< Safe indices
		/// Information about outstanding cut indices
		bag<arc> pending;
//		std::unordered_multimap<cutind, cut_info> pending;
//	private:
//		cutset new_blocked;             ///< Indices blocked this step
//		cutset new_released;            ///< Indices released this step
//	public:	
		unsigned long input_index;      ///< Current index in the input
		/// Information about possible matches
		bag<std::pair<unsigned long, arc>> pending_matches;
//		std::vector<std::pair<unsigned long, cutset>> pending_matches;
	private:
		cutind next_restrict;           ///< Index of next available restriction
		std::weak_ptr<node> match_ptr;  ///< Pointer to match node
	public:
		arc root;                       ///< Current root arc
	};  // derivative

	/// Levels of debug output
	enum dbg_level { no_dbg, cat_dbg, full_dbg };

	/// Recognizes the input
	/// @param l		Loaded DLF DAG
	/// @param in		Input stream
	/// @param rule		Start rule
	/// @param dbg		Print debug output? (default false)
	/// @return true for match, false for failure
	bool match(loader& l, std::istream& in, std::string rule, dbg_level dbg = no_dbg) {
		// find rule
		auto& nts = l.get_nonterminals();
		auto nt = nts.find(rule);

		// fail on no such rule
		if ( nt == nts.end() ) return false;

		// set up printer
		std::unordered_set<ptr<nonterminal>> names;
		for (auto& nit : nts) { names.emplace(nit.second); }
		dlf::printer p(std::cout, names);

		// set up derivative
		derivative d{nt->second};

		// take derivatives until failure, match, or end of input
		char x = '\x7f';  // DEL character; never read
		do {
			if ( dbg == full_dbg ) {
				// print pending cuts
				for (auto c : d.pending) {
					std::cout << as_ptr<cut_node>(c.succ)->cut << ":[";
//					std::cout << c.first << ":[";
					if ( ! c.blocking.empty() ) {
//					if ( ! c.second.blocking.empty() ) {
						auto ii = c.blocking.begin();
//						auto ii = c.second.blocking.begin();
						std::cout << (*ii)->cut;
//						std::cout << *ii;
						while ( ++ii != c.blocking.end() ) {
//						while ( ++ii != c.second.blocking.end() ) {
							std::cout << " " << (*ii)->cut;
//							std::cout << " " << *ii;
						}
					}
					std::cout << "] ";
				}
				// print pending matches
				for (auto m : d.pending_matches) {
					std::cout << "@" << m.first << ":[";
					if ( ! m.second.blocking.empty() ) {
//					if ( ! m.second.empty() ) {
						auto ii = m.second.blocking.begin();
//						auto ii = m.second.begin();
						std::cout << (*ii)->cut;
//						std::cout << *ii;
						while ( ++ii != m.second.blocking.end() ) {
//						while ( ++ii != m.second.end() ) {
							std::cout << (*ii)->cut;
//							std::cout << " " << *ii;
						}
					}
					std::cout << "] ";
				}
				// print root node
				p.print(d.root);
			}

			if ( d.failed() ) return false;
			else if ( d.matched() ) return true;
			else if ( x == '\0' ) return false;

			if ( ! in.get(x) ) { x = '\0'; }  // read character, \0 for EOF

			switch ( dbg ) {
			case full_dbg:
				std::cout << "d(\'" << (x == '\0' ? "\\0" : strings::escape(x)) << "\') =====>"
				          << std::endl;
				break;
			case cat_dbg:
				if ( x != '\0' ) std::cout << x;
				break;
			case no_dbg: break;
			}

			// take derivative
			d(x);
		} while ( true );
	}

	/// Recognizes the input
	/// @param g		Source grammar
	/// @param in		Input stream
	/// @param rule		Start rule
	/// @param dbg		Print debug output? (default false)
	/// @return true for match, false for failure
	bool match(ast::grammar& g, std::istream& in, std::string rule, dbg_level dbg = no_dbg) {
		loader l(g, dbg == full_dbg);
		return match(l, in, rule, dbg);
	}

}  // namespace dlf
