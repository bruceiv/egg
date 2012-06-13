
egg:  main.cpp egg.hpp parse.hpp
	$(CXX) $(CXXFLAGS) -o egg main.cpp $(OBJS) $(LDFLAGS)


