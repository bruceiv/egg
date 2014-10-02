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
#include <unordered_set>

#include "../dlf.hpp"
#include "../utils/strings.hpp"

namespace dlf {

	// Pretty-printer for DLF parse trees
	class printer : public visitor {
		//TODO investigate using ptr ref count to trim extra prints of repeated DAG branches
	public:
		printer(std::ostream& out = std::cout, 
		        const std::unordered_set<ptr<nonterminal>>& rp 
		              = std::unordered_set<ptr<nonterminal>>{}) 
			: out{out}, rp{rp}, pl{} {}
		
		/// Prints the definition of a rule
		void print(ptr<nonterminal> nt) {
			out << nt->name " := ";
			nt->get()->accept(this);
			out << std::endl;
		}
		
		/// Prints an expression
		void print(ptr<node> n) {
			if ( n->type() == rule_type ) {
				// Mark this rule as printed
				ptr<nonterminal> r = as_ptr<rule_node>(n)->r;
				if ( rp.count(r) == 0 ) { rp.emplace(r); }
				pl.emplace_back(r);
			} else {
				// Just print the node
				n->accept(this);
				out << std::endl;
			}
			
			// Print any rules we haven't printed yet
			for (auto& nt : pl) { print(nt); }
			pl.clear();
		}
		
		/// Prints an arc, with its restrictions and successor
		void print(const arc& a) {
			if ( ! a.blocking.restricted.empty() ) {
				out << "[";
				for (auto i : a.blocking.restricted) { out << " " << i; }
				out << " ] ";
			}
			a.succ->accept(this);
		}
		
		/// Prints the expression; optionally provides output stream and pre-printed rules
		static void print(ptr<node> n, std::ostream& out = std::cout, 
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
				rp.emplace(r);
				pl.emplace_back(r);
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
		std::vector<ptr<nonterminal>> pl;         ///< List of rules to print
	}; // printer

} // namespace dlf

