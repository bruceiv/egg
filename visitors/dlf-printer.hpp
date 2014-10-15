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

#include <iostream>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "../dlf.hpp"
#include "../utils/strings.hpp"

namespace dlf {

	// Pretty-printer for DLF parse trees
	class printer : public visitor {
		// np passed by reference to not throw off ref-count
		void print_deduped(const ptr<node>& np) {
			// look up node
			const node* key = np.get();
			auto it = dups.find(key);
			
			if ( it == dups.end() ) {
				// not in duplicated set
				
				// just print non-duplicated nodes (and singleton terminals)
				auto ty = np->type();
				if ( np.unique() || ty == fail_type || ty == inf_type ) {
					np->accept(this);
					return;
				}

				// otherwise add to set and print
				unsigned long ni = dups.size();
				dups.emplace(key, ni);
				out << ":" << ni << " ";
				np->accept(this);
			} else {
				// already seen
				out << "@" << it->second;
			}
		}

		/// Prints an arc, with its restrictions and successor
		void print(const arc& a) {
			if ( ! a.blocking.restricted.empty() ) {
				out << "[";
				for (auto i : a.blocking.restricted) { out << " " << i; }
				out << " ] ";
			}
			print_deduped(a.succ);
		}
		
		void print_nts() {
			for (auto it = pl.begin(); it != pl.end(); ++it) {
				out << (*it)->name << " := ";
				print_deduped((*it)->get());
				out << std::endl;
			}
			pl.clear();
		}
	public:
		printer(std::ostream& out = std::cout, 
		        const std::unordered_set<ptr<nonterminal>>& rp 
		              = std::unordered_set<ptr<nonterminal>>{}) 
			: out{out}, rp{rp}, pl{}, dups{} {}
			
		/// Prints the definition of a rule
		void print(ptr<nonterminal> nt) {
			dups.clear();  // nodes could have been mutated since last print
			rp.emplace(nt);
			pl.emplace_back(nt);
			print_nts();
		}
		
		/// Prints an expression
		void print(ptr<node> n) {
			dups.clear(); // nodes could have been mutated since last print
			// Print the node, followed by any unprinted rules
			print_deduped(n);
			out << std::endl;
			print_nts();
		}
		
		/// Prints the expression; optionally provides output stream and pre-printed rules
		static void print(ptr<node> n, std::ostream& out, 
		                  const std::unordered_set<ptr<nonterminal>>& rp 
		                        = std::unordered_set<ptr<nonterminal>>{}) {
			printer p(out, rp);
			p.print(n);
		}
		
		virtual void visit(match_node&)  { out << "{MATCH}"; }
		virtual void visit(fail_node&)   { out << "{FAIL}"; }
		virtual void visit(inf_node&)    { out << "{INF}"; }
		virtual void visit(end_node&)    { out << "{END}"; }
		
		virtual void visit(char_node& n) {
			out << "\'" << strings::escape(n.c) << "\' ";
			print(n.out);
		}
		
		virtual void visit(range_node& n) {
			out << "\'" << strings::escape(n.b) << "-" << strings::escape(n.e) << "\' ";
			print(n.out);
		}
		
		virtual void visit(any_node& n) {
			out << ". ";
			print(n.out);
		}
		
		virtual void visit(str_node& n) {
			out << "\"" << strings::escape(n.str()) << "\" ";
			print(n.out);
		}
		
		virtual void visit(rule_node& n) {
			// Mark rule for printing if not yet printed
			if ( rp.count(n.r) == 0 ) {
				rp.emplace(n.r);
				pl.emplace_back(n.r);
			}
			out << n.r->name << " ";
			print(n.out);
		}
		
		virtual void visit(alt_node& n) {
			out << "( ";
			auto it = n.out.begin();
			if ( it != n.out.end() ) do {
				print(*it);
				if ( ++it == n.out.end() ) break;
				out << " | ";
			} while (true);
			out << ")";
		}
		
		virtual void visit(cut_node& n) {
			out << "<" << n.i << "> ";
			print(n.out);
		}
		
	private:
		std::ostream& out;                        ///< output stream
		std::unordered_set<ptr<nonterminal>> rp;  ///< Rules that have already been printed
		std::list<ptr<nonterminal>> pl;           ///< List of rules to print
		std::unordered_map<const node*,
                                   unsigned long> dups;   ///< IDs for duplicated nodes
	}; // printer

} // namespace dlf

