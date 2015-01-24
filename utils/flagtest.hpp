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

//uncomment to disable asserts
//#define NDEBUG
#include <cassert>

#include <cstdint>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <utility>

#include "flags.hpp"

#include "flagvector.hpp"
#include "flagtrie.hpp"

namespace flags {

	/// A set of indices. Implemented as an octary trie with bitvectors for the bottom level and a 
	/// public interface consistent with flags::vector.
	class test {
		bool match() const {
			auto tt = t.begin();
			auto vt = v.begin();
			while ( tt != t.end() && vt != v.end() ) {
				if ( *tt != *vt ) return false;
				++tt; ++vt;
			}
			return ( tt == t.end() && vt == v.end() );
		}

		std::string print_t() const {
			std::stringstream ss;
			ss << "[";
			for (index i : t) ss << " " << i;
			ss << " ]";
			return ss.str();
		}
		
		std::string print_v() const {
			std::stringstream ss;
			ss << "[";
			for (index i : v) ss << " " << i;
			ss << " ]";
			return ss.str();
		}
		
		/// Data constructor; sets the storage set directly
		test(trie t, vector v) : t(t), v(v) {}
	public:

		/// Default constructor; creates empty set
		test() : t(), v() {};

		test(const test& o) = default;
		test(test&& o) = default;
		test& operator= (const test& o) = default;
		test& operator= (test&& o) = default;

		void swap(test& o) { t.swap(o.t); v.swap(o.v); }

		/// Creates a singleton trie, with the single index set
		static test of(index n) {
			test t{trie::of(n), vector::of(n)};
			if ( ! t.match() ) {
				std::cerr << "FAIL: of " << n << " " << t.print_t() << " != " << t.print_v() << std::endl;
				abort();
			}
			return t;
		}

		/// Gets the index of the first bit set (-1 for none such)
		index first() const {
			index ta = t.first(), va = v.first();
			if ( ta != va ) {
				std::cerr << "FAIL: first " << print_v() << " " << ta << " != " << va << std::endl;
				abort();
			}
			return ta;
		}

		/// Gets the index of the next bit set after i (-1 for none such)
		index next(index i) const {
			index ta = t.next(i), va = v.next(i);
			if ( ta != va ) {
				std::cerr << "FAIL: next " << i << " " << print_v() << " " << ta << " != " << va << std::endl;
				abort();
			}
			return ta;
		}

		/// Gets the index of the last bit set (-1 for none such)
		index last() const {
			index ta = t.last(), va = v.last();
			if ( ta != va ) {
				std::cerr << "FAIL: last " << print_v() << " " << ta << " != " << va << std::endl;
				abort();
			}
			return ta;
			
		}

		/// Non-modifying iterator built on first() and next()
		class const_iterator : public std::iterator<std::forward_iterator_tag, index> {
		friend test;
			const_iterator(const test& t) : t{t}, crnt{t.first()} {}
			const_iterator(const test& t, index crnt) : t{t}, crnt{crnt} {}
		public:
			const_iterator(const const_iterator&) = default;
			const_iterator(const_iterator&&) = default;

			index operator* () { return crnt; }

			const_iterator& operator++ () {
				if ( crnt != -1 ) { crnt = t.next(crnt); }
				return *this;
			}

			const_iterator operator++ (int) {
				const_iterator tmp = *this;
				++(*this);
				return tmp;
			}

			bool operator== (const const_iterator& o) {
				return &t == &o.t && crnt == o.crnt;
			}

			bool operator!= (const const_iterator& o) { return ! (*this == o); }
		private:
			const test& t;  ///< Trie iterated
			index crnt;     ///< Current index
		}; // const_iterator

		using iterator = const_iterator;

		const_iterator begin() const { return const_iterator(*this); }
		const_iterator end() const { return const_iterator(*this, -1); }
		const_iterator cbegin() const { return const_iterator(*this); }
		const_iterator cend() const { return const_iterator(*this, -1); }

		/// Gets the count of bits set
		index count() const {
			index ta = t.count(), va = v.count();
			if ( ta != va ) {
				std::cerr << "FAIL: count " << print_v() << " " << ta << " != " << va << std::endl;
				abort();
			}
			return ta;
		}

		/// Checks if all flags are zeroed
		bool empty() const {
			bool ta = t.empty(), va = v.empty();
			if ( ta != va ) {
				std::cerr << "FAIL: empty " << print_v() << " " << ta << " != " << va << std::endl;
				abort();
			}
			return ta;
		}

		/// Checks if this intersects another trie
		bool intersects(const test& o) const {
			bool ta = t.intersects(o.t), va = v.intersects(o.v);
			if ( ta != va ) {
				std::cerr << "FAIL: intersects " << print_v() << " " << o.print_v() << " " << ta << " != " << va << std::endl;
				abort();
			}
			return ta;
		}

		/// Clears all bits
		void clear() {
			t.clear(); v.clear();
			if ( ! match() ) {
				std::cerr << "FAIL: clear " << print_t() << " " << print_v() << std::endl;
				abort();
			}
		}

		/// Gets the i'th bit
		bool operator() (index i) const {
			bool ta = t(i), va = v(i);
			if ( ta != va ) {
				std::cerr << "FAIL: get " << i << " " << print_v() << " " << ta << " != " << va << std::endl;
				abort();
			}
			return ta;
		}

		/// Sets the i'th bit true
		test& operator|= (index i) {
			test bak{*this};
			t |= i; v |= i;
			if ( ! match() ) {
				std::cerr << "FAIL: set " << i << " " << bak.print_v() << " " << print_t() << " != " << print_v() << std::endl;
				abort();
			}
			return *this;
		}

		/// Sets the i'th bit false
		test& operator-= (index i) {
			test bak{*this};
			t -= i; v -= i;
			if ( ! match() ) {
				std::cerr << "FAIL: unset " << i << " " << bak.print_v() << " " << print_t() << " != " << print_v() << std::endl;
				abort();
			}
			return *this;
		}

		/// Unions another trie with this one
		test& operator|= (const test& o) {
			test bak{*this};
			t |= o.t; v |= o.v;
			if ( ! match() ) {
				std::cerr << "FAIL: union-assign " << bak.print_v() << " " << o.print_v() << " " << print_t() << " != " << print_v() << std::endl;
				abort();
			}
			return *this;
		}

		/// Creates a new trie with the union of the two tries
		test operator| (const test& o) const {
			test r{t | o.t, v | o.v};
			if ( ! r.match() ) {
				std::cerr << "FAIL: union " << print_v() << " " << o.print_v() << " " << r.print_t() << " != " << r.print_v() << std::endl;
				abort();
			}
			return r;
		}

		/// Intersects another trie with this one
		test& operator&= (const test& o) {
			test bak{*this};
			t &= o.t; v &= o.v;
			if ( ! match() ) {
				std::cerr << "FAIL: intersect-assign " << bak.print_v() << " " << o.print_v() << " " << print_t() << " != " << print_v() << std::endl;
				abort();
			}
			return *this;
		}

		/// Creates a new trie with the intersection of the the two tries
		test operator& (const test& o) const {
			test r{t & o.t, v & o.v};
			if ( ! r.match() ) {
				std::cerr << "FAIL: intersect " << print_v() << " " << o.print_v() << " " << r.print_t() << " != " << r.print_v() << std::endl;
				abort();
			}
			return r;
		}

		/// Removes all the elements of another trie from this one
		test& operator-= (const test& o) {
			test bak{*this};
			t -= o.t; v -= o.v;
			if ( ! match() ) {
				std::cerr << "FAIL: subtract-assign " << bak.print_v() << " " << o.print_v() << " " << print_t() << " != " << print_v() << std::endl;
				abort();
			}
			return *this;
		}

		/// Creates a new trie with the set difference of this trie and another
		test operator- (const test& o) const {
			test r{t - o.t, v - o.v};
			if ( ! r.match() ) {
				std::cerr << "FAIL: subtract " << print_v() << " " << o.print_v() << " " << r.print_t() << " != " << r.print_v() << std::endl;
				abort();
			}
			return r;
		}
		
		/// Shifts the elements of this trie higher by the specified index
		test& operator>>= (index i) {
			test bak{*this};
			t >>= i; v >>= i;
			if ( ! match() ) {
				std::cerr << "FAIL: rsh-assign " << i << " " << bak.print_v() << " " << print_t() << " != " << print_v() << std::endl;
				abort();
			}
			return *this;
		}

		/// Creates a new trie as a copy of this one shifted higher by the specified index
		test operator>> (index i) const {
			test r{t >> i, v >> i};
			if ( ! r.match() ) {
				std::cerr << "FAIL: rsh " << i << " " << print_v() << " " << r.print_t() << " != " << r.print_v() << std::endl;
				abort();
			}
			return r;
		}

	private:
		trie t;
		vector v;
	};

}
