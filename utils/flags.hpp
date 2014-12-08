#pragma once

/*
 * Copyright (c) 2013 Aaron Moss
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

#include <cstdint>

#include <cstdio> // FIXME
#include <iostream> // FIXME
#include <cinttypes> // FIXME

namespace flags {

/// Gets the array element containing an index
static inline uint64_t el(uint64_t i) { return i >> 6; }

/// Gets the bit index in an array element of an index
static inline uint64_t shft(uint64_t i) { return 63 - (i & ((1 << 6) - 1)); }

/// Gets the mask of an index inside an element
static inline uint64_t mask(uint64_t i) { return UINT64_C(1) << shft(i); }

/// Gets the index of the leftmost bit in a byte (counting from the left)
static const char lt[256] = {
	-1, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,  // 15
	 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // 31
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // 63
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 127
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   // 255
};

/// Gets the index of the rightmost bit in a byte (counting from the left)
static const char rt[256] = {
	-1, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,  // 15
	 3, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,  // 31
	 2, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 3, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,  // 63
	 1, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 3, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 2, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 3, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,  // 127
	 0, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 3, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 2, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 3, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 1, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 3, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 2, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7,
	 3, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7   // 255
};

/// Gets the number of bits set in a byte
static const char ct[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,  // 15
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,  // 31
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,  // 63
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,  
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,  // 127
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,  
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8   // 255
};

/// Gets the index of the high bit inside an element (0 is MSB, 63 is LSB)
static uint64_t first(uint64_t x) {
	uint64_t t;
	if      ( x == 0 )        return -1;
	else if ( (t = x >> 56) ) return      lt[t];
	else if ( (t = x >> 48) ) return  8 + lt[t];
	else if ( (t = x >> 40) ) return 16 + lt[t];
	else if ( (t = x >> 32) ) return 24 + lt[t];
	else if ( (t = x >> 24) ) return 32 + lt[t];
	else if ( (t = x >> 16) ) return 40 + lt[t];
	else if ( (t = x >>  8) ) return 48 + lt[t];
	else                      return 56 + lt[x];
}

/// gets the index of the first bit of a (length n) set
static uint64_t first(const uint64_t* a, uint64_t n) {
	for (uint64_t i = 0; i < n; ++i) if ( a[i] != 0 ) return i*64 + first(a[i]);
	return -1;
}

/// gets the index of the next bit set after i
static uint64_t next(uint64_t x, uint64_t i) {
	uint64_t t = x & (mask(i)-1);
	return ( t == 0 ) ? -1 : first(t);
}

/// gets the index of the next bit set after i of a (length n)
static uint64_t next(const uint64_t* a, uint64_t n, uint64_t i) {
	uint64_t j = el(i);
	// mask off lower bits of this element
	uint64_t t = a[j] & (mask(i)-1);
	if ( t != 0 ) return j*64 + first(t);
	// if nothing there, check later elements
	for (++j; j < n; ++j) if ( a[j] != 0 ) return j*64 + first(a[j]);
	return -1;
}

/// Gets the index of the low bit inside an element (0 is MSB, 63 is LSB)
static uint64_t last(uint64_t x) {
	uint64_t t;
	if      ( x == 0 )                     return -1;
	else if ( (t = x &             0xff) ) return 56 + rt[t];
	else if ( (t = x &           0xff00) ) return 48 + rt[t >>  8];
	else if ( (t = x &         0xff0000) ) return 40 + rt[t >> 16];
	else if ( (t = x &       0xff000000) ) return 32 + rt[t >> 24];
	else if ( (t = x &     0xff00000000) ) return 24 + rt[t >> 32];
	else if ( (t = x &   0xff0000000000) ) return 16 + rt[t >> 40];
	else if ( (t = x & 0xff000000000000) ) return  8 + rt[t >> 48];
	else                                   return      rt[x >> 56];
}

/// gets the index of the last bit of a (length n) set
static uint64_t last(const uint64_t* a, uint64_t n) {
	do {
		--n;
		if ( a[n] != 0 ) return n*64 + last(a[n]);
	} while ( n > 0 );
	return -1;
}

/// Counts the number of bits set in an element
static inline uint64_t count(uint64_t x) {
	return ct[x & 0xff]
	     + ct[(x >>  8) & 0xff]
	     + ct[(x >> 16) & 0xff]
	     + ct[(x >> 24) & 0xff]
	     + ct[(x >> 32) & 0xff]
	     + ct[(x >> 40) & 0xff]
	     + ct[(x >> 48) & 0xff]
	     + ct[ x >> 56];
}

/// gets the count of set bits of a (length n)
static uint64_t count(const uint64_t* a, uint64_t n) {
	uint64_t c = 0;
	for (uint64_t i = 0; i < n; ++i) {
		c += count(a[i]);
	}
	return c;
}

/// gets the number of set bits of a before i
static inline uint64_t rank(uint64_t x, uint64_t i) { return count(x & ~((mask(i) << 1)-1)); }
static inline uint64_t rank(const uint64_t* a, uint64_t i) { return count(a, el(i)) + rank(a[el(i)], i); }

/// gets the index of the j'th bit
static uint64_t select(uint64_t x, uint64_t s) {
	uint64_t i = first(x);
	for (uint64_t r = 1; i != -1 && r < s; ++r) i = next(x, i);
	return i;
}

static uint64_t select(const uint64_t* a, uint64_t n, uint64_t s) {
	uint64_t c = 0;
	uint64_t i;
	for (i = 0; i < n-1; ++i) {
		if ( a[i] == 0 ) continue;
		uint64_t d = count(a[i]);
		if ( c + d >= s ) break;
		c += d;
	}
	return i*64 + select(a[i], s-c);
}

/// zero all flags
static inline void clear(uint64_t& x) { x = UINT64_C(0); }
static inline void clear(uint64_t* a, uint64_t n) { for (uint64_t i = 0; i < n; ++i) clear(a[i]); }

/// true if the i'th bit of a is set
static inline bool get(uint64_t x, uint64_t i) { return (x & mask(i)) != 0; }
static inline bool get(const uint64_t* a, uint64_t i) { return get(a[el(i)], i); }

/// sets the i'th bit of a
static inline void set(uint64_t& x, uint64_t i) { x |= mask(i); }
static inline void set(uint64_t* a, uint64_t i) { set(a[el(i)], i); }

/// unsets the i'th bit of a
static inline void unset(uint64_t& x, uint64_t i) { x &= ~mask(i); }
static inline void unset(uint64_t* a, uint64_t i) { unset(a[el(i)], i); }

/// flips the i'th bit of a
static inline void flip(uint64_t& x, uint64_t i) { x ^= mask(i); }
static inline void flip(uint64_t* a, uint64_t i) { flip(a[el(i)], i); }

/// checks if the bitflags are zeroed
static inline bool is_zero(uint64_t x) { return x == 0; }
static inline bool is_zero(const uint64_t* a, uint64_t n) {
	for (uint64_t i = 0; i < n; ++i) if ( a[i] != 0 ) return false;
	return true;
}

/// checks if two sets of bitflags intersect
static inline bool intersects(uint64_t x, uint64_t y) { return (x & y) != 0; }
static inline bool intersects(const uint64_t* a, const uint64_t* b, uint64_t n) {
	for (uint64_t i = 0; i < n; ++i) if ( intersects(a[i], b[i]) ) return true;
	return false;
}

/// takes the union of two sets of bitflags a and b, writing it into c
static inline void set_union(uint64_t x, uint64_t y, uint64_t& z) { z = x | y; }
static inline void set_union(const uint64_t* a, const uint64_t* b, uint64_t* c, uint64_t n) {
	for (uint64_t i = 0; i < n; ++i) set_union(a[i], b[i], c[i]);
}

/// takes the intersection of two sets of bitflags a and b, writing it into c
static inline void set_intersection(uint64_t x, uint64_t y, uint64_t& z) { z = x & y; }
static inline void set_intersection(const uint64_t* a, const uint64_t* b, uint64_t* c, uint64_t n) {
	for (uint64_t i = 0; i < n; ++i) set_intersection(a[i], b[i], c[i]);
}

/// takes the set difference of two sets of bitflags a and b, writing it into c
static inline void set_difference(uint64_t x, uint64_t y, uint64_t& z) { z = x & ~y; }
static inline void set_difference(const uint64_t* a, const uint64_t* b, uint64_t* c, uint64_t n) {
	for (uint64_t i = 0; i < n; ++i) set_difference(a[i], b[i], c[i]);
}

/// shifts a (length n) left by s bits, writing the result into c (won't clear unshifted limbs of c)
static void lsh(const uint64_t* a, const uint64_t s, uint64_t* c, uint64_t n) {
	uint64_t s_l = s >> 6;   // s / 64, number of limbs to shift
	uint64_t s_b = s & 0x3F; // s % 64, number of bits to shift
	uint64_t u_s = 64 - s_b; // number of bits to unshift high-order bits

	for (uint64_t i = 0; i < n-1-s_l; ++i) {
		// get low-order bits of current limb, then high-order bits of next back
		c[i] = (a[s_l+i] << s_b) | (a[s_l+i+1] >> u_s);
	}
	c[n-1-s_l] = a[n-1] << s_b;
	// 
/*std::cout << "\t\t\t\tlsh: s_l=" << s_l << " s_b=" << s_b << " u_s=" << u_s << std::endl;
char in[17], in2[17], out[17], out0[17];
uint64_t zin[2] = {0, 0}, oin[2] = {1, 0}, fin[2] = {0xffffffffffffffff, 0}; 
uint64_t t = zin[1] >> u_s;
std::sprintf(out0, "%0.16" PRIx64, t);
t = oin[1] >> u_s;
std::sprintf(out, "%0.16" PRIx64, t);
t = fin[1] >> u_s;
std::sprintf(in2, "%0.16" PRIx64, t);
std::cout << "\t\t\t\t\t shifted from 0 above=" << out0 << " 1 above=" << out << " f... above=" << in2 << std::endl;
	
	// get high order bits of high limb
std::sprintf(out0, "%0.16" PRIx64, c[n+s_l]);
	c[n+s_l] = a[n-1] >> u_s;
std::sprintf(in, "%0.16" PRIx64, a[n-1]);
std::sprintf(out, "%0.16" PRIx64, c[n+s_l]);
std::cout << "\t\t\t\t\tin[" << (n-1) << "]=" << in << " -> out[" << (n+s_l) << "]=" << out << " <- " << out0 << std::endl; 
	// shift remaining limbs
	for (uint64_t i = n-1; i > 0; --i) {
		// get low order bits of current limb, then high order bits of next back
std::sprintf(out0, "%0.16" PRIx64, c[i+s_l]);
		c[i+s_l] = (a[i] << s_b) | (a[i-1] >> u_s);
std::sprintf(in, "%0.16" PRIx64, a[i]);
std::sprintf(in2, "%0.16" PRIx64, a[i-1]);
std::sprintf(out, "%0.16" PRIx64, c[i+s_l]);
std::cout << "\t\t\t\t\tin[" << i << "," << (i-1) << "]=" << in << " " << in2 << " -> out[" << (i+s_l) << "]=" << out << " <- " << out0 << std::endl;
	}
	// get low order bits of low limb
std::sprintf(out0, "%0.16" PRIx64, c[s_l]);
	c[s_l] = a[0] << s_b;
std::sprintf(in, "%0.16" PRIx64, a[0]);
std::sprintf(out, "%0.16" PRIx64, c[s_l]);
std::cout << "\t\t\t\t\tin[0]=" << in << " -> out[" << s_l << "]=" << out << " <- " << out0 << std::endl;
	// zero remaining low order limbs
	clear(c, s_l);
*/}

/// shifts a (length n) right by s bits, writing the result into c (won't clear low-order limbs of c)
static void rsh(const uint64_t* a, const uint64_t s, uint64_t* c, uint64_t n) {
	uint64_t s_l = s >> 6;    // s / 64, number of limbs to shift
	uint64_t s_b = s & 0x3F;  // s % 64, number of bits to shift
	uint64_t u_s = 64 - s_b;  // number of bits to unshift low-order bits

	c[n+s_l] = a[n-1] << u_s;
	for (uint64_t i = n-1; i > 0; --i) {
		// get high order bits of current limb, then low-order bits of previous
		c[i+s_l] = (a[i-1] << u_s) | (a[i] >> s_b);
	}
	c[s_l] = a[0] >> s_b;
}

} /* namespace flags */

