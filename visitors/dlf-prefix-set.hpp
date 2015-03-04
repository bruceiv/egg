#pragma once

/*
 * Copyright (c) 2015 Aaron Moss
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

#include "../utils/prefix_set.hpp"

namespace dlf {

	/// An over-approximation of the set of strings matched by an expression
	class prefixes : public visitor {
		using nonterminal_set = std::unordered_set<ptr<nonterminal>>;
	public:
		using prefix_cache = std::unordered_map<ptr<node>, strings::prefix_set>;
		
	private:
		prefixes(nonterminal_set& nts, prefix_cache& cache) : s(), nts(nts), cache(cache) {}
		
		strings::prefix_set get_prefixes(ptr<node> p) {
			auto it = cache.find(p);
			if ( it == cache.end() ) {
				p->accept(this);
				cache.emplace(p, s);
				return s;
			} else return it->second;
		}
		
		strings::prefix_set prefixes_of(ptr<node> p) {
			prefixes ps(nts, cache);
			return ps.get_prefixes(p);
		}
		inline strings::prefix_set prefixes_of(const arc& a) { return prefixes_of(a.succ); }

	public:	
		static strings::prefix_set of(ptr<node> p, prefix_cache& cache) {
			nonterminal_set nts;
			prefixes ps(nts, cache);
			return ps.get_prefixes(p);
		}
		
		static strings::prefix_set of(ptr<node> p) {
			prefix_cache cache;
			return of(p, cache);
		}
	
		void visit(const match_node&)   { s = {""}; }
		void visit(const fail_node&)    {}
		void visit(const inf_node&)     {}
		void visit(const end_node&)     { s = {""}; }
		void visit(const char_node& n)  { s = prefixes_of(n.out).prefixed_with(n.c); }
		void visit(const range_node& n) { s = prefixes_of(n.out).prefixed_with(n.b, n.e); }
		void visit(const any_node& n)   {}
		void visit(const str_node& n)   { s = prefixes_of(n.out).prefixed_with(*n.sp); }
		void visit(const rule_node& n)  {
			if ( nts.count(n.r) > 0 ) { s = {""}; return; }
			nts.insert(n.r);
			
			s = prefixes_of(n.r->sub);
			strings::prefix_set t = prefixes_of(n.out);
			s.insert(t.begin(), t.end());
			
			nts.erase(n.r);
		}
		void visit(const cut_node& n)   { s = prefixes_of(n.out); }
		void visit(const alt_node& n)   {
			for (const arc& a : n.out) {
				strings::prefix_set t = prefixes_of(a);
				s.insert(t.begin(), t.end());
			}
		}
		
	private:
		strings::prefix_set s;  ///< Internal prefix set
		nonterminal_set& nts;   ///< Set of nonterminals we're currently expanding
		prefix_cache& cache;    ///< Cached prefix sets
	}; // prefixes

} // namespace dlf
