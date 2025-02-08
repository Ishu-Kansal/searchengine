CXX = g++
CXXFLAGS = -std=c++11 -g

all: ConcurrentListDirectories ConcurrentWordCount

ConcurrentListDirectories: ConcurrentListDirectory.cpp
	$(CXX) $(CXXFLAGS) $^ -pthread -o listDirectory.exe

ConcurrentWordCount: ConcurrentWordCount.cpp
	$(CXX) $(CXXFLAGS) $^ -pthread -o wordCount.exe

.PHONY: clean

clean:
	rm -f listDirectory.exe wordCount.exe
