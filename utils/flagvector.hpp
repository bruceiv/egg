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

//uncomment to disable asserts
//#define NDEBUG
#include <cassert>

#include <algorithm>
#include <cstdint>
#include <vector>
#include <utility>

#include "flags.hpp"

namespace flags {
	
	using index = uint64_t;

	/// A vector of bitflags. Provides a more C++ style interface to flags.hpp
	class vector {
		/// Data constructor; sets the storage vector directly
		vector(std::vector<uint64_t>&& d) : v{std::move(d)} {}
	public:
		/// Default constructor; creates empty bitset
		vector() = default;
		/// Sized constructor; creates zeroed bitset with initial allocation for at least n bits
		vector(index n) : v{(n+63) >> 6, 0} {}
		
		vector(const vector& o) = default;
		vector(vector&& o) = default;
		vector& operator= (const vector& o) = default;
		vector& operator= (vector&& o) = default;
		
		/// Creates a singleton vector, with the single index set
		static vector of(index n) {
			vector s{n};
			s |= n;
			return s;
		}
		
		/// Gets the index of the first bit set (-1 for none such)
		index first() const {
			return v.size() == 1 ? 
				flags::first(v.front())
				: flags::first(v.data(), v.size());
		}

		/// Gets the index of the next bit set after i (-1 for none such)
		index next(index i) const {
			return v.size() == 1 ? 
				flags::next(v.front(), i) 
				: flags::next(v.data(), v.size(), i);
		}

		/// Gets the index of the last bit set (-1 for none such)
		index last() const {
			return v.size() == 1 ? 
				flags::last(v.front()) 
				: flags::last(v.data(), v.size());
		}
		
		/// Gets the count of bits set
		index count() const {
			return v.size() == 1 ? 
				flags::count(v.front()) 
				: flags::count(v.data(), v.size());
		}
		
		/// Gets the number of bits set before i
		index rank(index i) const {
			return v.size() == 1 ? 
				flags::rank(v.front(), i) 
				: flags::rank(v.data(), i);
		}
		
		/// Gets the index of the j'th bit
		index select(index j) const {
			return v.size() == 1 ? 
				flags::select(v.front(), j) 
				: flags::select(v.data(), v.size(), j);
		}
		
		/// Checks if all flags are zeroed
		bool empty() const {
			switch ( v.size() ) {
			case 0:  return true;
			case 1:  return is_zero(v.front());
			default: return is_zero(v.data(), v.size());
			}
		}
		
		/// Checks if this intersects another vector
		bool intersects(const vector& o) const {
			index n = std::min(v.size(), o.v.size());
			switch ( n ) {
			case 0:  return false;
			case 1:  return flags::intersects(v.front(), o.v.front());
			default: return flags::intersects(v.data(), o.v.data(), n);
			}
		}
		
		/// Clears all bits
		void clear() { v.clear(); }
		
		/// Gets the i'th bit
		bool operator() (index i) const {
			if ( el(i) >= v.size() ) return false;
			return v.size() == 1 ? get(v.front(), i) : get(v.data(), i);
		}
		
		/// Sets the i'th bit true
		vector& operator|= (index i) {
			index j = el(i);
			if ( j >= v.size() ) { v.resize(j+1, 0); }
			set(v[j], i);
			return *this;
		}
		
		/// Sets the i'th bit false
		vector& operator-= (index i) {
			index j = el(i);
			if ( j < v.size() ) { unset(v[j], i); } 
			return *this;
		}
		
		/// Flips the i'th bit
		vector& operator^= (index i) {
			index j = el(i);
			if ( j >= v.size() ) { v.resize(j+1, 0); }
			flip(v[j], i);
			return *this;
		}
		
		/// Unions another vector with this one
		vector& operator|= (const vector& o) {
			index n = std::min(v.size(), o.v.size());
			switch ( n ) {
			case 0:  break;
			case 1:  set_union(v.front(), o.v.front(), v.front()); break;
			default: set_union(v.data(), o.v.data(), v.data(), n); break;
			}
			return *this;
		}
		
		/// Creates a new vector with the union of the two vectors
		vector operator| (const vector& o) const {
			index n, m;
			const vector* big;
			if ( v.size() < o.v.size() ) {
				n = v.size();
				m = o.v.size();
				big = &o;
			} else {
				n = o.v.size();
				m = v.size();
				big = this;
			}
			std::vector<uint64_t> d{m};
			switch ( n ) {
			case 0:  break;
			case 1:  set_union(v.front(), o.v.front(), d.front()); break;
			default: set_union(v.data(), o.v.data(), d.data(), n); break;
			}
			for (index i = n; i < m; ++i) { d[i] = big->v[i]; }
			return vector{std::move(d)};
		}
		
		/// Intersects another vector with this one
		vector& operator&= (const vector& o) {
			index n = std::min(v.size(), o.v.size());
			switch ( n ) {
			case 0:  clear(); return *this;
			case 1:  set_intersection(v.front(), o.v.front(), v.front()); break;
			default: set_intersection(v.data(), o.v.data(), v.data(), n); break;
			}
			if ( n < v.size() ) { flags::clear(v.data()+n, v.size()-n); }
			return *this;
		}
		
		/// Creates a new vector with the intersection of the the two vectors
		vector operator& (const vector& o) const {
			index n = std::min(v.size(), o.v.size());
			std::vector<uint64_t> d{n};
			switch ( n ) {
			case 0:  break;
			case 1:  set_intersection(v.front(), o.v.front(), d.front()); break;
			default: set_intersection(v.data(), o.v.data(), d.data(), n); break;
			}
			return vector{std::move(d)};
		}
		
		/// Removes all the elements of another vector from this one
		vector& operator-= (const vector& o) {
			index n = std::min(v.size(), o.v.size());
			switch ( n ) {
			case 0:  break;
			case 1:  set_difference(v.front(), o.v.front(), v.front()); break;
			default: set_difference(v.data(), o.v.data(), v.data(), n); break;
			}
			return *this;
		}
		
		/// Creates a new vector with the set difference of this vector and another
		vector operator- (const vector& o) const {
			index n = std::min(v.size(), o.v.size());
			std::vector<uint64_t> d{v.size()};
			switch ( n ) {
			case 0:  break;
			case 1:  set_difference(v.front(), o.v.front(), d.front()); break;
			default: set_difference(v.data(), o.v.data(), d.data(), n); break;
			}
			for (index i = n; i < v.size(); ++i) { d[i] = v[i]; }
			return vector{std::move(d)};
		}
		
		/// Shifts the elements of this vector left by the specified number of bits
		vector& operator<<= (index i) {
			if ( i == 0 ) return *this;
			v.resize(v.size() + el(i) + 1, 0);
			lsh(v.data(), i, v.data(), v.size());
			return *this;
		}
		
		/// Creates a new vector as a copy of this one shifted left by the specified number of bits
		vector operator<< (index i) const {
			if ( i == 0 || v.size() == 0 ) return vector{*this};
			std::vector<uint64_t> d{v.size() + el(i) + 1};
			lsh(v.data(), i, d.data(), v.size());
			return vector{std::move(d)};
		}
		
	private:
		std::vector<uint64_t> v;
	};
}
