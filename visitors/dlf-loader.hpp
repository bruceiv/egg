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
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../ast.hpp"
#include "../dlf.hpp"

#include "../utils/strings.hpp"

#include "dlf-printer.hpp"

namespace dlf {
	/// Generates a name for a many-rule; if equivalent names are returned the rules are equivalent
	class many_name : public ast::visitor {
	public:
		many_name(ast::matcher_ptr p, char suffix, unsigned long& anon_index)
			: ss(), mi(anon_index) { p->accept(this); ss << suffix; }
		operator std::string () { return ss.str(); }
		
		virtual void visit(ast::char_matcher& m) { ss << "\'" << strings::escape(m.c) << "\'"; }
		virtual void visit(ast::str_matcher& m) { ss << "\"" << strings::escape(m.s) << "\""; }
		virtual void visit(ast::range_matcher& m) {
			ss << "[";

			for (auto iter = m.rs.begin(); iter != m.rs.end(); ++iter) {
				ast::char_range& r = *iter;
				ss << strings::escape(r.from);
				if ( r.from != r.to ) {
					ss << "-" << strings::escape(r.to);
				}
			}
			
			ss << "]";
		}
		virtual void visit(ast::rule_matcher& m) { ss << m.rule; }
		virtual void visit(ast::any_matcher& m) { ss << '.'; }
		virtual void visit(ast::empty_matcher& m) { ss << '~'; }
		virtual void visit(ast::none_matcher& m) { ss << '$'; }
		virtual void visit(ast::action_matcher& m) { ss << '~'; }
		virtual void visit(ast::opt_matcher& m) { ss << '~'; }
		virtual void visit(ast::many_matcher& m) { ss << '~'; }
		virtual void visit(ast::some_matcher& m) { m.m->accept(this); }
		virtual void visit(ast::seq_matcher& m) { ss << mi++; }
		virtual void visit(ast::alt_matcher& m) { ss << mi++; }
		virtual void visit(ast::ualt_matcher& m) { ss << mi++; }
		virtual void visit(ast::look_matcher& m) { ss << '~'; }
		virtual void visit(ast::not_matcher& m) { ss << '~'; }
		virtual void visit(ast::capt_matcher& m) { m.m->accept(this); }
		virtual void visit(ast::named_matcher& m) { m.m->accept(this); }
		virtual void visit(ast::fail_matcher& m) { ss << ';'; }
	private:
		std::stringstream ss;
		unsigned long& mi;
	}; // many_name
	
	/// Loads a set of derivatives from the grammar AST
	class loader : public ast::visitor {
		/// Gets unique nonterminal for each name
		ptr<nonterminal> get_nonterminal(const std::string& s) {
			auto it = nts.find(s);
			if ( it == nts.end() ) {
				ptr<nonterminal> nt = make_ptr<nonterminal>(s);
				nts.emplace(s, nt);
				return nt;
			} else return it->second;
		}

		/// Sets unique nonterminal for each name
		void set_nonterminal(const std::string& s, ptr<node> n) { get_nonterminal(s)->sub = n; }

		/// Gets a unique string pointer for each string literal in the grammar
		ptr<std::string> get_str(const std::string& s) {
			auto it = strs.find(s);
			if ( it == strs.end() ) {
				ptr<std::string> sp = make_ptr<std::string>(s);
				strs.emplace(s, sp);
				return sp;
			} else return it->second;
		}

		/// Produces a new arc to the next node
		arc out() { return arc{next}; }
		/// Produces a new arc to the next node with the given cutset
		arc out(cutset&& blocking) { return arc{next, std::move(blocking)}; }

		/// Makes an anonymous nonterminal for the given many-matcher
		void make_many(ast::matcher_ptr mp) {
			// attempt to retrieve rule from cache
			std::string name = many_name(mp, '*', mi);
			auto it = nts.find(name);
			if ( it != nts.end() ) {
				next = rule_node::make(out(), it->second);
				return;
			}
			
			// idea is to set up a new anonymous non-terminal R_i and set next to R_i
			// R_i = mp <0> R_i end | [0] end

			// set rule node for new anonymous non-terminal
			ptr<nonterminal> R_i = make_ptr<nonterminal>(name);
			ptr<node> nt = rule_node::make(out(), R_i);
			
			// build anonymous rule
			cutind ri_bak = ri; ri = 1;          // save ri
			next = end_node::make();             // make end node for rule
			arc skip = out();                    // save arc that skips match
			next = rule_node::make(out(), R_i);  // build recursive invocation of rule
			next = cut_node::make(out(), 0);     // set up cut on out-edges of many-expression
			skip.block(next);                    // block skip arc on match cut
			mp->accept(this);                    // build many-expression
			ri = ri_bak;                         // restore ri
			R_i->sub = alt_node::make(out(),     // reset rule's substitution
			                          std::move(skip));

			// cache new rule and reset next to rule reference
			nts.emplace(name, R_i);
			next = nt;
		}

		/// Makes an anonymous nonterminal for the given some-matcher
		void make_some(ast::matcher_ptr mp) {
			// attempt to retrieve rule from cache
			std::string name = many_name(mp, '+', mi);
			auto it = nts.find(name);
			if ( it != nts.end() ) {
				next = rule_node::make(out(), it->second);
				return;
			}
			
			// idea is to set up a new anonymous non-terminal R_i and set next to R_i
			// R_i = mp (R_i <0> end | [0] end)

			// set rule node for new anonymous non-terminal
			ptr<nonterminal> R_i = make_ptr<nonterminal>("+" + std::to_string(mi++));
			ptr<node> nt = rule_node::make(out(), R_i);

			// build anonymous rule
			cutind ri_bak = ri; ri = 1;          // save ri
			next = end_node::make();             // make end node for rule
			arc skip = out();                    // save arc that skips match
			next = cut_node::make(out(), 0);     // set up cut for successive match
			skip.block(next);                    // block skip arc on match cut
			next = rule_node::make(out(), R_i);  // build recursive invocation of rule
			next = alt_node::make(out(),         // alternate successor and skip branches
			                      std::move(skip));
			mp->accept(this);                    // match subexpression
			ri = ri_bak;                         // restore ri
			R_i->sub = next;                     // reset rule's substitution

			// cache new rule and reset next to rule reference
			nts.emplace(name, R_i);
			next = nt;
		}

	public:
		/// Builds a DLF parse DAG from the given PEG grammar
		loader(ast::grammar& g, bool dbg = false) : nts(), strs(), next(), ri(0), mi(0) {
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

		virtual void visit(ast::str_matcher& m) { next = str_node::make(out(), get_str(m.s)); }

		virtual void visit(ast::range_matcher& m) {
			arc_set rs;
			for (const ast::char_range& r : m.rs) {
				rs.insert(arc{range_node::make(out(), r.from, r.to)});
			}
			next = alt_node::make(std::move(rs));
		}

		virtual void visit(ast::rule_matcher& m) {
			next = rule_node::make(out(), get_nonterminal(m.rule));
		}

		virtual void visit(ast::any_matcher& m) { next = any_node::make(out()); }

		virtual void visit(ast::empty_matcher& m) { /* do nothing; next remains next */ }
		
		virtual void visit(ast::none_matcher& m) { next = eoi_node::make(out()); }

		virtual void visit(ast::action_matcher& m) { /* TODO implement; for now no-op */ }

		virtual void visit(ast::opt_matcher& m) {
			// Idea: m.m <i> next | [i] next
			cutind i = ri++;                  // get a restriction index to use
			arc skip = out();                 // save arc that skips the optional
			next = cut_node::make(out(), i);  // add blocker for skip branch
			skip.block(next);                 // block skip branch on blocker
			m.m->accept(this);                // build opt-expression
			next = alt_node::make(out(),      // make alternation of two paths
			                      std::move(skip));
		}

		virtual void visit(ast::many_matcher& m) {
#ifdef DLF_NN
			cutind si = ri++;                  // get restriction for skip
			arc skip = out();                  // save arc that skips match
			next = cut_node::make(out(), si);  // add blocker for skip branch
			skip.block(next);                  // block skip branch on blocker
			make_some(m.m);                    // build match arc
			next = alt_node::make(out(),       // alternate match and skip arcs
			                      std::move(skip));
#else
			make_many(m.m);  // generate new many-rule nonterminal
#endif
		}

		virtual void visit(ast::some_matcher& m) {
#ifdef DLF_NN
			make_some(m.m);  // generate new some-rule nonterminal
#else
			make_many(m.m);     // generate new many-rule nonterminal
			m.m->accept(this);  // sequence one copy of the rule before
#endif
		}

		virtual void visit(ast::seq_matcher& m) {
			// build out sequence in reverse order
			for (auto it = m.ms.rbegin(); it != m.ms.rend(); ++it) { (*it)->accept(this); }
		}

		virtual void visit(ast::alt_matcher& m) {
			// Idea: m0 <0> next | [0] m1 <1> next | ... | [0...n-1] mn next
			if ( m.ms.empty() ) { next = fail_node::make(); return; }

			ptr<node> alt_next = next;  // save next value
			ptr<cut_node> new_cut;      // to save new cut to add to block set
			cutset blocking;            // cuts for greedy longest match

			arc_set rs;
			for (unsigned long i = 0; i < m.ms.size() - 1; ++i) {
				auto& mi = m.ms[i];

				cutind ci = ri++;                  // get a restriction index to use
				next = cut_node::make(out(), ci);  // flag cut for greedy longest match
				new_cut = as_ptr<cut_node>(next);  // save cut for later blocking
				mi->accept(this);                  // build subexpression
				rs.insert(out(cutset{blocking}));  // add to list of arcs
				next = alt_next;                   // restore next values for next iteration
				blocking.insert(new_cut.get());    // add new cut to block set
			}
			// Don't put a cut on the last branch
			m.ms.back()->accept(this);
			rs.insert(out(std::move(blocking)));

			next = alt_node::make(std::move(rs));
		}
		
		virtual void visit(ast::ualt_matcher& m) {
			// Idea: m0 next | m1 next | ... | mn next
			if ( m.ms.empty() ) { next = fail_node::make(); return; }
			
			ptr<node> alt_next = next;
			
			arc_set rs;
			for (unsigned long i = 0; i < m.ms.size(); ++i) {
				m.ms[i]->accept(this);
				rs.insert(out());
				next = alt_next;
			}
			next = alt_node::make(std::move(rs));
		}

		virtual void visit(ast::look_matcher& m) {
			// Idea - !!m.m: m.m <j> fail | [j] <i> fail | [i] next
			// If m.m matches, we cut out the [j] <i> branch, freeing next to proceed safely

			// save restriction indices
			cutind j = ri++;
			cutind i = ri++;
			// build continuing branch
			arc cont = out();
			// build cut branch
			next = fail_node::make();
			next = cut_node::make(out(), i);
			cont.block(next);  // block continuing branch on cut
			arc cut = out();
			// build matching branch
			next = fail_node::make();
			next = cut_node::make(out(), j);
			cut.block(next);  // block cut branch on match
			m.m->accept(this);
			// set alternate paths
			next = alt_node::make(out(), std::move(cut), std::move(cont));
		}

		virtual void visit(ast::not_matcher& m) {
			// Idea - match both paths, failing if the not path matches: m.m <i> fail | [i] next
			cutind i = ri++;                  // get a restriction index to use
			arc cont = out();                 // build continuing branch
			next = fail_node::make();         // terminate blocking branch
			next = cut_node::make(out(), i);  // ... with a cut on the match index
			cont.block(next);                 // ... which blocks the continuing branch
			m.m->accept(this);                // build blocking branch
			next = alt_node::make(out(),      // alternate continuing and blocking branches
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
		/// Set of pointers to string literals
		std::unordered_map<std::string, ptr<std::string>> strs;
		ptr<node> next;                               ///< Next node
		cutind ri;                                    ///< Current restriction index
		unsigned long mi;                             ///< Index to uniquely name many-nodes
	}; // loader
}
