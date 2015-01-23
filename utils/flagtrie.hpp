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
#include <utility>

#include "flags.hpp"

namespace flags {

	using index = uint64_t;

	/// A set of indices. Implemented as an octary trie with bitvectors for the bottom level and a 
	/// public interface consistent with flags::vector.
	class trie {
		class node;
		
		/// Memory manager for T objects; current version is just a giant memory leak.
		class mem_mgr {
		public:
			node* make() { return new node; }
			void acquire(node* n) { /* do nothing */ }
			void release(node* n, index l) { /* do nothing */ }
		}; // mem_mgr
		static mem_mgr* mem;
		
		/// Gets memory manager
		static inline mem_mgr& get_mem() {
			if ( mem == nullptr ) { mem = new mem_mgr; }
			return *mem;
		}

		/// Makes a new node
		static node* mem_make() { return get_mem().make(); }
		
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
			static nptr of(nptr* a) {
				node* n = nullptr;
				bool is_mt = true;
				for (index i = 0; i < 8; ++i) {
					if ( ! a[i].empty() ) {
						if ( is_mt ) { n = trie::mem_make(); is_mt = false; }
						n->a[i] = a[i];
					}
				}
				return nptr::of(n);
			}
			
			/// Clones this node with p placed in the i'th slot of the clone
			nptr set(index i, nptr p) {
				node* n = trie::mem_make();
				index j = 0;
				while ( j < i ) { n->a[j] = a[j]; ++j; }
				n->a[i] = p; ++j;
				while ( j < 8 ) { n->a[j] = a[j]; ++j; }
				return nptr::of(n);
			}
			
			/// True if all slots but 0 are empty
			bool only_first() {
				for (index j = 1; j < 8; ++j) { if ( ! a[j].empty() ) return false; }
				return true;
			}
			
			/// True if all slots but 0 and i are empty
			bool only_first_and(index i) {
				for (index j = 1; j < 8; ++j) {
					if ( j == i ) continue;
					if ( ! a[j].empty() ) return false;
				}
				return true;
			}
			
			/// True if all slots but i are empty
			bool only(index i) {
				for (index j = 0; j < 8; ++j) {
					if ( j == i ) continue;
					if ( ! a[j].empty() ) return false;
				}
				return true;
			}
			
			nptr a[8];  ///< Children of this node
		}; // node
		
		/// Gets the number of bits covered by level l of the trie
		static constexpr index bits(index l) { return l*3 + 6; }
		
		/// Gets the number of values contained in the range of an l-level trie
		static constexpr index levelsize(index l) { return UINT64_C(1) << bits(l); }
		
		/// Gets the trie level needed to store an index
		static index levelof(index i) {
			if ( i < 64 ) return 0;     // 2^6
			if ( i < 512 ) return 1;    // 2^9 == 2^(6+3)
			if ( i < 4096 ) return 2;   // 2^12 == 2^(6+6)
			if ( i < 32768 ) return 3;  // 2^15 == 2^(6+9)
			
			index l = 4;
			while ( i >= 262144 ) { ++l; i >>= 3; }  // while i >= 2^18, i /= 8
			return l;
		}
		
		/// Gets the array index of i at level l > 0 of the trie
		static constexpr index el(index i, index l) { return (i >> bits(l-1)) & UINT64_C(7); }
		
		/// Gets the leftmost subtrie of this l-level trie, rooted no higher than level l
		static nptr leftat(nptr p, index m, index l) {
			while ( l > m && ! p.empty() ) {
				p = p.ptr->a[0];
				--l;
			}
			return p;
		}
		
		/// Returns an l-level trie containing only index i
		static nptr of(index i, index l) {
			nptr p = nptr::of( flags::mask(i & UINT64_C(63)) );
			if ( l == 0 ) return p;
			
			i >>= 6;
			do {
				node* n = trie::mem_make();
				n->a[i & UINT64_C(7)] = p;
				p = nptr::of(n);
				
				i >>= 3; --l;
			} while ( l > 0 );
			return p;
		}
		
		/// Gets the index of the first bit set in this trie (-1 for none such)
		static index first(nptr p, index l) {
			if ( p.empty() ) return -1;
			
			index a = 0;
			while ( l > 0 ) {
				index i = 0;
				do {
					if ( ! p.ptr->a[i].empty() ) break;
					++i;
				} while ( i < 7 );
				assert((i < 7 || ! p.ptr->a[i].empty()) && "non-empty node must have non-empty child");
				
				--l;
				a += levelsize(l)*i;
				p = p.ptr->a[i];
			}
			return a + flags::first(p.bits);
		}
		
		/// Gets the index of the next bit set after i in this trie (-1 for none such)
		static index next(nptr p, index i, index l) {
			if ( p.empty() ) return -1;
			if ( l == 0 ) return flags::next(p.bits, i);
			
			index j = el(i, l);
			index r = next(p.ptr->a[j], i, l-1);
			if ( r != -1 ) return levelsize(l-1)*j + r;
			
			++j;
			while ( j < 8 ) {
				if ( ! p.ptr->a[j].empty() ) {
					return levelsize(l-1)*j + first(p.ptr->a[j], l-1);
				}
				++j;
			}
			return -1;
		}
		
		/// Gets the index of the last bit set in this trie (-1 for none such)
		static index last(nptr p, index l) {
			if ( p.empty() ) return -1;
			
			index a = 0;
			while ( l > 0 ) {
				index i = 7;
				do {
					if ( ! p.ptr->a[i].empty() ) break;
					--i;
				} while ( i > 0 );
				assert((i > 0 || ! p.ptr->a[i].empty()) && "non-empty node must have non-empty child");
				
				--l;
				a += levelsize(l)*i;
				p = p.ptr->a[i];
			}
			return a + flags::last(p.bits);
		}
		
		/// Counts the bits set in the trie
		static index count(nptr p, index l) {
			if ( p.empty() ) return 0;
			if ( l == 0 ) return flags::count(p.bits);
			
			index a = 0;
			for (index i = 0; i < 8; ++i) { a += count(p.ptr->a[i], l-1); }
			return a;
		}
		
		/// true if the i-bit in the l-level trie is set
		static bool get(nptr p, index i, index l) {
			while ( l > 0 && ! p.empty() ) {
				p = p.ptr->a[el(i, l)];
				--l;
			}
			return p.empty() ? false : flags::get(p.bits, i);
		}
		
		/// returns a copy of the l-level trie p with bit i set
		static nptr set(nptr p, index i, index l) {
			if ( l == 0 ) { flags::set(p.bits, i); return p; }
			
			index j = el(i, l);
			if ( p.ptr->a[j].empty() ) return p.ptr->set(j, of(i, l-1));
			nptr q = set(p.ptr->a[j], i, l-1);
			return q == p.ptr->a[j] ? p : p.ptr->set(j, q);
		}
		
		/// returns a copy of the l-level trie p with bit i unset
		static nptr unset(nptr p, index i, index l) {
			// do nothing for empty trie
			if ( p.empty() ) return p;
			// flip bits in base case
			if ( l == 0 ) { flags::unset(p.bits, i); return p; }
			
			index j = trie::el(i, l);
			nptr q = trie::unset(p.ptr->a[j], i, l-1);
			
			// no change in case where bit wasn't present
			if ( q == p.ptr->a[j] ) return p;
			
			// return empty if took last bit
			if ( q.empty() && p.ptr->only(j) ) return q;
			
			// set replacement into tree
			return p.ptr->set(j, q);
		}
		
		/// Checks if two l-level tries intersect
		static bool intersects(nptr p, nptr q, index l) {
			if ( p.empty() || q.empty() ) return false;
			if ( l == 0 ) return flags::intersects(p.bits, q.bits);
			
			for (index i = 0; i < 8; ++i) if ( intersects(p.ptr->a[i], q.ptr->a[i], l-1) ) return true;
			return false;
		}
		
		/// Takes the union of two l-level tries
		static nptr set_union(nptr p, nptr q, index l) {
			if ( p.empty() ) return q;
			if ( p.empty() ) return p;
			if ( l == 0 ) { flags::set_union(p.bits, q.bits, p.bits); return p; }
			
			node* n = mem_make();
			for (index i = 0; i < 8; ++i) { n->a[i] = set_union(p.ptr->a[i], q.ptr->a[i], l-1); }
			return nptr::of(n);
		}
		
		/// Takes the union of an l-level trie p with a shorter m-level trie q
		static nptr set_union(nptr p, nptr q, index l, index m) {
			assert(l >= m);
			if ( l == m ) return set_union(p, q, l);
			
			if ( p.empty() ) { p = nptr::of(mem_make()); }
			return p.ptr->set(0, set_union(p.ptr->a[0], q, l-1, m));
		}
		
		/// Takes the intersection of two l-level tries
		static nptr set_intersection(nptr p, nptr q, index l) {
			if ( p == q ) return p;
			if ( p.empty() || q.empty() ) return nptr::of(nullptr);
			if ( l == 0 ) { flags::set_intersection(p.bits, q.bits, p.bits); return p; }
			
			node* n = nullptr;
			bool is_mt = true;
			for (index i = 0; i < 8; ++i) {
				nptr r = set_intersection(p.ptr->a[i], q.ptr->a[i], l-1);
				if ( ! r.empty() ) {
					if ( is_mt ) { n = mem_make(); is_mt = false; }
					n->a[i] = r;
				}
			}
			return nptr::of(n);
		}
		
		/// Removes the elements of the second l-level trie from the first
		static nptr set_difference(nptr p, nptr q, index l) {
			if ( p == q || p.empty() ) return nptr::of(nullptr);
			if ( q.empty() ) return p;
			if ( l == 0 ) { flags::set_difference(p.bits, q.bits, p.bits); return p; }
			
			node* n = nullptr;
			bool is_mt = true, is_p = true;  // is_mt || is_p => haven't made n
			for (index i = 0; i < 8; ++i) {
				nptr r = set_difference(p.ptr->a[i], q.ptr->a[i], l-1);
				if ( r.empty() ) {
					if ( is_p && ! p.ptr->a[i].empty() ) {
						is_p = false;
						if ( ! is_mt ) {
							n = mem_make();
							for (index j = 0; j < i; ++j) { n->a[j] = p.ptr->a[j]; }
						}
					}
				} else {
					if ( is_mt ) {
						is_mt = false;
						if ( is_p && r != p.ptr->a[i] ) {
							is_p = false;
							n = mem_make();
							n->a[i] = r;
						}
					} else {
						if ( is_p && r != p.ptr->a[i] ) {
							is_p = false;
							n = mem_make();
							for (index j = 0; j < i; ++j) { n->a[j] = p.ptr->a[j]; }
						}
						n->a[i] = r;
					}
				}
			}
			return is_p ? p : nptr::of(n);
		}
		
		/// Removes the elements of the second (m-level) trie from the first (l-level) trie
		static nptr set_difference(nptr p, nptr q, index l, index m) {
			assert(l >= m);
			if ( l == m ) return set_difference(p, q, l);
			if ( p.empty() ) return p;
			
			nptr r = set_difference(p.ptr->a[0], q, l-1, m);
			if ( r == p.ptr->a[0] ) return p;
			if ( r.empty() && p.ptr->only_first() ) return r;
			return p.ptr->set(0, r);
		}
		
		/// Returns the l-level trie shifted right by i bits (as a pair with its successor)
		static std::pair<nptr, nptr> rsh(nptr p, index i, index l) {
			assert ( i < levelsize(l) );
			if ( p.empty() ) return std::make_pair(p, p);
			if ( i == 0 ) return std::make_pair(p, nptr::of(nullptr));
			
			if ( l == 0 ) {
				uint64_t r[2] = { p.bits, UINT64_C(0) };
				flags::rsh(r, i, r, 2);
				return std::make_pair(nptr::of(r[0]), nptr::of(r[1]));
			}
			
			nptr r[16];
			index bs = i >> bits(l-1);
			index is = i & ( levelsize(l-1) - 1 );
			
			for (index j = 15; j > 7+bs+1; --j) { r[j] = nptr::of(nullptr); }
			auto p2 = rsh(p.ptr->a[7], is, l-1);
			r[7+bs] = p2.first; r[7+bs+1] = p2.second;
			index j = 7;
			do {
				--j;
				p2 = rsh(p.ptr->a[j], is, l-1);
				r[j+bs] = p2.first; r[j+bs+1] = set_union(r[j+bs+1], p2.second, l-1);
			} while ( j > 0 );
			
			return std::make_pair(node::of(r), node::of(r+8));
		}
		
		/// Data constructor; sets the storage set directly
		trie(nptr p, index l) : p(p), l(l) {}
	public:

		/// Default constructor; creates empty set
		trie() = default;

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
					node* n = trie::mem_make();
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
						node* n = trie::mem_make();
						n->a[7] = p2.first;
						p2.first = nptr::of(n);
					}
					if ( ! p2.second.empty() ) {
						node* m = trie::mem_make();
						m->a[0] = p2.second;
						p2.second = nptr::of(m);
					}
				} else {
					node* n = trie::mem_make();
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
				node* n = trie::mem_make();
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
						node* n = trie::mem_make();
						n->a[7] = p2.first;
						p2.first = nptr::of(n);
					}
					if ( ! p2.second.empty() ) {
						node* m = trie::mem_make();
						m->a[0] = p2.second;
						p2.second = nptr::of(m);
					}
				} else {
					node* n = trie::mem_make();
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
				node* n = trie::mem_make();
				n->a[0] = p2.first;
				n->a[1] = p2.second;
				return trie{nptr::of(n), m+1};
			}
		}

	private:
		nptr p;   ///< root pointer
		index l;  ///< number of trie levels under the root pointer
	};

	trie::mem = nullptr;
}
