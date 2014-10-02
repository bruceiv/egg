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
#CXXFLAGS = -O0 -ggdb --std=c++0x
CXXFLAGS = -O0 -ggdb --std=c++0x -fmax-errors=10 -fdiagnostics-color=auto

# Profiling
#CXXFLAGS = -O0 -ggdb --std=c++0x -DNDEBUG
#CXXFLAGS = -O2 -ggdb --std=c++0x -DNDEBUG

# Release
#CXXFLAGS = -O2 --std=c++0x -DNDEBUG
#CXXFLAGS = -O3 --std=c++0x -DNDEBUG

OBJS = dlf.o

dlf.o:  dlf.cpp dlf.hpp utils/flagvector.hpp utils/flags.hpp
	$(CXX) $(CXXFLAGS) -c dlf.cpp

egg:  $(OBJS) main.cpp egg.hpp parser.hpp visitors/printer.hpp visitors/compiler.hpp visitors/normalizer.hpp
	$(CXX) $(CXXFLAGS) -o egg main.cpp $(OBJS) $(LDFLAGS)

clean:  
	-rm dlf.o
	-rm egg
