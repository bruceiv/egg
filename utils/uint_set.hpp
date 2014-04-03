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

#include <initializer_list>

#include <algorithm>
#include <iterator>
#include <set>

namespace utils {

/// Stores a set of unsigned integers
class uint_set {
public:
	typedef unsigned int                             value_type;
	typedef std::vector<value_type>::size_type       size_type;
	typedef std::vector<value_type>::difference_type difference_type;
	typedef std::vector<value_type>::iterator        iterator;
	typedef std::vector<value_type>::const_iterator  const_iterator;
	
	// Use implicit c'tors, d'tors, assignment operator
	uint_set() = default;
	uint_set(const uint_set&) = default;
	uint_set(uint_set&&) = default;
	/// Initialize to a given list
	uint_set(std::initializer_list<value_type> xs) : xs(xs) {}
	
	uint_set& operator= (const uint_set&) = default;
	uint_set& operator= (uint_set&&) = default;
	
	~uint_set() = default;
	
	/// Adds a value to the set (values must be added in increasing order)
	inline uint_set& operator|= (value_type x) {
		if ( xs.back() < x ) { xs.push_back(x); }
		return *this;
	}
	
	inline uint_set operator| (value_type x) {
		uint_set out = *this;
		out |= x;
		return out;
	}
	
	/// Takes the union of two sets
	uint_set operator| (const uint_set& o) const {
		uint_set out;
		std::set_union(xs.begin(), xs.end(), o.xs.begin(), o.xs.end(), std::back_inserter(out.xs));
		return out;
	}
	
	inline uint_set& operator|= (const uint_set& o) {
		return *this = *this | o;
	}
	
	/// Gets the i'th entry in this set (not bounds checked)
	inline value_type operator() (value_type i) const { return xs[i]; }
	
	/// Returns the set of the i'th entries in this set for each i in is (not bounds checked)
	uint_set operator() (uint_set is) const {
		uint_set ms;
		for (auto& i : is) { ms.xs.push_back(xs[i]); }
		return ms;
	}
	
	/// Deep equality check for two nodes
	bool operator== (const uint_set& o) const {
		if ( xs.size() != o.xs.size() ) return false;
		
		for (size_type i = 0; i < xs.size(); ++i) {
			if ( xs[i] != o.xs[i] ) return false;
		}
		
		return true;
	}
	
	/// Number of elements
	inline size_type count() const { return xs.size(); }
	
	inline value_type min() const { return xs.front(); }
	
	inline value_type max() const { return xs.back(); }
	
	/// Is this set empty?
	inline bool empty() const { return xs.empty(); }
	
	// Just forward iterator calls (use only constant iterators)
	inline const_iterator begin()  const { return xs.cbegin(); }
	inline const_iterator cbegin() const { return xs.cbegin(); }
	inline const_iterator end()    const { return xs.cend(); }
	inline const_iterator cend()   const { return xs.cend(); }
	
private:
	std::vector<value_type> xs;  ///< Underlying list
}; // class uint_set

} // namespace utils

