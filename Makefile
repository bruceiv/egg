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

#CXX = clang++
CXX = g++

# Development
CXXFLAGS = -O0 -ggdb --std=c++0x

# Profiling
#CXXFLAGS = -O0 -ggdb --std=c++0x -DNDEBUG
#CXXFLAGS = -O2 -ggdb --std=c++0x -DNDEBUG

# Release
#CXXFLAGS = -O2 --std=c++0x -DNDEBUG
CXXFLAGS = -O3 --std=c++0x -DNDEBUG

OBJS = derivs.o
OBJS_MUT = derivs-mut.o deriv_fixer-mut.o

derivs.o:  derivs.cpp derivs.hpp utils/uint_pfn.hpp utils/uint_set.hpp
	$(CXX) $(CXXFLAGS) -c derivs.cpp

derivs-mut.o:  derivs-mut.cpp derivs-mut.hpp utils/uint_pfn.hpp utils/uint_set.hpp
	$(CXX) $(CXXFLAGS) -c derivs-mut.cpp

deriv_fixer-mut.o:  derivs-mut.o
	$(CXX) $(CXXFLAGS) -c visitors/deriv_fixer-mut.cpp

egg:  main.cpp $(OBJS) egg.hpp parser.hpp \
      visitors/printer.hpp visitors/compiler.hpp visitors/interpreter.hpp visitors/normalizer.hpp \
      visitors/deriv_printer.hpp
	$(CXX) $(CXXFLAGS) -o egg main.cpp $(OBJS) $(LDFLAGS)

egg-mut:  main.cpp $(OBJS_MUT) egg.hpp parser.hpp \
      visitors/printer.hpp visitors/compiler.hpp visitors/interpreter-mut.hpp visitors/normalizer.hpp \
      visitors/deriv_printer-mut.hpp
	$(CXX) $(CXXFLAGS) -DEGG_MUT -o egg-mut main.cpp $(OBJS_MUT) $(LDFLAGS)

clean:  
	-rm derivs.o
	-rm egg
