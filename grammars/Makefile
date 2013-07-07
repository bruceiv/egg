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

test: egg abc anbncn calc
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
