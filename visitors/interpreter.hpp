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

#include "../utils/flagvector.hpp"

#include "dlf-loader.hpp"
#include "dlf-printer.hpp"

namespace dlf {

	/// Gets all the cuts in an expression
	class cuts_in : public iterator {
	public:
		cuts_in(ptr<node> np) : cuts{} { if ( np ) iterator::visit(np); }
		operator flags::vector () { return cuts; }
		void visit(const cut_node& n) { cuts |= n.cut; iterator::visit(n); }
	private:
		flags::vector cuts;  ///< Cuts included in this expression
	};  // restrictions_of

	/// Clones a nonterminal, repointing its out-arcs to the given node
	class clone : public visitor {
		/// Common code for visiting an arc
		arc clone_of(const arc& a) {
			return ( a.succ->type() == end_type ) ?
				arc{out.succ, out.blocking | (a.blocking << nShift)} :
			 	arc{clone_of(a.succ), a.blocking << nShift};
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
		clone(nonterminal& nt, const arc& out,
		      cut_listener* listener = nullptr, flags::index nShift = 0)
		: rVal{out.succ}, out{out}, listener{listener}, nShift{nShift}, visited{} {
			rVal = clone_of(nt.sub);
		}
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
			rVal = node::make<cut_node>(clone_of(n.out), n.cut + nShift, listener);
		}
		void visit(const alt_node& n)   {
			arc_set rs;
			for (const arc& a : n.out) { rs.emplace(clone_of(a)); }
			rVal = node::make<alt_node>(std::move(rs));
		}

	private:
		ptr<node> rVal;          ///< Return value of last visit
		arc out;                 ///< Replacement for end nodes
		cut_listener* listener;  ///< Cut listener to release cloned cuts to
		flags::index nShift;     ///< Amount to shift restrictions by

		/// Memoizes visited nodes to maintain the structure of the DAG
		std::unordered_map<ptr<node>, ptr<node>> visited;
	};  // clone

	/// Implements the derivative computation over a DLF DAG
	class derivative : public visitor, public cut_listener {
		/// Information about a nonterminal
		struct nt_info {
			nt_info(const flags::vector& cuts)
			: cuts{cuts}, nCuts{cuts.empty() ? 0 : cuts.last() + 1}, inDeriv{false} {}

			flags::vector cuts;  ///< Cuts used by non-terminal
			flags::index nCuts;  ///< Number of cuts used by non-terminal
			bool inDeriv;        ///< Flag used when currently in derivative
		}; // nt_info

		/// Cut that can be applied
		struct cut_info {
			cut_info() : blocking{}, fired{false}, refs{0} {}

			flags::vector blocking;  ///< Restrictions that can block the cut
			bool fired;              ///< Has this cut been fired?
			unsigned long refs;      ///< Reference count
		};  // block_info

		/// Gets the info block for a nonterminal
		nt_info& get_info(ptr<nonterminal> nt) {
			auto it = nt_state.find(nt);
			if ( it == nt_state.end() ) {
				nt_info info(cuts_in(nt->sub));
				return nt_state.emplace_hint(it, nt, std::move(info))->second;
			} else return it->second;
		}

		/// Expands a nonterminal
		ptr<node> expand(ptr<nonterminal> nt, const arc& out, const nt_info& info) {
			flags::index nShift = next_restrict;
			next_restrict += info.nCuts;
			return clone(*nt, out, this, nShift);
		}

		inline ptr<node> expand(ptr<nonterminal> nt, const arc& out) {
			return expand(nt, out, get_info(nt));
		}

		/// Called when a set of cuts is applied
		void block_new() {
			// apply new blocks to root node
			if ( root.blocking.intersects(new_blocked) ) {
				root.succ = fail_node::make();
			} else {
				// check if root node is alt_node, if so check its successors
				if ( root.succ->type() == alt_type ) {
					alt_node& an = *as_ptr<alt_node>(root.succ);
					auto it = an.out.begin();
					while ( it != an.out.end() ) {
						if ( root.blocking.intersects(new_blocked) ) {
							it = an.out.erase(it);
						} else {
							++it;
						}
					}
					if ( an.out.empty() ) { root.succ = fail_node::make(); }
				}
			}

			// apply new blocks to pending list
			auto it = pending.begin();
			while ( it != pending.end() ) {
				flags::index cut = it->first;
				cut_info& info = it->second;

				// release any pending cuts that are now blocked
				if ( info.blocking.intersects(new_blocked) ) {
					new_released |= cut;
					pending.erase(it++);
					continue;
				}

				++it;
			}

			// clear the new_blocked list
			blocked |= new_blocked;
			new_blocked.clear();

			// recurse with newly released nodes
			if ( ! new_released.empty() ) { release_new(); }
		}

		/// Called when a set of cuts will never match
		void release_new() {
			// apply new releases to root node
			root.blocking -= new_released;
			// Also apply to arc successors if alt_node
			if ( root.succ->type() == alt_type ) {
				alt_node& an = *as_ptr<alt_node>(root.succ);
				for (const arc& a : an.out) {
					const_cast<flags::vector&>(a.blocking) -= new_released;
				}
			}

			// apply new releases to pending list
			auto it = pending.begin();
			while ( it != pending.end() ) {
				flags::index cut = it->first;
				cut_info& info = it->second;

				// block any cuts that have been fired pending some newly released blocks
				if ( info.fired ) {
					info.blocking -= new_released;
					if ( info.blocking.empty() ) {
						new_blocked |= cut;
						pending.erase(it++);
						continue;
					}
				}

				++it;
			}

			// clear the new_released list
			released |= new_released;
			new_released.clear();

			// recurse with newly blocked nodes
			if ( ! new_blocked.empty() ) { block_new(); }
		}

		/// Returns a new arc merging in the block-set of another arc
		inline arc merge(const arc& a, const flags::vector& blocking) {
			return arc{a.succ, a.blocking | blocking};
		}

		/// Traverses an arc, modifying it in-place
		arc traverse(arc&& a) {
			// Clear released cuts
			a.blocking -= released;

			// Fail on blocked arc
			if ( a.blocking.intersects(blocked) ) { a.succ = fail_node::make(); return a; }

			// Conditionally apply cut node
			if ( a.succ->type() == cut_type ) {
				const cut_node& cn = *as_ptr<cut_node>(a.succ);
				auto it = pending.find(cn.cut);

				// If cut hasn't already been applied
				if ( it != pending.end() ) {
					cut_info& info = it->second;

					// Apply blocking set to cut
					if ( info.fired ) {
						info.blocking &= a.blocking;
					} else {
						info.fired = true;
						info.blocking = a.blocking;
					}

					// Block if necessary
					if ( info.blocking.empty() ) {
						new_blocked |= cn.cut;
						pending.erase(it);
					}
				}

				// Merge block-set into successor and traverse
				a = merge(cn.out, a.blocking);
				return traverse(std::move(a));
			}

			return a;
		}
		inline arc traverse(const arc& a) { return traverse(arc{a}); }

		/// Takes the derivative of the node on the other side of an arc
		arc&& deriv(arc&& a) {
			pVal = traverse(std::move(a));
			pVal.succ->accept(this);
			pVal.succ.reset();
			return std::move(rVal);
		}
//		inline arc deriv(const arc& a) { return deriv(arc{a}); }
		inline arc&& deriv(const arc& a) { return std::move(deriv(arc{a})); }

		/// Sets a failure arc into rVal
//		inline void fail() { rVal = arc{fail_node::make(), std::move(pVal.blocking)}; }
		inline void fail() { rVal = arc{fail_node::make(), flags::vector{pVal.blocking}}; }

		/// Merges the given arc into rVal
		inline void follow(const arc& a) { rVal = traverse(merge(a, pVal.blocking)); }

		/// Resets rVal to the given node
//		inline void reset(ptr<node> np) { rVal = arc{np, std::move(pVal.blocking)}; }
		inline void reset(ptr<node> np) { rVal = arc{np, flags::vector{pVal.blocking}}; }

		/// Sets rVal to the passed parameter
		inline void pass() { rVal = pVal; }

	public:
		/// Sets up derivative computation for a nonterminal
		derivative(ptr<nonterminal> nt)
		: nt_state{}, blocked{}, released{}, pending{}, new_blocked{}, new_released{},
		  next_restrict{0}, match_ptr{}, root{ptr<node>{}}, 
		  pVal{ptr<node>{}}, rVal{ptr<node>{}}, x{'\0'} {
			ptr<node> mp = match_node::make();
			match_ptr = mp;
			root.succ = expand(nt, arc{mp});
		}

		~derivative() {
			root.succ.reset();  // delete all the arcs before destroying their cut listener
		}

		/// Checks if the current expression is an unrestricted match
		bool matched() const {
			// Check root arc for possible blockages
			if ( ! root.blocking.empty() ) return false;

			// Check for root match
			if ( root.succ->type() == match_type ) return true;

			// Check for alt-match without possibly blocking cut
			if ( root.succ->type() != alt_type ) return false;
			for (const arc& a : as_ptr<alt_node>(root.succ)->out) {
				// Can only be one level of alt-node, so only need to check matches here
				// Can only be one match successor, check it and return
				if ( a.succ->type() == match_type ) return a.blocking.empty();
			}

			return false;
		}

		/// Checks if the current expression cannot match
		bool failed() const { return match_ptr.expired(); }

		/// Called when new cuts are added
		void acquire_cut(flags::index cut) { pending[cut].refs++; assert(pending[cut].refs == 1); }

		/// Called when cuts can no longer be applied
		void release_cut(flags::index cut) {
			auto it = pending.find(cut);
			if ( it == pending.end() ) return;
			cut_info& info = it->second;
			assert(info.refs == 1);
			if ( --info.refs == 0 && ! info.fired ) {
				// Cut never fired, never will now
				new_released |= cut;
				pending.erase(it);
			}
		}

		/// Takes the derviative of the current root node
		void operator() (char x) {
			this->x = x;
			root = deriv(std::move(root));

			if ( ! new_blocked.empty() ) { block_new(); }
			else if ( ! new_released.empty() ) { release_new(); }
		}

		void visit(const match_node&)   { pass(); }
		void visit(const fail_node&)    { pass(); }
		void visit(const inf_node&)     { pass(); }
		void visit(const end_node&)     { assert(false && "Should never take derivative of end node"); }

		void visit(const char_node& n)  { x == n.c ? follow(n.out) : fail(); }

		void visit(const range_node& n) { n.b <= x && x <= n.e ? follow(n.out) : fail(); }

		void visit(const any_node& n)   { x != '\0' ? follow(n.out) : fail(); }

		void visit(const str_node& n)   {
			if ( x == (*n.sp)[n.i] ) {
				n.size() == 1 ?
					follow(n.out) : 
					reset(node::make<str_node>(arc{n.out}, n.sp, n.i+1));
			} else fail();
		}

		void visit(const rule_node& n)  {
			nt_info& info = get_info(n.r);
			// return infinite loop on left-recursion
			if ( info.inDeriv ) { reset(inf_node::make()); return; }

			// take derivative of expanded rule under left-recursion flag
			info.inDeriv = true;
			expand(n.r, n.out, info)->accept(this);
			info.inDeriv = false;
		}

		void visit(const cut_node& n)   {
			assert(false && "should never take derivative of cut node");
			auto it = pending.find(n.cut);
			if ( it != pending.end() ) {
				// Fire unconditionally
				new_blocked |= n.cut;
				pending.erase(it);
			}

			follow(n.out);
		}

		void visit(const alt_node& n)   {
			arc pVal_bak{pVal};
			arc_set as;
			for (const arc& o : n.out) { as.emplace(deriv(o)); }
			pVal = std::move(pVal_bak);

			switch ( as.size() ) {
			case 0:  fail();                                     return;  // no following node
			case 1:  follow(*as.begin());                        return;  // merge single node
			default: reset(node::make<alt_node>(std::move(as))); return;  // replace alt node
			}
		}

	private:
		/// Stored state for each nonterminal
		std::unordered_map<ptr<nonterminal>, nt_info> nt_state;

		flags::vector blocked;                 ///< Blocked indices
		flags::vector released;                ///< Safe indices
		std::unordered_map<flags::index,
		                   cut_info> pending;  ///< Information about outstanding cut indices
		flags::vector new_blocked;             ///< Indices blocked this step
		flags::vector new_released;            ///< Indices released this step

		flags::index next_restrict;            ///< Index of next available restriction
		std::weak_ptr<node> match_ptr;         ///< Pointer to match node
	public:
		arc root;                              ///< Current root arc
	private:
		arc pVal;                              ///< Parameter value for derivative computation
		arc rVal;                              ///< Return value from derivative computation
		char x;                                ///< Character to take derivative with respect to
	};  // derivative

	/// Recognizes the input
	/// @param l		Loaded DLF DAG
	/// @param in		Input stream
	/// @param rule		Start rule
	/// @param dbg		Print debug output? (default false)
	/// @return true for match, false for failure
	bool match(loader& l, std::istream& in, std::string rule, bool dbg = false) {
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
			if ( dbg ) { p.print(d.root); }

			if ( d.failed() ) return false;
			else if ( d.matched() ) return true;
			else if ( x == '\0' ) return false;

			if ( ! in.get(x) ) { x = '\0'; }  // read character, \0 for EOF

			if ( dbg ) {
				std::cout << "d(\'" << (x == '\0' ? "\\0" : strings::escape(x)) << "\') =====>"
				          << std::endl;
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
	bool match(ast::grammar& g, std::istream& in, std::string rule, bool dbg = false) {
		loader l(g, dbg);
		return match(l, in, rule, dbg);
	}

}  // namespace dlf
