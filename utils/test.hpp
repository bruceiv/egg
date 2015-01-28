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

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

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

	/// Check pointers are equal (with optional description)
	template<typename T>
	inline void equal(T* a, T* b, const char* err = "") {
		equal(reinterpret_cast<uint64_t>(a), reinterpret_cast<uint64_t>(b), err);
	}

	/// Check collections are equal (with optional description)
	template<typename C, typename D>
	void equal_range(C c, D d, const char* err = "") {
		++tests;

		auto begin1 = c.begin(), end1 = c.end();
		auto begin2 = d.begin(), end2 = d.end();
		unsigned long count = 0;
		while ( begin1 != end1 && begin2 != end2) {
			if ( *begin1 != *begin2 ) {
				std::stringstream ss1, ss2;
				ss1 << *begin1; ss2 << *begin2;
				printf("\t\tFAILED %s @%lu %s != %s\n",
				       err, count, ss1.str().c_str(), ss2.str().c_str());
				return;
			}
			
			++begin1; ++begin2; ++count;
		}
		if ( begin1 != end1 ) {
			printf("\t\tFAILED %s @%lu first range has extra elements\n", err, count);
			return;
		}
		if ( begin2 != end2 ) {
			printf("\t\tFAILED %s @%lu second range has extra elements\n", err, count);
			return;
		}

		++passed;
	}

	/// Check unsorted collections are equal (with optional description)
	template<typename C, typename D>
	void equal_set(C c, D d, const char* err = "") {
		std::vector<typename C::value_type> cv(c.begin(), c.end());
		std::vector<typename D::value_type> dv(d.begin(), d.end());
		std::sort(cv.begin(), cv.end());
		std::sort(dv.begin(), dv.end());
		equal_range(cv, dv, err);
	}

}; /* struct tester */

