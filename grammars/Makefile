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

CXX = clang++
#CXX = g++

# Development
CXXFLAGS = -O0 -ggdb --std=c++0x

# Profiling
#CXXFLAGS = -O0 -ggdb --std=c++0x -DNDEBUG
#CXXFLAGS = -O2 -ggdb --std=c++0x -DNDEBUG

# Release
#CXXFLAGS = -O2 --std=c++0x -DNDEBUG
#CXXFLAGS = -O3 --std=c++0x -DNDEBUG

parser.hpp:  
	ln -s ../parser.hpp .

%.cpp:  %.egg
	../egg -o $@ -i $<

xml.hpp:  ../perf_test/XML.egg
	../egg -o xml.hpp -i ../perf_test/XML.egg

json.hpp:  ../perf_test/JSON.egg
	../egg -o json.hpp -i ../perf_test/JSON.egg

java.hpp:  ../perf_test/Java.egg
	../egg -o java.hpp -i ../perf_test/Java.egg

%-nomemo.cpp:  %.egg
	../egg --no-memo -o $@ -i $<

xml-nomemo.hpp:  ../perf_test/XML.egg
	../egg --no-memo -o xml-nomemo.hpp -i ../perf_test/XML.egg

json-nomemo.hpp:  ../perf_test/JSON.egg
	../egg --no-memo -o json-nomemo.hpp -i ../perf_test/JSON.egg

java-nomemo.hpp:  ../perf_test/Java.egg
	../egg --no-memo -o java-nomemo.hpp -i ../perf_test/Java.egg

abc:  abc.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o abc abc.cpp $(LDFLAGS)

anbncn:  anbncn.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o anbncn anbncn.cpp $(LDFLAGS)

calc:  calc.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o calc calc.cpp $(LDFLAGS)

nested:  nested.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o nested nested.cpp $(LDFLAGS)

xml:  xml.hpp xml.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o xml xml.cpp $(LDFLAGS)

json:  json.hpp json.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o json json.cpp $(LDFLAGS)

java:  java.hpp java.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o java java.cpp $(LDFLAGS)

abc-nomemo:  abc-nomemo.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o abc-nomemo abc-nomemo.cpp $(LDFLAGS)

anbncn-nomemo:  anbncn-nomemo.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o anbncn-nomemo anbncn-nomemo.cpp $(LDFLAGS)

calc-nomemo:  calc-nomemo.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o calc-nomemo calc-nomemo.cpp $(LDFLAGS)

nested-nomemo:  nested-nomemo.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -o nested-nomemo nested-nomemo.cpp $(LDFLAGS)

xml-nomemo:  xml-nomemo.hpp xml.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -DNOMEMO -o xml-nomemo xml.cpp $(LDFLAGS)

json-nomemo:  json-nomemo.hpp json.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -DNOMEMO -o json-nomemo json.cpp $(LDFLAGS)

java-nomemo:  java-nomemo.hpp java.cpp parser.hpp
	$(CXX) $(CXXFLAGS) -DNOMEMO -o java-nomemo java.cpp $(LDFLAGS)

egg:
	cd .. && $(MAKE) egg

clean:  
	-rm abc abc.cpp 
	-rm anbncn anbncn.cpp 
	-rm calc calc.cpp
	-rm nested nested.cpp
	-rm xml xml.hpp
	-rm json json.hpp
	-rm java java.hpp
	-rm abc-nomemo abc-nomemo.cpp
	-rm anbncn-nomemo anbncn-nomemo.cpp
	-rm calc-nomemo calc-nomemo.cpp
	-rm nested-nomemo nested-nomemo.cpp
	-rm xml-nomemo xml-nomemo.hpp
	-rm json-nomemo json-nomemo.hpp
	-rm java-nomemo java-nomemo.hpp

test-compile: abc anbncn calc nested xml json
	@echo
	./abc < tests/abc.in.txt > tests/abc.test.txt
	diff tests/abc.out.txt tests/abc.test.txt
	./anbncn < tests/anbncn.in.txt > tests/anbncn.test.txt
	diff tests/anbncn.out.txt tests/anbncn.test.txt
	./calc < tests/calc.in.txt > tests/calc.test.txt
	diff tests/calc.out.txt tests/calc.test.txt
	./nested < tests/nested.in.txt > tests/nested.test.txt
	diff tests/nested.out.txt tests/nested.test.txt
	./xml < ../perf_test/input/ijs-elan2.0/kuca-xx.tei > tests/xml.test.txt
	diff tests/xml.out.txt tests/xml.test.txt
	./json < ../perf_test/input/json-ijs-elan2.0/kuca-xx.tei.json > tests/json.test.txt
	diff tests/json.out.txt tests/json.test.txt
	rm tests/*.test.txt
	@echo
	@echo COMPILED TESTS PASSED

test-nomemo: abc-nomemo anbncn-nomemo calc-nomemo nested-nomemo xml-nomemo json-nomemo
	@echo
	./abc-nomemo < tests/abc.in.txt > tests/abc.test.txt
	diff tests/abc.out.txt tests/abc.test.txt
	./anbncn-nomemo < tests/anbncn.in.txt > tests/anbncn.test.txt
	diff tests/anbncn.out.txt tests/anbncn.test.txt
	./calc-nomemo < tests/calc.in.txt > tests/calc.test.txt
	diff tests/calc.out.txt tests/calc.test.txt
	./nested-nomemo < tests/nested.in.txt > tests/nested.test.txt
	diff tests/nested.out.txt tests/nested.test.txt
	./xml-nomemo < ../perf_test/input/ijs-elan2.0/kuca-xx.tei > tests/xml.test.txt
	diff tests/xml.out.txt tests/xml.test.txt
	./json-nomemo < ../perf_test/input/json-ijs-elan2.0/kuca-xx.tei.json > tests/json.test.txt
	diff tests/json.out.txt tests/json.test.txt
	rm tests/*.test.txt
	@echo
	@echo NO-MEMO TESTS PASSED

test-interpret: egg
	@echo
	../egg match-lines abc.egg g1 tests/abc.in.txt > tests/abc.test.txt
	diff tests/abc.dout.txt tests/abc.test.txt
	../egg match-lines anbncn.egg G tests/anbncn.in.txt > tests/anbncn.test.txt
	diff tests/anbncn.dout.txt tests/anbncn.test.txt
	../egg match-lines calc.egg expr tests/calc.in.txt > tests/calc.test.txt
	diff tests/calc.dout.txt tests/calc.test.txt
	../egg match-lines nested.egg String tests/nested.in.txt > tests/nested.test.txt
	diff tests/nested.dout.txt tests/nested.test.txt
	../egg match-lines eggr.egg grammar tests/egg.in.txt > tests/egg.test.txt
	diff tests/egg.dout.txt tests/egg.test.txt
	../egg match eggr.egg grammar eggr.egg > tests/eggr.test.txt
	diff tests/eggr.dout.txt tests/eggr.test.txt
	../egg match ../perf_test/JSON.egg JSON ../perf_test/input/json-ijs-elan2.0/kuca-xx.tei.json > tests/json.test.txt
	diff tests/json.dout.txt tests/json.test.txt
	../egg match ../perf_test/XML.egg Document ../perf_test/input/ijs-elan2.0/kuca-xx.tei > tests/xml.test.txt
	diff tests/xml.dout.txt tests/xml.test.txt
	rm tests/*.test.txt
	@echo
	@echo INTERPRETED TESTS PASSED

test: test-compile test-interpret

