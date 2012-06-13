CXXFLAGS = -O0 -ggdb --std=c++0x
#CXXFLAGS = -O3 --std=c++0x

egg:  main.cpp egg.hpp parse.hpp
	$(CXX) $(CXXFLAGS) -o egg main.cpp $(OBJS) $(LDFLAGS)


