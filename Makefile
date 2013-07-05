CXXFLAGS = -O0 -ggdb --std=c++0x
#CXXFLAGS = -O3 --std=c++0x

egg:  main.cpp egg.hpp parse.hpp visitors/printer.hpp visitors/compiler.hpp visitors/normalizer.hpp
	$(CXX) $(CXXFLAGS) -o egg main.cpp $(OBJS) $(LDFLAGS)


