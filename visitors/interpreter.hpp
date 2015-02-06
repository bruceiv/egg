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

namespace dlf {
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
		/// Clones a nonterminal nt's substitution into an expression followed by out.
		/// maxcut is the number of cuts in nt's substitution, while nShift is the 
		/// amount to shift the cut indices by.
		clone(nonterminal& nt, const arc& out, cutind maxcut, cutind nShift = 0)
			: rVal(out.succ), out(out), nShift(nShift), visited(), cuts(maxcut) {
			// initialize cut nodes
			for (cutind i = 0; i < maxcut; ++i) {
				cuts[i] = node::make<cut_node>(arc{fail_node::make()}, i + nShift);
			}
			// perform clone
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
		void visit(const eoi_node& n)   { rVal = node::make<eoi_node>(clone_of(n.out)); }
		void visit(const str_node& n)   { rVal = node::make<str_node>(clone_of(n.out), n.sp, n.i); }
		void visit(const rule_node& n)  { rVal = node::make<rule_node>(clone_of(n.out), n.r); }
		void visit(const cut_node& n)   {
			// Rewrite appropriate cut
			cut_node& cn = *as_ptr<cut_node>(cuts[n.cut]);
			cn.out = clone_of(n.out);
			rVal = cuts[n.cut];
		}
		void visit(const alt_node& n)   {
			arc_set rs;
			for (const arc& a : n.out) { rs.insert(clone_of(a)); }
			rVal = node::make<alt_node>(std::move(rs));
		}

	private:
		ptr<node> rVal;          ///< Return value of last visit
		const arc out;           ///< Replacement for end nodes
		cutind nShift;           ///< Amount to shift restrictions by

		/// Memoizes visited nodes to maintain the structure of the DAG
		std::unordered_map<ptr<node>, ptr<node>> visited;
		/// List of cloned cut nodes (need to be built before outgoing arcs)
		std::vector<ptr<node>> cuts;
	};  // clone

	/// Implements the derivative computation over a DLF DAG
	class derivative {
		/// Information about a nonterminal
		struct nt_info {
			nt_info(ptr<nonterminal> nt) : nCuts(cut_end(nt->sub)), inDeriv(false) {}

			cutind nCuts;  ///< Number of cuts used by non-terminal
			bool inDeriv;  ///< Flag used when currently in derivative
		}; // nt_info

		/// Gets the info block for a nonterminal
		nt_info& get_info(ptr<nonterminal> nt) {
			auto it = nt_state.find(nt);
			if ( it == nt_state.end() ) {
				nt_info info(nt);
				return nt_state.emplace_hint(it, nt, std::move(info))->second;
			} else return it->second;
		}

		/// Expands a nonterminal
		ptr<node> expand(ptr<nonterminal> nt, const arc& out, const nt_info& info) {
			cutind nShift = next_restrict;
			next_restrict += info.nCuts;
			return clone(*nt, out, info.nCuts, nShift);
		}

		inline ptr<node> expand(ptr<nonterminal> nt, const arc& out) {
			return expand(nt, out, get_info(nt));
		}

		/// Functionalal interface to actually take the derivative
		class deriv : public visitor {
			void traverse() {
				// Fail on blocked arc
				if ( pVal.blocked() ) return;

				switch ( pVal.succ->type() ) {
				// Conditionally apply cut node
				case cut_type: { 
					cut_node& cn = *as_ptr<cut_node>(pVal.succ);
					assert(pVal.blocking.count(&cn) == 0 && "cut cannot block itself");
					
					if ( pVal.blocking.empty() ) {
						cn.fire();
					} else {
						// save copy of self on pending list
						arc c{cut_node::make(arc{}, cn.cut, cn.blocked), pVal.blocking};
						st.pending.insert(std::move(c));
					}
					
					// Merge block-set into successor and traverse
					follow(cn.out);
					return;
				}
				// Conditionally apply match node
				case match_type: {
					st.pending_matches.emplace(st.input_index, arc{pVal});
					pVal.block();
					return;
				}
				// Traverse all branches of an alternation
				case alt_type: {
					const alt_node& an = *as_ptr<alt_node>(pVal.succ);

					arc bak{pVal};

					arc_set as;
					for (const arc& aa : an.out) {
						follow(aa);
						as.insert(arc{pVal});
						pVal = bak;
					}

					switch ( as.size() ) {
					case 0:  pVal.block(); return;
					case 1:  pVal = *as.begin(); return;
					default: pVal = arc{alt_node::make(std::move(as))}; return;
					}
				}
				// Return non-mutator node
				default: return;
				}
			}

			/// Sets a failure arc into rVal
			inline void fail() { rVal.block(); }

			/// Merges the given arc into rVal
			inline void follow(const arc& a) {
				arc f{a};
				f.block_all(pVal.blocking);
				pVal = std::move(f);
				traverse();
				rVal = pVal;
			}

			/// Resets rVal to the given node
			inline void reset(ptr<node> np) { rVal = arc{np, pVal.blocking}; }
			inline void reset(const arc& a) { rVal = a; }

			/// Does nothing to rVal
			inline void pass() {}

			/// Takes the derivative of another arc
			inline arc d(const arc& a) { return deriv(a, x, st); }
			inline void take_deriv() {
				traverse();
				if ( ! pVal.is_blocked ) pVal.succ->accept(this);
			}
		
		public:	
			deriv(const arc& a, char x, derivative& st)
				: st(st), pVal(a), rVal(fail_node::make()), x(x) { take_deriv(); }
			operator arc() { return rVal; }

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
			
			void visit(const eoi_node& n)   { x == '\0' ? follow(n.out) : fail(); }

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
				pVal.succ = st.expand(n.r, n.out, info);
				take_deriv();
				info.inDeriv = false;
			}

			void visit(const cut_node& n) {
				assert(false && "should never take derivative of cut node");
			}

			void visit(const alt_node& n) {
				arc_set as;
				for (arc o : n.out) {
					o.block_all(pVal.blocking);
					as.insert(d(o));
				}

				switch ( as.size() ) {
				case 0: fail(); return;  // no following node
				case 1: reset(*as.begin()); return;  // replace with single node
				default: reset(arc{node::make<alt_node>(std::move(as)), cutset{}}); return;  // replace alt
				}
			}

		private:
			derivative& st;   ///< Derivative state
			arc pVal;         ///< Parameter value
			arc rVal;         ///< Return value
			char x;           ///< Character to take derivative with respect to
		};  // deriv

		inline arc d(const arc& a, char x) { return deriv(a, x, *this); }

	public:
		/// Sets up derivative computation for a nonterminal
		derivative(ptr<nonterminal> nt)
			: nt_state(), pending(), input_index(0), pending_matches(),
			  next_restrict(0), match_ptr(), root(ptr<node>{}) {
			ptr<node> mp = match_node::make();
			match_ptr = mp;
			root.succ = expand(nt, arc{mp});
		}

		/// Checks if the current expression is an unrestricted match
		bool matched() const {
			for (const auto& p : pending_matches) {
				if ( p.second.blocking.empty() ) return true;
			}
			return false;
		}

		/// Checks if the current expression cannot match
		bool failed() const { return pending_matches.empty() && match_ptr.expired(); }

		/// Takes the derviative of the current root node
		void operator() (char x) {
			root = d(root, x);
			
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
		}

	private:
		/// Stored state for each nonterminal
		std::unordered_map<ptr<nonterminal>, nt_info> nt_state;
	public:
		/// Information about outstanding cut indices
		bag<arc> pending;
		unsigned long input_index;      ///< Current index in the input
		/// Information about possible matches
		bag<std::pair<unsigned long, arc>> pending_matches;
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
					if ( ! c.blocking.empty() ) {
						auto ii = c.blocking.begin();
						std::cout << (*ii)->cut;
						while ( ++ii != c.blocking.end() ) {
							std::cout << " " << (*ii)->cut;
						}
					}
					std::cout << "] ";
				}
				// print pending matches
				for (auto m : d.pending_matches) {
					std::cout << "@" << m.first << ":[";
					if ( ! m.second.blocking.empty() ) {
						auto ii = m.second.blocking.begin();
						std::cout << (*ii)->cut;
						while ( ++ii != m.second.blocking.end() ) {
							std::cout << (*ii)->cut;
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
