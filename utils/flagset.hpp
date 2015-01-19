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
#include <set>

namespace flags {

	using index = uint64_t;

	/// A set of indices. Essentially a set of operator overloads to give std::set an interface 
	/// consistent with flags::vector.
	class set {
		/// Data constructor; sets the storage set directly
		set(std::set<uint64_t>&& d) : s{std::move(d)} {}
	public:

		/// Default constructor; creates empty set
		set() = default;

		set(const set& o) = default;
		set(set&& o) = default;
		set& operator= (const set& o) = default;
		set& operator= (set&& o) = default;

		void swap(set& o) { s.swap(o.s); }

		/// Creates a singleton vector, with the single index set
		static set of(index n) { return set{std::set<uint64_t>{n}}; }

		/// Gets the index of the first bit set (-1 for none such)
		index first() const { return s.empty() ? -1 : *s.begin(); }

		/// Gets the index of the next bit set after i (-1 for none such)
		index next(index i) const {
			auto it = s.upper_bound(i);
			return it == s.end() ? -1 : *it;
		}

		/// Gets the index of the last bit set (-1 for none such)
		index last() const { return s.empty() ? -1 : *s.rbegin(); }

		using const_iterator = std::set<uint64_t>::const_iterator;
		using iterator = const_iterator;

		const_iterator begin() const { return s.cbegin(); }
		const_iterator end() const { return s.cend(); }
		const_iterator cbegin() const { return s.cbegin(); }
		const_iterator cend() const { return s.cend(); }

		/// Gets the count of bits set
		index count() const { return s.size(); }

		/// Checks if all flags are zeroed
		bool empty() const { return s.empty(); }

		/// Checks if this intersects another vector
		bool intersects(const set& o) const {
			auto it = begin(), jt = o.begin();
			if ( it == end() || jt == o.end() ) return false;
			index i = *it, j = *jt;
			do {
				if ( i == j ) return true;
				while ( i < j ) {
					++it;
					if ( it == end() ) return false;
					i = *it;
				}
				while ( i > j ) {
					++jt;
					if ( jt == o.end() ) return false;
					j = *jt;
				}
			} while (true);
		}

		/// Clears all bits
		void clear() { s.clear(); }

		/// Gets the i'th bit
		bool operator() (index i) const { return s.count(i) == 1; }

		/// Sets the i'th bit true
		set& operator|= (index i) {
			s.emplace(i);
			return *this;
		}

		/// Sets the i'th bit false
		set& operator-= (index i) {
			s.erase(i);
			return *this;
		}

		/// Flips the i'th bit
		set& operator^= (index i) {
			auto r = s.emplace(i);
			if ( r.second /* element already present */ ) s.erase(r.first /* present element */);
			return *this;
		}

		/// Unions another vector with this one
		set& operator|= (const set& o) {
			s.insert(o.s.begin(), o.s.end());
			return *this;
		}

		/// Creates a new vector with the union of the two vectors
		set operator| (const set& o) const {
			set d{*this};
			return d |= o;
		}

		/// Intersects another vector with this one
		set& operator&= (const set& o) {
			if ( o.empty() ) { clear(); return *this; }

			auto it = s.begin();
			auto jt = o.s.begin();
			while ( it != s.end() ) {
				index i = *it, j = *jt;
				if ( i == j ) { ++it; ++jt; }
				else if ( i < j ) { it = s.erase(it); }
				else /* if ( i > j ) */ { ++jt; }

				if ( jt == o.s.end() ) it = s.erase(it, s.end());
			}

			return *this;
		}

		/// Creates a new vector with the intersection of the the two vectors
		set operator& (const set& o) const {
			std::set<uint64_t> d;
			
			auto it = s.begin(), jt = o.s.begin();
			while ( it != s.end() && jt != o.s.end() ) {
				index i = *it, j = *jt;
				if ( i == j ) { d.emplace(i); ++it; ++jt; }
				else if ( i < j ) { ++it; }
				else /* if ( i > j ) */ { ++jt; }
			}			

			return set{std::move(d)};
		}

		/// Removes all the elements of another vector from this one
		set& operator-= (const set& o) {
			if ( empty() || o.empty() ) { return *this; }

			auto it = s.begin();
			auto jt = o.s.begin();
			do {
				index i = *it, j = *jt;
				if ( i == j ) { it = s.erase(it); ++jt; }
				else if ( i < j ) { ++it; }
				else /* if ( i > j ) */ { ++jt; }
			} while ( it != s.end() && jt != o.s.end() );
			
			return *this;
		}

		/// Creates a new vector with the set difference of this vector and another
		set operator- (const set& o) const {
			if ( empty() ) return set{};
			if ( o.empty() ) return set{*this};
			std::set<uint64_t> d;

			auto it = s.begin(), jt = o.s.begin();
			do {
				index i = *it, j = *jt;
				if ( i == j ) { ++it; ++jt; }
				else if ( i < j ) { d.emplace(i); ++it; }
				else /* if ( i > j ) */ { ++jt; }

				if ( jt == o.s.end() ) { d.insert(it, s.end()); }
			} while ( jt != o.s.end() && it != s.end() );

			return set{std::move(d)};
		}

		/// Shifts the elements of this set lower by the specified index
		set& operator<<= (index i) {
			if ( i == 0 || s.empty() ) return *this;
			
			std::set<uint64_t> d;
			auto it = s.begin();
			while ( it != s.end() && *it < i ) { ++it; }
			while ( it != s.end() ) {
				d.emplace(*it - i);
				++it;
			}
			s.swap(d);

			return *this;
		}

		/// Creates a new set as a copy of this one shifted lower by the specified index
		set operator<< (index i) const {
			if ( i == 0 || s.empty() ) return set{*this};

			std::set<uint64_t> d;
			auto it = s.begin();
			while ( it != s.end() && *it < i ) { ++it; }
			while ( it != s.end() ) {
				d.emplace(*it - i);
				++it;
			}

			return set{std::move(d)};
		}

		/// Shifts the elements of this set higher by the specified index
		set& operator>>= (index i) {
			if ( i == 0 || s.empty() ) return *this;

			std::set<uint64_t> d;
			auto it = s.begin();
			do {
				d.emplace(*it + i);
				++it;
			} while ( it != s.end() );
			s.swap(d);

			return *this;
		}

		/// Creates a new set as a copy of this one shifted higher by the specified index
		set operator>> (index i) const {
			if ( i == 0 || s.empty() ) return set{*this};

			std::set<uint64_t> d;
			auto it = s.begin();
			do {
				d.emplace(*it + i);
				++it;
			} while ( it != s.end() );

			return set{std::move(d)};
		}

	private:
		std::set<uint64_t> s;
	};
}
