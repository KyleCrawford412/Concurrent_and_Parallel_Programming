CXX := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -Wpedantic

all: mytree.x mytime.x mymtimes.x

mytree.x: mytree.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

mytime.x: mytime.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

mymtimes.x: mymtimes.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f mytree mytime mymtimes
