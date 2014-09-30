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
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../ast.hpp"
#include "../dlf.hpp"

#include "../utils/flagvector.hpp"

#include "dlf-printer.hpp"

namespace dlf {

	/// Loads a set of derivatives from the grammar AST
	class loader : public ast::visitor {
		/// Gets unique nonterminal for each name
		ptr<nonterminal> get_nonterminal(const std::string& s) {
			auto it = nts.find(s);
			if ( it == nts.end() ) {
				ptr<nonterminal> nt = make_ptr<nonterminal>(s, fail_node::make());
				nts.emplace(s, nt);
				return nt;
			} else {
				return it->second;
			}
		}
		
		/// Sets unique nonterminal for each name
		void set_nonterminal(const std::string& s, ptr<node> n) {
			ptr<nonterminal> nt = get_nonterminal(s);
			nt->begin = n;
			nt->nRestrict = count_restrict(n);
		}
		
		/// Produces a new restriction check from the current manager
		restriction_ck ck(flags::vector&& blocking = flags::vector{}) {
			return restriction_ck{mgr, std::move(blocking)};
		}
		
		/// Produces a new arc to the next node
		arc out(flags::vector&& blocking = flags::vector{}) {
			return arc{next, ck(std::move(blocking))};
		}
		
		void make_many(ptr<ast::matcher> mp) {
			// idea is to set up a new anonymous non-terminal R_i and set next to R_i
			// R_i = m.m < ri > R_i | end
			
			// set rule node for new anonymous non-terminal
			ptr<nonterminal> R_i = 
					make_ptr<nonterminal>("*" + std::to_string(mi++), inf_node::make());
			ptr<node> nt = rule_node::make(out(), R_i, mgr);
			
			// build anonymous rule
			flags::index i = ri++;                        // get a restriction index to use
			next = end_node::make();                      // make end node for rule
			arc skip = out(flags::vector::of(i));         // save arc that skips match
			next = rule_node::make(out(), R_i, mgr);      // build recursive invocation of rule
			next = cut_node::make(out(), i, mgr);         // build cut node to succeed match
			flags::index ri_bak = ri; ri = 0;             // save ri
			mp->accept(this);                             // build many-expression
			ri = ri_bak;                                  // restore ri
			R_i->begin = alt_node::make({out(), skip});   // reset rule ...
			R_i->nRestrict = count_restrict(R_i->begin);  // ... and count of restrictions
			
			// reset next to rule reference
			next = nt;
		}

	public:
		/// Builds a DLF parse DAG from the given PEG grammar
		loader(ast::grammar& g, bool dbg = false) : nts{}, next{}, mgr{}, ri{0}, mi{0} {
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
			std::vector<arc> rs;
			for (const char_range& r : m.rs) {
				rs.emplace_back(arc{range_node::make(out(), r.from, r.to), ck()});
			}
			next = alt_node::make(rs.begin(), rs.end());
		}
		
		virtual void visit(ast::rule_matcher& m) {
			next = rule_node::make(out(), get_nonterminal(m.rule), mgr);
		}
		
		virtual void visit(ast::any_matcher& m) { next = any_node::make(out()); }
		
		virtual void visit(ast::empty_matcher& m) { /* do nothing; next remains next */ }
		
		virtual void visit(ast::action_matcher& m) { /* TODO implement; for now no-op */ }
		
		virtual void visit(ast::opt_matcher& m) {
			// Idea: m.m < i > next | [i] next
			flags::index i = ri++;                 // get a restriction index to use
			arc skip = out(flags::vector::of(i));  // save arc that skips the optional
			next = cut_node::make(out(), i, mgr);  // build cut node to succeed match
			m.m->accept(this);                     // build opt-expression
			next = alt_node::make({out(), skip});  // make alternation of two paths
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
			ptr<node> alt_next = next; // save next value
			flags::vector blocking;    // for greedy longest match
			
			std::vector<arc> rs;
			for (auto& mi : m.ms) {
				flags::index i = ri++;                 // get a restriction index to use
				next = cut_node::make(out(), i, mgr);  // build cut for greedy longest match
				mi->accept(this);                      // build subexpression
				rs.emplace_back(out());                // add to list of arcs
				next = alt_next;                       // restore next pointer for next iteration
				blocking |= i;                         // add index to greedy longest match blocker
			}
			next = alt_node::make(rs.begin(), rs.end());
		}
		
		virtual void visit(ast::look_matcher& m) {
			// Idea - !!m.m: m.m <j> fail | [j] <i> fail | [i] next
			// If m.m matches, we cut out the <i> branch, freeing next to proceed safely
			
			// save restriction indices and next pointer
			flags::index i = ri++;
			flags::index j = ri++;
			ptr<node> next_bak = next;
			// build cut branch
			next = fail_node::make();
			next = cut_node::make(out(), i, mgr);
			arc cut = out(flags::vector::of(j));
			// build matching branch
			next = fail_node::make();
			next = cut_node::make(out(), j, mgr);
			// alternate paths
			next = alt_node::make({
					arc{next_bak, ck(flags::vector::of(i))},
					cut,
					out()});
		}
		
		virtual void visit(ast::not_matcher& m) {
			// Idea - match both paths, failing if the not path matches: m.m <i> fail | [i] next
			flags::index i = ri++;                 // get a restriction index to use
			ptr<node> next_bak = next;             // save next pointer
			next = fail_node::make();              // terminate blocking path
			next = cut_node::make(out(), i, mgr);  // cut straight path on match
			m.m->accept(this);                     // build blocking path
			next = alt_node::make({                // alternate straight and blocking paths
					arc{next_bak, ck(flags::vector::of(i))}, 
					out()});
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
		restriction_mgr mgr;                          ///< Restriction manager to set up
		flags::index ri;                              ///< Current restriction index
		unsigned long mi;                             ///< Index to uniquely name many-nodes
	}; // loader

}  // namespace dlf

