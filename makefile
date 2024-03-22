CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra

all: mytoolkit.x

mytoolkit.x: mytoolkit.cpp
	$(CXX) $(CXXFLAGS) -o mytoolkit mytoolkit.cpp

clean:
	rm -f mytoolkit
