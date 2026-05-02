CXX := g++-14
CXXFLAGS := -O3 -march=native -std=c++11

.PHONY: all clean

all: samplecode run

samplecode: samplecode.cpp ObjectiveFunction.cpp Operation.cpp
	$(CXX) $(CXXFLAGS) -o samplecode samplecode.cpp ObjectiveFunction.cpp Operation.cpp

run: run.cpp
	$(CXX) $(CXXFLAGS) -o run run.cpp

clean:
	rm -f samplecode run
