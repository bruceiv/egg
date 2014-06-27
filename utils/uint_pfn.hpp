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

#include <algorithm>
#include <utility>
#include <vector>

#include "uint_set.hpp"

namespace utils {

/// A partial function from unsigned int to unsigned int; assumed to be monotonically increasing.
class uint_pfn {
public:
	typedef unsigned int                                                   value_type;
	typedef uint_set                                                       set_type;
	typedef std::vector<std::pair<value_type,value_type>>::size_type       size_type;
	typedef std::vector<std::pair<value_type,value_type>>::difference_type difference_type;
	typedef std::vector<std::pair<value_type,value_type>>::const_iterator  iterator;
	typedef std::vector<std::pair<value_type,value_type>>::const_iterator  const_iterator;
	
	// Use implicit c'tors, d'tors, assignment operator
	uint_pfn() = default;
	uint_pfn(const uint_pfn&) = default;
	uint_pfn(uint_pfn&&) = default;
	
	/// Initialize to a given list (must be sorted in increasing order)
	uint_pfn(std::initializer_list<value_type> xs) : fm() {
		value_type i = 0;
		for (value_type xi : xs) {
			fm.emplace_back(i, xi);
			++i;
		}
	}
	
	uint_pfn& operator= (const uint_pfn&) = default;
	uint_pfn& operator= (uint_pfn&&) = default;
	
	~uint_pfn() = default;
	
	/// Adds a mapping to the function (must be greater than all previous values)
	inline void add_back(value_type i, value_type fi) {
		assert(fm.empty() || (i > fm.back().first && fi > fm.back().second) && "adds in strict order");
		fm.emplace_back(i, fi);
	}
	
	/// Gets the value of the function for i (undefined if i not in)
	value_type operator() (value_type i) const {
		size_type begin = 0, end = fm.size();
		while ( begin < end ) {
			size_type mid = (begin+end)/2;
			value_type j = fm[mid].first;
			if ( i == j ) return fm[mid].second;
			if ( i < j ) { end = mid; }
			else /* if ( i > j ) */ { begin = mid; }
		}
		assert(false && "Index must be in pfn");
		return (value_type)-1;
	}
	
	/// Gets the values of the function for a set of indices (undefined if s not subset of domain)
	set_type operator() (const set_type& s) const {
		uint_set fs;
		auto ft = fm.begin();
		auto st = s.begin();
		while ( ft != fm.end() && st != s.end() ) {
			if ( ft->first < *st ) { ++ft; continue; }
			assert(ft->first == *st && "Index must be in pfn");
			fs.add_back(ft->second);
			++ft; ++st;
		}
		assert(st == s.end() && "Index must be in pfn");
		return fs;
	}
	
	/// Gets the composition of this function with another (undefined if range(g) not subset of 
	/// domain)
	uint_pfn operator() (const uint_pfn& g) const {
		uint_pfn fg;
		auto ft = fm.begin();
		auto gt = g.begin();
		while ( ft != fm.end() && gt != g.end() ) {
			if ( ft->first < gt->second ) { ++ft; continue; }
			assert(ft->first == gt->second && "Index must be in pfn");
			fg.add_back(gt->first, ft->second);
			++ft; ++gt;
		}
		assert(gt == g.end() && "Index must be in pfn");
		return fg;
	}
	
	/// Deep equality check for two functions
	bool operator== (const uint_pfn& o) const {
		if ( fm.size() != o.fm.size() ) return false;
		return std::equal(fm.begin(), fm.end(), o.fm.begin());
	}
	
	/// Deep inequality check for two nodes
	inline bool operator!= (const uint_pfn& o) const { return ! operator==(o); }
	
	/// Minimum value of function (undefined if empty)
	inline value_type min() const {
		assert(!fm.empty() && "can't call min on empty pfn");
		return fm.begin()->second;
	}
	
	/// Maximum value of function (undefined if empty)
	inline value_type max() const {
		assert(!fm.empty() && "can't call max on empty pfn");
		return fm.rbegin()->second;
	}
	
	/// Maximum value of function's domain (undefined if empty)
	inline value_type max_key() const {
		assert(!fm.empty() && "can't call max_key on empty pfn");
		return fm.rbegin()->first;
	}
	
	/// Is this set empty?
	inline bool empty() const { return fm.empty(); }
	
	// Just forward iterator calls (use only constant iterators)
	inline const_iterator begin()  const { return fm.cbegin(); }
	inline const_iterator cbegin() const { return fm.cbegin(); }
	inline const_iterator end()    const { return fm.cend(); }
	inline const_iterator cend()   const { return fm.cend(); }
	
private:
	std::vector<std::pair<value_type, value_type>> fm; // underlying map
}; // class uint_map

} // namespace utils
