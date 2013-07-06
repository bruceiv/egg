CXXFLAGS = -O0 -ggdb --std=c++0x
#CXXFLAGS = -O3 --std=c++0x

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
