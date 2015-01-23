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
#include <iterator>
#include <utility>

#include "flags.hpp"

namespace flags {

	using index = uint64_t;

	class node;
		
	/// Memory manager for T objects; current version is just a giant memory leak.
	class mem_mgr {
	public:
		node* make();
		void acquire(node* n);
		void release(node* n, index l);
	}; // mem_mgr
	extern mem_mgr mem;
	
	/// Either a node pointer or a 64-bit bitvector
	union nptr {
		/// Constructor from node*
		static nptr of(node* p) { nptr n; n.ptr = p; return n; }
		/// Constructor from bits
		static nptr of(uint64_t x) { nptr n; n.bits = x; return n; }
		
		/// True if value is zeroed
		bool empty() const { return ptr == nullptr; }

		bool operator== (const nptr& o) const { return bits == o.bits; }
		bool operator!= (const nptr& o) const { return bits != o.bits; }
		
		node* ptr;
		uint64_t bits;
	};
		
	/// A trie node
	struct node {
		node() = default;
		node(const node&) = default;
		node(node&&) = default;
		
		node& operator= (const node&) = default;
		node& operator= (node&&) = default;
		
		/// Constructor from 8-array of nptrs (will return nullptr if all empty)
		static nptr of(nptr* a);
		
		/// Clones this node with p placed in the i'th slot of the clone
		nptr set(index i, nptr p) const;
		
		/// True if all slots but 0 are empty
		bool only_first() const;
		
		/// True if all slots but 0 and i are empty
		bool only_first_and(index i) const;
		
		/// True if all slots but i are empty
		bool only(index i) const;
		
		nptr a[8];  ///< Children of this node
	}; // node

	/// A set of indices. Implemented as an octary trie with bitvectors for the bottom level and a 
	/// public interface consistent with flags::vector.
	class trie {
		/// Gets the number of bits covered by level l of the trie
		static index bits(index l);
		
		/// Gets the number of values contained in the range of an l-level trie
		static index levelsize(index l);
		
		/// Gets the trie level needed to store an index
		static index levelof(index i);
		
		/// Gets the array index of i at level l > 0 of the trie
		static index el(index i, index l);
		
		/// Gets the leftmost subtrie of this l-level trie, rooted no higher than level l
		static nptr leftat(nptr p, index m, index l);
		
		/// Returns an l-level trie containing only index i
		static nptr of(index i, index l);
		
		/// Gets the index of the first bit set in this trie (-1 for none such)
		static index first(nptr p, index l);
		
		/// Gets the index of the next bit set after i in this trie (-1 for none such)
		static index next(nptr p, index i, index l);
		
		/// Gets the index of the last bit set in this trie (-1 for none such)
		static index last(nptr p, index l);
		
		/// Counts the bits set in the trie
		static index count(nptr p, index l);
		
		/// true if the i-bit in the l-level trie is set
		static bool get(nptr p, index i, index l);
		
		/// returns a copy of the l-level trie p with bit i set
		static nptr set(nptr p, index i, index l);
		
		/// returns a copy of the l-level trie p with bit i unset
		static nptr unset(nptr p, index i, index l);
		
		/// Checks if two l-level tries intersect
		static bool intersects(nptr p, nptr q, index l);
		
		/// Takes the union of two l-level tries
		static nptr set_union(nptr p, nptr q, index l);
		
		/// Takes the union of an l-level trie p with a shorter m-level trie q
		static nptr set_union(nptr p, nptr q, index l, index m);
		
		/// Takes the intersection of two l-level tries
		static nptr set_intersection(nptr p, nptr q, index l);
		
		/// Removes the elements of the second l-level trie from the first
		static nptr set_difference(nptr p, nptr q, index l);
		
		/// Removes the elements of the second (m-level) trie from the first (l-level) trie
		static nptr set_difference(nptr p, nptr q, index l, index m);
		
		/// Returns the l-level trie shifted right by i bits (as a pair with its successor)
		static std::pair<nptr, nptr> rsh(nptr p, index i, index l);
		
		/// Data constructor; sets the storage set directly
		trie(nptr p, index l) : p(p), l(l) {}
	public:

		/// Default constructor; creates empty set
		trie() : p(nptr::of(nullptr)), l(0) {};

		trie(const trie& o) = default;
		trie(trie&& o) = default;
		trie& operator= (const trie& o) = default;
		trie& operator= (trie&& o) = default;

		void swap(trie& o) {
			nptr pt = p; p = o.p; o.p = pt;
			index lt = l; l = o.l; o.l = lt;
		}

		/// Creates a singleton trie, with the single index set
		static trie of(index n) {
			index l = trie::levelof(n);
			return trie{trie::of(n, l), l};
		}

		/// Gets the index of the first bit set (-1 for none such)
		index first() const { return trie::first(p, l); }

		/// Gets the index of the next bit set after i (-1 for none such)
		index next(index i) const { return trie::next(p, i, l); }

		/// Gets the index of the last bit set (-1 for none such)
		index last() const { return trie::last(p, l); }

		/// Non-modifying iterator built on first() and next()
		class const_iterator : public std::iterator<std::forward_iterator_tag, index> {
		friend trie;
			const_iterator(const trie& t) : t{t}, crnt{t.first()} {}
			const_iterator(const trie& t, index crnt) : t{t}, crnt{crnt} {}
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
			const trie& t;  ///< Trie iterated
			index crnt;     ///< Current index
		}; // const_iterator

		using iterator = const_iterator;

		const_iterator begin() const { return const_iterator(*this); }
		const_iterator end() const { return const_iterator(*this, -1); }
		const_iterator cbegin() const { return const_iterator(*this); }
		const_iterator cend() const { return const_iterator(*this, -1); }

		/// Gets the count of bits set
		index count() const { return trie::count(p, l); }

		/// Checks if all flags are zeroed
		bool empty() const { return p.empty(); }

		/// Checks if this intersects another trie
		bool intersects(const trie& o) const {
			nptr a = trie::leftat(p, o.l, l);
			nptr b = trie::leftat(o.p, l, o.l);
			return trie::intersects(a, b, std::min(l, o.l));
		}

		/// Clears all bits
		void clear() { p = nptr::of(nullptr); l = 0; }

		/// Gets the i'th bit
		bool operator() (index i) const {
			return trie::levelof(i) > l ? false : trie::get(p, i, l);
		}

		/// Sets the i'th bit true
		trie& operator|= (index i) {
			index li = trie::levelof(i);
			if ( li <= l ) {
				// inserted value within current range, set
				p = trie::set(p, i, l);
			} else {
				// extend range upward
				do {
					node* n = mem.make();
					n->a[0] = p;
					p = nptr::of(n); ++l;
				} while ( li > l );
				// set new value in
				assert(trie::el(i,l) != 0 && "added value can't be in 0-limb, or its level would be lower");
				p.ptr->a[trie::el(i, l)] = trie::of(i, l-1);
			}
			return *this;
		}

		/// Sets the i'th bit false
		trie& operator-= (index i) {
			// can't remove out-of-range value
			if ( levelof(i) > l ) return *this;
			// flip bits in base case
			if ( l == 0 ) {
				flags::unset(p.bits, i);
				return *this;
			}
			
			index j = trie::el(i, l);
			nptr q = trie::unset(p.ptr->a[j], i, l-1);
			
			// no change in case where bit wasn't present
			if ( q == p.ptr->a[j] ) return *this;
			
			// trim down tree if took last far bit
			if ( q.empty() && p.ptr->only_first_and(j) ) {
				do {
					p = p.ptr->a[0]; --l;
				} while ( l > 0 && p.ptr->only_first() );
				
				return *this;
			}
			
			// set replacement into tree
			p = p.ptr->set(j, q);
			return *this;
		}

		/// Unions another trie with this one
		trie& operator|= (const trie& o) {
			if ( o.p.empty() ) return *this;
			if ( p.empty() ) return *this = o;
			
			if ( l >= o.l ) {
				p = trie::set_union(p, o.p, l, o.l);
			} else {
				p = trie::set_union(o.p, p, o.l, l);
				l = o.l;
			}
			return *this;
		}

		/// Creates a new trie with the union of the two tries
		trie operator| (const trie& o) const {
			if ( o.p.empty() ) return trie{*this};
			if ( p.empty() ) return trie{o};
			
			if ( l >= o.l ) return trie{trie::set_union(p, o.p, l, o.l), l};
			else return trie{trie::set_union(o.p, p, o.l, l), o.l};
		}

		/// Intersects another trie with this one
		trie& operator&= (const trie& o) {
			if ( p.empty() ) return *this;
			if ( o.p.empty() ) { clear(); return *this; }

			nptr a = trie::leftat(p, o.l, l);
			nptr b = trie::leftat(o.p, l, o.l);
			l = std::min(l, o.l);
			p = trie::set_intersection(a, b, l);
			
			return *this;
		}

		/// Creates a new trie with the intersection of the the two tries
		trie operator& (const trie& o) const {
			if ( p.empty() || o.p.empty() ) return trie{};
			
			nptr a = trie::leftat(p, o.l, l);
			nptr b = trie::leftat(o.p, l, o.l);
			index m = std::min(l, o.l);
			return trie{trie::set_intersection(a, b, m), m};
		}

		/// Removes all the elements of another trie from this one
		trie& operator-= (const trie& o) {
			if ( p.empty() || o.p.empty() ) { return *this; }
			
			p = ( o.l > l ) ?
				trie::set_difference(p, trie::leftat(o.p, l, o.l), l) :
				trie::set_difference(p, o.p, l, o.l);
			
			if ( p.empty() ) { l = 0; return *this; }
			
			while ( l > 0 && p.ptr->only_first() ) {
				p = p.ptr->a[0];
				--l;
			}
			return *this;
		}

		/// Creates a new trie with the set difference of this trie and another
		trie operator- (const trie& o) const {
			if ( p.empty() ) return trie{};
			if ( o.p.empty() ) return trie{*this};
			
			nptr r = ( o.l > l ) ?
				trie::set_difference(p, trie::leftat(o.p, l, o.l), l) :
				trie::set_difference(p, o.p, l, o.l);
			
			if ( r.empty() ) return trie{};
			if ( r == p ) return trie{*this};
			
			index m = l;
			while ( m > 0 && r.ptr->only_first() ) {
				r = r.ptr->a[0];
				--m;
			}
			return trie{r, m};
		}
		
		/// Shifts the elements of this trie higher by the specified index
		trie& operator>>= (index i) {
			if ( i == 0 || p.empty() ) return *this;
			
			// right shift the trie internally
			auto p2 = trie::rsh(p, i & (levelsize(l)-1), l);
			// handle any higher levels the right shift induces
			i >>= bits(l);
			while ( i > 0 ) {
				index bs = i & 7;  // amount to shift new limbs by
				
				if ( bs == 7 ) {
					if ( ! p2.first.empty() ) {
						node* n = mem.make();
						n->a[7] = p2.first;
						p2.first = nptr::of(n);
					}
					if ( ! p2.second.empty() ) {
						node* m = mem.make();
						m->a[0] = p2.second;
						p2.second = nptr::of(m);
					}
				} else {
					node* n = mem.make();
					n->a[bs] = p2.first;
					n->a[bs+1] = p2.second;
					p2.first = nptr::of(n);
					p2.second = nptr::of(nullptr);
				}
				
				i >>= 3;  // take off next level of i, add it to l
				++l;
			}
			// handle spillover from non-empty second limb
			if ( p2.second.empty() ) {
				p = p2.first;
			} else {
				node* n = mem.make();
				n->a[0] = p2.first;
				n->a[1] = p2.second;
				p = nptr::of(n);
				++l;
			}
			
			return *this;
		}

		/// Creates a new trie as a copy of this one shifted higher by the specified index
		trie operator>> (index i) const {
			if ( i == 0 || p.empty() ) return trie{*this};

			// right shift the trie internally
			auto p2 = trie::rsh(p, i & (levelsize(l)-1), l);
			// handle any higher levels the right shift induces
			i >>= bits(l);
			index m = l;
			while ( i > 0 ) {
				index bs = i & 7;  // amount to shift new limbs by
				
				if ( bs == 7 ) {
					if ( ! p2.first.empty() ) {
						node* n = mem.make();
						n->a[7] = p2.first;
						p2.first = nptr::of(n);
					}
					if ( ! p2.second.empty() ) {
						node* m = mem.make();
						m->a[0] = p2.second;
						p2.second = nptr::of(m);
					}
				} else {
					node* n = mem.make();
					n->a[bs] = p2.first;
					n->a[bs+1] = p2.second;
					p2.first = nptr::of(n);
					p2.second = nptr::of(nullptr);
				}
				
				i >>= 3;  // take off next level of i, add it to l
				++m;
			}
			// handle spillover from non-empty second limb
			if ( p2.second.empty() ) {
				return trie{p2.first, m};
			} else {
				node* n = mem.make();
				n->a[0] = p2.first;
				n->a[1] = p2.second;
				return trie{nptr::of(n), m+1};
			}
		}

	private:
		nptr p;   ///< root pointer
		index l;  ///< number of trie levels under the root pointer
	};

}
