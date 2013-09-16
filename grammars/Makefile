# Copyright (c) 2013 Aaron Moss
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

#CXXFLAGS = -O0 -ggdb --std=c++0x
CXXFLAGS = -O3 --std=c++0x

parse.hpp:  
	ln -s ../parse.hpp .

%.cpp:  %.egg
	../egg -o $@ -i $<

abc:  abc.cpp parse.hpp
	$(CXX) $(CXXFLAGS) -o abc abc.cpp $(LDFLAGS)

anbncn:  anbncn.cpp parse.hpp
	$(CXX) $(CXXFLAGS) -o anbncn anbncn.cpp $(LDFLAGS)

calc:  calc.cpp parse.hpp
	$(CXX) $(CXXFLAGS) -o calc calc.cpp $(LDFLAGS)

egg:
	cd .. && $(MAKE) egg

clean:  
	-rm abc abc.cpp 
	-rm anbncn anbncn.cpp 
	-rm calc calc.cpp

reporting:
	../egg -i errorcalc.egg 2>&1 | grep -i -q "line 7"

test: egg abc anbncn calc reporting
	@echo
	./abc < tests/abc.in.txt > tests/abc.test.txt
	diff tests/abc.out.txt tests/abc.test.txt
	./anbncn < tests/anbncn.in.txt > tests/anbncn.test.txt
	diff tests/anbncn.out.txt tests/anbncn.test.txt
	./calc < tests/calc.in.txt > tests/calc.test.txt
	diff tests/calc.out.txt tests/calc.test.txt
	rm tests/*.test.txt
	@echo
	@echo TESTS PASSED

.PHONY: reporting test clean
