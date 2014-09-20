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
#include <cstdio>

struct tester {
	int total_passed;
	int total_tests;
	int passed;
	int tests;

	/// Constructor - intializes empty test object and prints header
	tester() : total_passed(0), total_tests(0), passed(0), tests(0) {
		printf("Starting tests\n");
	}

	/// Destructor - prints trailer
	~tester() {
		printf("%d of %d tests passed\n", total_passed, total_tests);
	}

	/// Checks whether all the tests run to date have passed
	bool success() { return (total_passed == total_tests && passed == tests); }

	/// Setup named test block
	void setup(const char* name) {
		passed = 0;
		tests = 0;
		printf("\tTesting %s\n", name);
	}

	/// Close test block
	void cleanup() {
		printf("\t%d of %d tests passed\n", passed, tests);
		total_passed += passed;
		total_tests += tests;
	}

	/// Check condition (with optional description)
	void check(bool cond, const char* err = "") {
		++tests; 
		if ( cond ) ++passed; 
		else printf("\t\tFAILED %s\n", err);
	}

	/// Check 64-bit integers are equal (with optional description)
	void equal(uint64_t a, uint64_t b, const char* err = "") {
		++tests;
		if ( a == b ) ++passed;
		else printf("\t\tFAILED %s %016llx != %016llx\n", err, a, b);
	}

}; /* struct tester */

