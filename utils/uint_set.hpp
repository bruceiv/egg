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
#include <initializer_list>
#include <iterator>
#include <vector>
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
	/// Initialize from a sorted container
	template<typename Iter>
	uint_set(Iter begin, Iter end) : xs(begin, end) {}
	
	uint_set& operator= (const uint_set&) = default;
	uint_set& operator= (uint_set&&) = default;
	
	~uint_set() = default;
	
	/// Adds a value to the set (must be greater than all other values)
	inline void add_back(value_type x) { xs.push_back(x); }
	
	/// Adds a value to the set
	inline uint_set& operator|= (value_type x) {
		if ( xs.empty() ) {
			xs.push_back(x);
		} else {
			auto i = xs.back();
			if ( i < x ) { xs.push_back(x); }
			else if ( i != x ){
				for (auto it = ++(xs.rbegin()); it != xs.rend(); ++it) {
					i = *it;
					if ( i == x ) break;
					if ( i < x ) {
						xs.insert(it.base(), x);
						break;
					}
				}
			}
		}
		return *this;
	}
	
	/// Adds a value to a new set
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
	
	/// Deep equality check for two nodes
	bool operator== (const uint_set& o) const {
		if ( xs.size() != o.xs.size() ) return false;
		return std::equal(xs.begin(), xs.end(), o.xs.begin());
	}
	
	/// Deep inequality check for two nodes
	inline bool operator!= (const uint_set& o) const { return ! operator==(o); }
	
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

/// Adds all elements of a uint_set to a std::set
inline void add_all(std::set<uint_set::value_type>& s, const uint_set& t) {
	s.insert(t.begin(), t.end());
}

} // namespace utils

