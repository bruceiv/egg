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
#include <map>

#include "uint_set.hpp"

namespace utils {

/// A partial function from unsigned int to unsigned int; assumed to be monotonically increasing.
class uint_pfn {
public:
	typedef unsigned int                                      value_type;
	typedef uint_set                                          set_type;
	typedef std::map<value_type, value_type>::size_type       size_type;
	typedef std::map<value_type, value_type>::difference_type difference_type;
	typedef std::map<value_type, value_type>::const_iterator  iterator;
	typedef std::map<value_type, value_type>::const_iterator  const_iterator;
	
	// Use implicit c'tors, d'tors, assignment operator
	uint_pfn() = default;
	uint_pfn(const uint_pfn&) = default;
	uint_pfn(uint_pfn&&) = default;
	
	/// Initialize to a given list
	uint_pfn(std::initializer_list<value_type> xs) : xm() {
		value_type i = 0;
		for (value_type xi : xs) {
			xm.emplace_hint(xm.cend(), i, xi);
			++i;
		}
	}
	
	uint_pfn& operator= (const uint_pfn&) = default;
	uint_pfn& operator= (uint_pfn&&) = default;
	
	~uint_pfn() = default;
	
	/// Adds a mapping to the function
	inline uint_pfn& add(value_type i, value_type xi) {
		xm.emplace_hint(xm.cend(), i, xi);
		return *this;
	}
	
	/// Adds all the mappings in another function
	inline uint_pfn& addAll(const uint_pfn& o) {
		xm.insert(o.xm.begin(), o.xm.end());
		return *this;
	}
	
	/// Gets the value of the function for i
	inline value_type operator() (value_type i) const { return xm.at(i); }
	
	/// Gets the values of the function for a set of indices
	set_type operator() (const set_type& s) const {
		uint_set fs;
		for (auto& i : s) { fs |= xm.at(i); }
		return fs;
	}
	
	/// Gets the composition of this function with another
	uint_pfn operator() (const uint_pfn& g) const {
		uint_pfn fg;
		for (auto& m : g) { fg.add(m.first, xm.at(m.second)); }
		return fg;
	}
	
	/// Deep equality check for two functions
	bool operator== (const uint_pfn& o) const {
		if ( xm.size() != o.xm.size() ) return false;
		return std::equal(xm.begin(), xm.end(), o.xm.begin());
	}
	
	/// Deep inequality check for two nodes
	inline bool operator!= (const uint_pfn& o) const { return ! operator==(o); }
	
	/// Minimum value of function (does not check for empty)
	value_type min() const {
		return xm.begin()->second;
	}
	
	/// Maximum value of function (does not check for empty)
	value_type max() const {
		return xm.rbegin()->second;
	}
	
	/// Maximum value of function's domain (does not check for empty)
	value_type max_key() const {
		return xm.rbegin()->first;
	}
	
	/// Is this set empty?
	inline bool empty() const { return xm.empty(); }
	
	// Just forward iterator calls (use only constant iterators)
	inline const_iterator begin()  const { return xm.cbegin(); }
	inline const_iterator cbegin() const { return xm.cbegin(); }
	inline const_iterator end()    const { return xm.cend(); }
	inline const_iterator cend()   const { return xm.cend(); }
	
private:
	std::map<value_type, value_type> xm;  // underlying map	
}; // class uint_map

} // namespace utils
