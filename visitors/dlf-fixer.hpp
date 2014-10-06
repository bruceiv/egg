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

#include <unordered_map>
#include <unordered_set>

#include "../dlf.hpp"

namespace dlf {
	
	/// Visitor with function-like interface to calculate the set of nonterminals a rule depends on
	class depends_on : public iterator {
	public:
		depends_on(ptr<nonterminal> nt) : iterator{}, depends{} { visit(np->begin); }
		operator std::unordered_set<ptr<nonterminal>> () { return depends; }
		
		virtual void visit(rule_node&) { depends.emplace(n.r); iterator::visit(n); }
	private:
		std::unordered_set<ptr<nonterminal>> depends;  ///< Dependency set
	};  // depends_on
	
	/// Calculates the least fixed point of nullability for DLF nonterminals and modifies the 
	/// nonterminals appropriately. Approach based on Kleene's fixed point theorem, as implemented 
	/// in the derivative parser of Might et al. for CFGs.
	class fixer : public visitor {
	public:
		/// Calculates the least fixed point of nullability for nt and modifies it appropriately
		void operator() (ptr<nonterminal> nt);
		
		virtual void visit(match_node&);
		virtual void visit(fail_node&);
		virtual void visit(inf_node&);
		virtual void visit(end_node&);
		virtual void visit(char_node&);
		virtual void visit(range_node&);
		virtual void visit(any_node&);
		virtual void visit(str_node&);
		virtual void visit(rule_node&);
		virtual void visit(alt_node&);
		virtual void visit(cut_node&);
	private:
		/// Performs fixed point computation to calculate nullability of np.
		/// Based on Kleene's theorem, iterates upward from a bottom of false
		bool fix_match(ptr<node> np);
		
		/// Set of nonterminals already fixed
		std::unordered_set<ptr<nonterminal>> fixed;
		/// Set of in progress non-terminals
		std::unordered_set<ptr<nonterminal>> running;
		
		/// Set of 
		/// Map from nonterminals to the nonterminals they modify
		std::unordered_map<ptr<nonterminal>,
		                   std::unordered_set<ptr<nonterminal>> changes;
	}; // fixer
	
} // namespace dlf

