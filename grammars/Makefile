CXXFLAGS = -O0 -ggdb --std=c++0x
#CXXFLAGS = -O3 --std=c++0x

parse.hpp:  
	ln -s ../parse.hpp .

%.hpp:  %.egg
	../egg -o $@ -i $<

abc:  abc.cpp abc.hpp parse.hpp
	$(CXX) $(CXXFLAGS) -o abc abc.cpp $(LDFLAGS)

anbncn:  anbncn.cpp anbncn.hpp parse.hpp
	$(CXX) $(CXXFLAGS) -o anbncn anbncn.cpp $(LDFLAGS)

calc:  calc.cpp calc.hpp parse.hpp
	$(CXX) $(CXXFLAGS) -o calc calc.cpp $(LDFLAGS)
