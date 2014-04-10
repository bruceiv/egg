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

#include "test.hpp"

#include "../flags.hpp"
#include "../uint_set.hpp"

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

void test_uint_set(tester& test) {
	test.setup("uint_set");
	
	utils::uint_set x{1,3}, y{1,2,3};
	
	test.check(x != y, "sets start unequal");
	test.equal(x.min(), 1, "x.min == 1");
	test.equal(x.max(), 3, "x.max == 3");
	test.equal(y.min(), 1, "y.min == 1");
	test.equal(y.max(), 3, "y.max == 3");
	test.equal(x.count(), 2, "|x| == 2");
	test.equal(y.count(), 3, "|y| == 3");
	
	x |= 2;
	
	test.equal(x.count(), 3, "new |x| == 3");
	test.check(x == y, "sets end equal");
	
	// TODO better test coverage
	
	test.cleanup();
}

int main(int argc, char** argv) {
	tester test;

	test_flags(test);
	test_uint_set(test);
	
	return test.success() ? 0 : 1;
}

