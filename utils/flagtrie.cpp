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

#include "flagtrie.hpp"

#include "flags.hpp"

namespace flags {

	using trie::node;
	using trie::nptr;

	node* trie::mem_mgr::make() { return new node; }
	void trie::mem_mgr::acquire(node* n) { /* do nothing */ }
	void trie::mem_mgr::release(node* n, index l) { /* do nothing */ }
	
	trie::mem = new mem_mgr;
	
	static node* trie::mem_make() { return trie::mem.make(); }

	static nptr trie::node::of(nptr* a) {
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
	
	nptr node::set(index i, nptr p) {
		node* n = trie::mem_make();
		index j = 0;
		while ( j < i ) { n->a[j] = a[j]; ++j; }
		n->a[i] = p; ++j;
		while ( j < 8 ) { n->a[j] = a[j]; ++j; }
		return nptr::of(n);
	}
	
	bool node::only_first() {
		for (index j = 1; j < 8; ++j) { if ( ! a[j].empty() ) return false; }
		return true;
	}
	
	bool node::only_first_and(index i) {
		for (index j = 1; j < 8; ++j) {
			if ( j == i ) continue;
			if ( ! a[j].empty() ) return false;
		}
		return true;
	}
	
	bool node::only(index i) {
		for (index j = 0; j < 8; ++j) {
			if ( j == i ) continue;
			if ( ! a[j].empty() ) return false;
		}
		return true;
	}
	
	static constexpr index trie::bits(index l) { return l*3 + 6; }
		
	static constexpr index trie::levelsize(index l) { return UINT64_C(1) << bits(l); }
	
	static index trie::levelof(index i) {
		if ( i < 64 ) return 0;     // 2^6
		if ( i < 512 ) return 1;    // 2^9 == 2^(6+3)
		if ( i < 4096 ) return 2;   // 2^12 == 2^(6+6)
		if ( i < 32768 ) return 3;  // 2^15 == 2^(6+9)
		
		index l = 4;
		while ( i >= 262144 ) { ++l; i >>= 3; }  // while i >= 2^18, i /= 8
		return l;
	}
	
	static constexpr index trie::el(index i, index l) { return (i >> bits(l-1)) & UINT64_C(7); }
	
	static nptr trie::leftat(nptr p, index m, index l) {
		while ( l > m && ! p.empty() ) {
			p = p.ptr->a[0];
			--l;
		}
		return p;
	}
	
	static nptr trie::of(index i, index l) {
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
	
	static index trie::first(nptr p, index l) {
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
	
	static index trie::next(nptr p, index i, index l) {
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
	
	static index trie::last(nptr p, index l) {
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
	
	static index trie::count(nptr p, index l) {
		if ( p.empty() ) return 0;
		if ( l == 0 ) return flags::count(p.bits);
		
		index a = 0;
		for (index i = 0; i < 8; ++i) { a += count(p.ptr->a[i], l-1); }
		return a;
	}
	
	static bool trie::get(nptr p, index i, index l) {
		while ( l > 0 && ! p.empty() ) {
			p = p.ptr->a[el(i, l)];
			--l;
		}
		return p.empty() ? false : flags::get(p.bits, i);
	}
	
	static nptr trie::set(nptr p, index i, index l) {
		if ( l == 0 ) { flags::set(p.bits, i); return p; }
		
		index j = el(i, l);
		if ( p.ptr->a[j].empty() ) return p.ptr->set(j, of(i, l-1));
		nptr q = set(p.ptr->a[j], i, l-1);
		return q == p.ptr->a[j] ? p : p.ptr->set(j, q);
	}
	
	static nptr trie::unset(nptr p, index i, index l) {
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
	
	static bool trie::intersects(nptr p, nptr q, index l) {
		if ( p.empty() || q.empty() ) return false;
		if ( l == 0 ) return flags::intersects(p.bits, q.bits);
		
		for (index i = 0; i < 8; ++i) if ( intersects(p.ptr->a[i], q.ptr->a[i], l-1) ) return true;
		return false;
	}
	
	static nptr trie::set_union(nptr p, nptr q, index l) {
		if ( p.empty() ) return q;
		if ( p.empty() ) return p;
		if ( l == 0 ) { flags::set_union(p.bits, q.bits, p.bits); return p; }
		
		node* n = mem_make();
		for (index i = 0; i < 8; ++i) { n->a[i] = set_union(p.ptr->a[i], q.ptr->a[i], l-1); }
		return nptr::of(n);
	}
	
	static nptr trie::set_union(nptr p, nptr q, index l, index m) {
		assert(l >= m);
		if ( l == m ) return set_union(p, q, l);
		
		if ( p.empty() ) { p = nptr::of(mem_make()); }
		return p.ptr->set(0, set_union(p.ptr->a[0], q, l-1, m));
	}
	
	static nptr trie::set_intersection(nptr p, nptr q, index l) {
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
	
	static nptr trie::set_difference(nptr p, nptr q, index l) {
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
	
	static nptr trie::set_difference(nptr p, nptr q, index l, index m) {
		assert(l >= m);
		if ( l == m ) return set_difference(p, q, l);
		if ( p.empty() ) return p;
		
		nptr r = set_difference(p.ptr->a[0], q, l-1, m);
		if ( r == p.ptr->a[0] ) return p;
		if ( r.empty() && p.ptr->only_first() ) return r;
		return p.ptr->set(0, r);
	}
	
	static std::pair<nptr, nptr> trie::rsh(nptr p, index i, index l) {
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
}
