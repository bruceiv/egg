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

/** Tests for utility code. Returns 0 for all passed, 1 otherwise */

#include <cstdint>
#include <vector>

#include "test.hpp"

#include "../flags.hpp"
#include "../flagtrie.hpp"

void test_flags(tester& test) {
	test.setup("flags");

	uint64_t a[2] = { UINT64_C(0xDEADBEEFDEADBEEF), UINT64_C(0xDEADBEEFDEADBEEF) };

	// Test that these are zeroed as expected
	flags::clear(a, 2);
	test.equal(a[0], 0); test.equal(a[1], 0);

	// Test the basic element, bit-index, and mask functions
	test.equal(flags::el(0),     UINT64_C(0), "el(0)");
	test.equal(flags::shft(0),   UINT64_C(63), "shft(0)");
	test.equal(flags::mask(0),   UINT64_C(0x8000000000000000), "mask(0)");
	test.equal(flags::el(42),    UINT64_C(0), "el(42)");
	test.equal(flags::shft(42),  UINT64_C(21), "shft(42)");
	test.equal(flags::mask(42),  UINT64_C(0x0000000000200000), "mask(42)");
	test.equal(flags::el(63),    UINT64_C(0), "el(63)");
	test.equal(flags::shft(63),  UINT64_C(0), "shft(63)");
	test.equal(flags::mask(63),  UINT64_C(0x0000000000000001), "mask(63)");
	test.equal(flags::el(64),    UINT64_C(1), "el(64)");
	test.equal(flags::shft(64),  UINT64_C(63), "shft(64)");
	test.equal(flags::mask(64),  UINT64_C(0x8000000000000000), "mask(64)");
	test.equal(flags::el(71),    UINT64_C(1), "el(71)");
	test.equal(flags::shft(71),  UINT64_C(56), "shft(71)");
	test.equal(flags::mask(71),  UINT64_C(0x0100000000000000), "mask(71)");
	test.equal(flags::el(127),   UINT64_C(1), "el(127)");
	test.equal(flags::shft(127), UINT64_C(0), "shft(127)");
	test.equal(flags::mask(127), UINT64_C(0x0000000000000001), "mask(127)");

	// Test that set and get work together as expected
	flags::set(a, 0);   test.check(flags::get(a, 0),   "set then get 0");
	flags::set(a, 42);  test.check(flags::get(a, 42),  "set then get 42");
	flags::set(a, 63);  test.check(flags::get(a, 63),  "set then get 63");
	test.equal(a[0], UINT64_C(0x8000000000200001), "a[0] value post set 1");
	test.equal(a[1], UINT64_C(0x0000000000000000), "a[1] value post set 1");
	
	flags::set(a, 64);  test.check(flags::get(a, 64),  "set then get 64");
	flags::set(a, 71);  test.check(flags::get(a, 71),  "set then get 71");
	flags::set(a, 127); test.check(flags::get(a, 127), "set then get 127");
	test.equal(a[0], UINT64_C(0x8000000000200001), "a[0] value post set 2");
	test.equal(a[1], UINT64_C(0x8100000000000001), "a[1] value post set 2");

	// Test that unsetting bits works as well
	flags::unset(a, 0);   test.check(!flags::get(a, 0),   "unset then get 0");
	flags::unset(a, 42);  test.check(!flags::get(a, 42),  "unset then get 42");
	flags::unset(a, 63);  test.check(!flags::get(a, 63),  "unset then get 63");
	flags::unset(a, 64);  test.check(!flags::get(a, 64),  "unset then get 64");
	flags::unset(a, 71);  test.check(!flags::get(a, 71),  "unset then get 71");
	flags::unset(a, 127); test.check(!flags::get(a, 127), "unset then get 127");

	// Test that the bitflags look like I expect
	test.equal(a[0], UINT64_C(0x0000000000000000), "a[0] value post unset");
	test.equal(a[1], UINT64_C(0x0000000000000000), "a[1] value post unset");
	
	uint64_t b[6] = {
		UINT64_C(0xfedcba9876543210), UINT64_C(0x0edcba9876543210),
		UINT64_C(0x000cba9876543210), UINT64_C(0x0000000000043210),
		UINT64_C(0x0000000000000010), UINT64_C(0x0000000000000000)
	};
	a[0] = UINT64_C(0x0000000000000000); a[1] = b[2];

	// Test high bit function
	test.equal(flags::first(b[0]),  0, "first(b[0])");
	test.equal(flags::first(b[1]),  4, "first(b[1])");
	test.equal(flags::first(b[2]), 12, "first(b[2])");
	test.equal(flags::first(b[3]), 45, "first(b[3])");
	test.equal(flags::first(b[4]), 59, "first(b[4])");
	test.equal(flags::first(b[5]), -1, "first(b[5])");
	
	uint64_t c[2] = { UINT64_C(0x8000000000000000), UINT64_C(0x0000000000000002) };
	
	// Test low bit function
	test.equal(flags::last(c, 2), 126, "last(c)");
	test.equal(flags::last(c[0]),   0, "last(c[0])");
	test.equal(flags::last(c[1]),  62, "last(c[1])");
	
	// Test count bits
	test.equal(flags::count(b[0]), 32, "count(b[0])");
	test.equal(flags::count(b[1]), 28, "count(b[1])");
	test.equal(flags::count(b[2]), 22, "count(b[2])");
	test.equal(flags::count(b[3]),  5, "count(b[3])");
	test.equal(flags::count(b[4]),  1, "count(b[4])");
	test.equal(flags::count(b[5]),  0, "count(b[5])");

	test.equal(flags::count(b, 6), 88, "count(b)");
	test.equal(flags::count(a, 2), 22, "count(a)");

	// Test rank
	test.equal(flags::rank(b,   0),  0, "rank(b, 0)");
	test.equal(flags::rank(b,   1),  1, "rank(b, 1)");
	test.equal(flags::rank(b,   8),  7, "rank(b, 8)");
	test.equal(flags::rank(b,  63), 32, "rank(b, 63)");
	test.equal(flags::rank(b,  64), 32, "rank(b, 64)");
	test.equal(flags::rank(b,  72), 35, "rank(b, 72)");
	test.equal(flags::rank(b, 127), 60, "rank(b, 127)");

	// Test first and next
	test.equal(flags::first(b, 6),    0, "first(b)");
	test.equal(flags::first(b+1, 5),  4, "first(b+1)");
	test.equal(flags::first(b+5, 1), -1, "first(b+5)");
	test.equal(flags::first(a, 2),   76, "first(a)");

	test.equal(flags::next(b, 6,  0),    1, "next(b, 0)");
	test.equal(flags::next(b, 6, 68),   69, "next(b, 68)");
	test.equal(flags::next(b+4, 2, 60), -1, "next(b+4, 60)");
	test.equal(flags::next(a, 2, 32),   76, "next(a, 32)");
	
	// TODO add tests for last
	
	test.cleanup();
}

void test_flagtrie(tester& test) {
	using flags::trie;
	using l = std::vector<uint64_t>;
	using std::vector;

	test.setup("flagtrie");

	// Test creation
	trie t_mt;
	trie t0 = trie::of(7);
	trie t1 = trie::of(65);
	trie t2 = trie::of(600);

	test.equal_range(t_mt, l{}, "create empty");
	test.equal_range(t0, l{7}, "create level-0");
	test.equal_range(t1, l{65}, "create level-1");
	test.equal_range(t2, l{600}, "create level-2");

	// Test union
	trie t0b = trie::of(0);
	trie t0u = t0 | t0b;
	test.equal_range(t0u, l{0, 7}, "union level-0");
	
	t0b |= t0;
	test.equal_range(t0u, t0b, "union-assign level-0");

	trie t1b = trie::of(72);
	trie t1u = t1 | t1b;
	test.equal_range(t1u, l{65, 72}, "union level-1");
	
	t1b |= t1;
	test.equal_range(t1b, t1u, "union-assign level-1");

	trie t01u = t0u | t1u;
	test.equal_range(t01u, l{0, 7, 65, 72}, "union level-0+1");

	// Test intersects
	test.check(!t_mt.intersects(t0), "empty no-intersect level-0");
	test.check(!t_mt.intersects(t1), "empty no-intersect level-1");
	test.check(!t_mt.intersects(t2), "empty no-intersect level-2");
	test.check(!t0.intersects(t1), "no-intersect level-0+1");
	test.check(!t0.intersects(t2), "no-intersect level-0+2");
	test.check(!t1.intersects(t2), "no-intersect level-1+2");
	test.check(t0.intersects(t0u), "intersect level-0");
	test.check(t1.intersects(t1u), "intersect level-1");
	test.check(t0.intersects(t01u), "intersect level-0+1 from 0");
	test.check(t1.intersects(t01u), "intersect level-0+1 from 1");

	trie t01u_ = trie::of(0) | trie::of(7) | trie::of(65) | trie::of(72);
	test.equal_range(t01u, t01u_, "independent rebuild level-0+1");
	test.check(t01u.intersects(t01u_), "intersect independed rebuild level-0+1");

	// Test subtract
	trie t_mt_ = t01u - t01u_;
	test.equal_range(t_mt_, l{}, "subtract trie from rebuilt self");
	
	trie t1c = t1b - t1;
	test.equal_range(t1c, l{72}, "subtract level-0");
	trie t1d = t1c - t1;
	test.equal_range(t1c, t1d, "subtract non-present level-0");

	trie s0 = trie::of(60) | trie::of(61) | trie::of(66) | trie::of(76) | trie::of(77);
	trie s1 = trie::of(0) | trie::of(60) | trie::of(61) | trie::of(67);
	s0 -= s1;
	test.equal_range(s0, l{66, 76, 77}, "subtract from auto-test");

	// TODO Test intersect

	// Test right-shift
	trie t_zero = trie::of(0);
	trie t_one = t_zero >> 1;
	trie t_64 = t_zero >> 64;
	trie t_65 = t_zero >> 65;
	trie t_576 = t_zero >> 576;
	trie t_600 = t_zero >> 600;
	
	test.equal_range(t_one, l{1}, "0 >> 1");
	test.equal_range(t_64, l{64}, "0 >> 64");
	test.equal_range(t_65, l{65}, "0 >> 65");
	test.equal_range(t_576, l{576}, "0 >> 576");
	test.equal_range(t_600, l{600}, "0 >> 600");

	trie r0 = trie::of(0) | trie::of(65) | trie::of(130) | trie::of(195) 
                  | trie::of(260) | trie::of(325) | trie::of(390) | trie::of(455);

	trie r_1 = r0 >> 1;
	trie r_63 = r0 >> 63;
	trie r_64 = r0 >> 64;
	trie r_448 = r0 >> 448;
	trie r_450 = r0 >> 450;
	trie r_511 = r0 >> 511;
	trie r_512 = r0 >> 512;
	trie r_550 = r0 >> 550;

	test.equal_range(r_1, l{1, 66, 131, 196, 261, 326, 391, 456}, "r0 >> 1");
	test.equal_range(r_63, l{63, 128, 193, 258, 323, 388, 453, 518}, "r0 >> 63");
	test.equal_range(r_64, l{64, 129, 194, 259, 324, 389, 454, 519}, "r0 >> 64");
	test.equal_range(r_448, l{448, 448+65, 448+130, 448+195, 448+260, 448+325, 448+390, 448+455}, "r0 >> 448");
	test.equal_range(r_450, l{450, 450+65, 450+130, 450+195, 450+260, 450+325, 450+390, 450+455}, "r0 >> 450");
	test.equal_range(r_511, l{511, 511+65, 511+130, 511+195, 511+260, 511+325, 511+390, 511+455}, "r0 >> 511");
	test.equal_range(r_512, l{512, 512+65, 512+130, 512+195, 512+260, 512+325, 512+390, 512+455}, "r0 >> 512");
	test.equal_range(r_550, l{550, 550+65, 550+130, 550+195, 550+260, 550+325, 550+390, 550+455}, "r0 >> 550");

	// TODO test more complex things - you really want to lean on the boundary conditions and unions

	test.cleanup();
}

int main(int argc, char** argv) {
	tester test;

	test_flags(test);
	test_flagtrie(test);
	
	return test.success() ? 0 : 1;
}

