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

#include <map>
#include <string>

#include "../ast.hpp"
#include "../dlf.hpp"

#include "dlf-printer.hpp"

#include "../utils/flagvector.hpp"

namespace dlf {
	/// Loads a set of derivatives from the grammar AST
	class loader : public ast::visitor {
		/// Gets unique nonterminal for each name
		ptr<nonterminal> get_nonterminal(const std::string& s) {
			auto it = nts.find(s);
			if ( it == nts.end() ) {
				ptr<nonterminal> nt = make_ptr<nonterminal>(s);
				nts.emplace(s, nt);
				return nt;
			} else {
				return it->second;
			}
		}

		/// Sets unique nonterminal for each name
		void set_nonterminal(const std::string& s, ptr<node> n) { get_nonterminal(s)->sub = n; }

		/// Produces a new arc to the next node
		arc out(flags::vector&& blocking = flags::vector{}) { return arc{next, std::move(blocking)}; }

		/// Makes an anonymous nonterminal for the given matcher
		void make_many(ptr<ast::matcher> mp) {
			// idea is to set up a new anonymous non-terminal R_i and set next to R_i
			// R_i = m.m <0> R_i end | [0] end

			// set rule node for new anonymous non-terminal
			ptr<nonterminal> R_i = make_ptr<nonterminal>("*" + std::to_string(mi++));
			ptr<node> nt = rule_node::make(out(), R_i);

			// build anonymous rule
			flags::index ri_bak = ri; ri = 1;      // save ri
			next = end_node::make();               // make end node for rule
			arc skip = out(flags::vector::of(0));  // save arc that skips match
			next = rule_node::make(out(), R_i);    // build recursive invocation of rule
			next = cut_node::make(out(), 0);       // set up cut on out-edges of many-expression
			mp->accept(this);                      // build many-expression
			ri = ri_bak;                           // restore ri
			R_i->sub = alt_node::make(out(),       // reset rule's substitution
                                                  std::move(skip));

			// reset next to rule reference
			next = nt;
		}

	public:
		/// Builds a DLF parse DAG from the given PEG grammar
		loader(ast::grammar& g, bool dbg = false) : nts{}, next{}, ri{0}, mi{0} {
			// Read in rules
			for (auto r : g.rs) {
				next = end_node::make();
				r->m->accept(this);
				set_nonterminal(r->name, next);
				ri = 0;
			}

			if ( dbg ) {
				dlf::printer p;
				for (auto ntp : nts) {
					p.print(ntp.second);
				}
				std::cout << "\n***** DONE LOADING RULES *****\n" << std::endl;
			}
		}

		std::map<std::string, ptr<nonterminal>>& get_nonterminals() { return nts; }

		virtual void visit(ast::char_matcher& m) { next = char_node::make(out(), m.c); }

		virtual void visit(ast::str_matcher& m) { next = str_node::make(out(), m.s); }

		virtual void visit(ast::range_matcher& m) {
			arc_set rs;
			for (const ast::char_range& r : m.rs) {
				rs.emplace(arc{range_node::make(out(), r.from, r.to)});
			}
			next = alt_node::make(std::move(rs));
		}

		virtual void visit(ast::rule_matcher& m) {
			next = rule_node::make(out(), get_nonterminal(m.rule));
		}

		virtual void visit(ast::any_matcher& m) { next = any_node::make(out()); }

		virtual void visit(ast::empty_matcher& m) { /* do nothing; next remains next */ }

		virtual void visit(ast::action_matcher& m) { /* TODO implement; for now no-op */ }

		virtual void visit(ast::opt_matcher& m) {
			// Idea: m.m <i> next | [i] next
			flags::index i = ri++;                 // get a restriction index to use
			arc skip = out(flags::vector::of(i));  // save arc that skips the optional
			next = cut_node::make(out(), i);       // add blocker for skip branch
			m.m->accept(this);                     // build opt-expression
			next = alt_node::make(out(),           // make alternation of two paths
			                      std::move(skip));
		}

		virtual void visit(ast::many_matcher& m) {
			make_many(m.m);  // generate new many-rule nonterminal
		}

		virtual void visit(ast::some_matcher& m) {
			make_many(m.m);     // generate new many-rule nonterminal
			m.m->accept(this);  // sequence one copy of the rule before
		}

		virtual void visit(ast::seq_matcher& m) {
			// build out sequence in reverse order
			for (auto it = m.ms.rbegin(); it != m.ms.rend(); ++it) { (*it)->accept(this); }
		}

		virtual void visit(ast::alt_matcher& m) {
			// Idea: m0 <0> next | [0] m1 <1> next | ... | [0...n-1] mn next
			ptr<node> alt_next = next;  // save next value
			flags::vector blocking;     // cuts for greedy longest match

			arc_set rs;
			for (auto& mi : m.ms) {
				flags::index i = ri++;                     // get a restriction index to use
				next = cut_node::make(out(), i);           // flag cut for greedy longest match
				mi->accept(this);                          // build subexpression
				rs.emplace(out(flags::vector{blocking}));  // add to list of arcs
				next = alt_next;                           // restore next values for next iteration
				blocking |= i;                             // add index to greedy longest match blocker
			}
			next = alt_node::make(std::move(rs));
		}

		virtual void visit(ast::look_matcher& m) {
			// Idea - !!m.m: m.m <j> fail | [j] <i> fail | [i] next
			// If m.m matches, we cut out the [j] <i> branch, freeing next to proceed safely

			// save restriction indices
			flags::index j = ri++;
			flags::index i = ri++;
			// build continuing branch
			arc cont = out(flags::vector::of(i));
			// build cut branch
			next = fail_node::make();
			next = cut_node::make(out(), i);
			arc cut = out(flags::vector::of(j));
			// build matching branch
			next = fail_node::make();
			next = cut_node::make(out(), j);
			m.m->accept(this);
			// set alternate paths
			next = alt_node::make(out(), std::move(cont), std::move(cut));
		}

		virtual void visit(ast::not_matcher& m) {
			// Idea - match both paths, failing if the not path matches: m.m <i> fail | [i] next
			flags::index i = ri++;                  // get a restriction index to use
			arc cont = out(flags::vector::of(i));   // build continuing branch
			next = fail_node::make();               // terminate blocking branch
			next = cut_node::make(out(), i);        // with a cut on the match index
			m.m->accept(this);                      // build blocking branch
			next = alt_node::make(out(),            // alternate continuing and blocking branches
			                      std::move(cont));
		}

		virtual void visit(ast::capt_matcher& m) {
			// TODO implement; for now ignore the capture
			m.m->accept(this);
		}

		virtual void visit(ast::named_matcher& m) {
			// TODO implement; for now ignore the error message
			m.m->accept(this);
		}

		virtual void visit(ast::fail_matcher& m) {
			// TODO complete implementation; for now ignore the error message
			next = fail_node::make();
		}

	private:
		std::map<std::string, ptr<nonterminal>> nts;  ///< List of non-terminals
		ptr<node> next;                               ///< Next node
		flags::index ri;                              ///< Current restriction index
		unsigned long mi;                             ///< Index to uniquely name many-nodes
	}; // loader
}
