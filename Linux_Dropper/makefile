CXX=g++
CXXFLAGS=-Wall -O2
LDFLAGS=

all: dropper

dropper: dropper.cpp network.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o dropper dropper.cpp

clean:
	rm -f dropper
