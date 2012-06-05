OBJS=egg.o

egg:  $(OBJS)
	$(CXX) $(CXXFLAGS) -o egg main.cpp $(OBJS) $(LDFLAGS)

