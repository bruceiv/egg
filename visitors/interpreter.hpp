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
		void visit(const arc& a) { cuts |= a.cuts; iterator::visit(a); }
	public:
		cuts_in(ptr<node> np) : cuts{} { if ( np ) iterator::visit(np); }
		operator flags::vector () { return cuts; }
	private:
		flags::vector cuts;  ///< Cuts included in this expression
	};  // restrictions_of

	/// Clones a nonterminal, repointing its out-arcs to the given node
	class clone : public visitor {
		/// Common code for visiting an arc
		arc clone_of(const arc& a) {
			return ( a.succ->type() == end_type ) ?
				arc{out.succ, listener,
					out.blocking | (a.blocking << nShift),
					out.cuts | (a.cuts << nShift)} :
			 	arc{clone_of(a.succ), listener, a.blocking << nShift, a.cuts << nShift};
		}  // TODO set up register/unregister ref-counts for cut indices

		/// Functional interface for visitor pattern
		ptr<node> clone_of(const ptr<node> np) {
			auto it = visited.find(np);
			if ( it != visited.end() ) return rVal = it->second;
			np->accept(this);
			visited.emplace(np, rVal);
			return rVal;
		}
	public:
		clone(nonterminal& nt, arc& out,
		      cut_listener* listener = nullptr, flags::index nShift = 0)
		: rVal{out.succ}, out{out}, listener{listener}, nShift{nShift}, visited{} {
			rVal = clone_of(nt->sub);
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
			: cuts{cuts}, nCuts{cuts.count()}, inDeriv{false} {}

			flags::vector cuts;  ///< Cuts used by non-terminal
			flags::index nCuts;  ///< Number of cuts used by non-terminal
			bool inDeriv;        ///< Flag used when currently in derivative
		}; // nt_info

		/// Cut that can be applied
		struct block_info {
			block_info(const flags::vector& cuts, const flags::vector& blocking)
			: cuts{cuts}, blocking{blocking} {}

			void swap(block_info& o) {
				cuts.swap(o.cuts);
				blocking.swap(o.blocking);
			}

			flags::vector cuts;      ///< Cuts that are applied unless blocked
			flags::vector blocking;  ///< Restrictions that can block the cuts
		};  // block_info

		/// Expands a nonterminal
		ptr<node> expand(ptr<nonterminal> nt, ptr<node> succ) {
			flags::index nShift = next_restrict;

			auto it = nt_state.find(nt);
			if ( it == nt_state.end() ) {
				nt_info info{cuts_in(nt.sub)};
				next_restrict += info.nCuts;
				nt_state.emplace(nt, std::move(info));
			} else {
				next_restrict += it.second.nCuts;
			}

			return clone(nt, succ, this, nShift);
		}

		/// Called when a set of cuts is applied
		void block(const flags::vector& cuts) {
			blocked |= cuts;

			flags::vector new_release;
			unsigned long i = 0;
			while ( i < pending.size() ) {
				block_info& info = pending[i];
				if ( info.blocking.intersects(cuts) ) {
					new_release |= info.cuts;
					unsigned long last = pending.size() - 1;
					if ( i < last ) {
						pending[i].swap(pending[last]);
						pending.pop_back();
						continue;
					} else {
						pending.pop_back();
						break;
					}
				}
				++i;
			}

			if ( ! new_release.empty() ) { release_block(new_release); }
		}

		/// Called when a set of cuts will be applied unless the blocking cuts are
		void block_unless(const flags::vector& cuts, const flags::vector& blocking) {
			pending.emplace_back(cuts, blocking);
		}

		/// Called when a set of cuts will never match
		void release_block(const flags::vector& released) {
			flags::vector new_cuts;

			unsigned long i = 0;
			while ( i < pending.size() ) {
				block_info& info = pending[i];
				info.blocking -= released;
				if ( info.blocking.empty() ) {
					new_cuts |= info.cuts;
					unsigned long last = pending.size()-1;
					if ( i < last ) {
						pending[i].swap(pending[last]);
						pending.pop_back();
						continue;
					} else {
						pending.pop_back();
						break;
					}
				}
				++i;
			}

			if ( ! new_cuts.empty() ) { block(new_cuts); }
		}

		/// Follows an arc; returns successor node or failure if blocked
		ptr<node> follow(const arc& a) {
			if ( a.blocking.empty() ) {
				// apply cuts and follow arc
				if ( ! a.cuts.empty() ) { block(a.cuts); }
				return a.succ;
			} else {
				// fail on blocked arc
				if ( a.blocking.intersects(blocked) ) return fail_node::make();
				// apply cuts and follow arc
				block_unless(a.cuts, a.blocking);
				return a.succ->block_succ_on(a.blocking);
			}
		}

	public:
		/// Sets up derivative computation for a nonterminal
		derivative(ptr<nonterminal> nt)
		: nt_state{}, blocked{}, outstanding{}, pending{}, cut_counts{},
		  next_restrict{0}, rVal{}, x{'\0'} {
			ptr<node> mp = match_node::make();
			match_ptr = mp;
			root = expand(nt, mp);
		}

		~derivative() {
			root.reset();  // delete all the arcs before destroying their cut listener
		}

		/// Checks if the current expression is an unrestricted match
		bool matched() const {
			// Check for root match
			if ( root->type() == match_type ) return true;

			// Check for alt-match without blocking cut
			if ( root->type() != alt_type ) return false;
			for (arc& a : as_ptr<alt_node>(root)->out) {
				// Can only be one level of alt-node, so only need to check matches here
				if ( a.succ->type() == match_type
					 && ! a.blocking.intersects(blocked) ) return true;
			}

			return false;
		}

		/// Checks if the current expression cannot match
		bool failed() const { return match_ptr.expired() }

		/// Called when new cuts are added
		void acquire_cuts(const flags::vector& cuts) {
			for (flags::index i : cuts) { ++cut_counts[i]; }
		}

		/// Called when cuts can no longer be applied
		void release_cuts(const flags::vector& cuts) {
			if ( cuts.empty() ) return;
			flags::vector new_release;
			for (flags::index i : cuts) {
				if ( --cut_counts[i] == 0 ) { new_release |= i; }
			}
			if ( ! new_release.empty() ) { release_block(new_release); }
		}

		/// Takes the derviative of the current root node
		void operator(char x) {
			this->x = x;
			root->accept(this);
			root = rVal;
			rVal.reset();
		}

		void visit(const match_node& n) { rVal = n.clone(); }
		void visit(const fail_node&)    { rVal = fail_node::make(); }
		void visit(const inf_node&)     { rVal = inf_node::make(); }
		void visit(const end_node&)     { assert(false && "Should never take derivative of end node"); }

		void visit(const char_node& n)  {
			rVal = ( x == n.c ) ? follow(n.out) : fail_node::make();
		}

		void visit(const range_node& n) {
			rVal = ( n.b <= x && x <= n.e ) ? follow(n.out) : fail_node::make();
		}

		void visit(const any_node& n)   {
			rVal = ( x != '\0' ) ? follow(n.out) : fail_node::make();
		}

		void visit(const str_node& n)   {
			if ( x == (*n.sp)[i] ) {
				rVal = ( n.size() == 1 ) ?
						follow(n.out) :
						node::make<str_node>(n.out, n.sp, n.i+1);
			} else { rVal = fail_node::make(); }
		}

		void visit(const rule_node& n)  {}  // TODO rewrite expand to use out-arcs

		void visit(const alt_node& n)   {}  // TODO

	private:
		/// Stored state for each nonterminal
		std::unordered_map<ptr<nonterminal>, nt_info> nt_state;

		flags::vector blocked;            ///< Blocked indices
		flags::vector outstanding;        ///< Outstanding cut nodes
		std::vector<block_info> pending;  ///< Cuts to be applied if they're not blocked
		/// Ref-count for each cut
		std::unordered_map<flags::index, unsigned long> cut_counts;

		flags::index next_restrict;       ///< Index of next available restriction
		std::weak_ptr<node> match_ptr;    ///< Pointer to match node
	public:
		ptr<node> root;                   ///< Current root node
	private:
		ptr<node> rVal;                   ///< Return value from derivative computation
		char x;                           ///< Character to take derivative with respect to
		bool new_blocks;                  ///< Flag for new blocked indices
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
		derivative d(nt->second());
		if ( dbg ) { p.print(d.root); }

		// Check for initial match
		if ( d.matched() ) {

			return true;
		}

		// teke derivatives until failure, match, or end of input
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
