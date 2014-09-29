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

#include "../ast.hpp"
#include "../dlf.hpp"

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
	
	public:
		/// Builds a DLF parse DAG from the given PEG grammar
		loader(ast::grammar& g, bool dbg = false) {
			// Read in rules
			for (auto r : g.rs) {
				rVal = nullptr;
				postVal = end_node::make();
				r->m->accept(this);
				set_nonterminal(r->name, rVal);
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
		
		virtual void visit(ast::char_matcher& m) {}
		virtual void visit(ast::str_matcher& m) {}
		virtual void visit(ast::range_matcher& m) {}
		virtual void visit(ast::rule_matcher& m) {}
		virtual void visit(ast::any_matcher& m) {}
		virtual void visit(ast::empty_matcher& m) {}
		virtual void visit(ast::action_matcher& m) {}
		virtual void visit(ast::opt_matcher& m) {}
		virtual void visit(ast::many_matcher& m) {}
		virtual void visit(ast::some_matcher& m) {}
		virtual void visit(ast::seq_matcher& m) {}
		virtual void visit(ast::alt_matcher& m) {}
		virtual void visit(ast::look_matcher& m) {}
		virtual void visit(ast::not_matcher& m) {}
		virtual void visit(ast::capt_matcher& m) {}
		virtual void visit(ast::named_matcher& m) {}
		virtual void visit(ast::fail_matcher& m) {}
	private:
		std::map<std::string, ptr<nonterminal>> nts;  ///< List of non-terminals
		ptr<node> postVal;                            ///< Node that comes after this
		ptr<node> rVal;                               ///< Return value from node visit
	}; // loader

}  // namespace dlf

