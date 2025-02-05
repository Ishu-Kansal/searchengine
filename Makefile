CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O0
EXECUTABLE  = Top10
PROJECTFILE = Top10.cpp
SOURCES     = $(Top10.cpp)
SOURCES     := $(filter-out $(TESTSOURCES), $(SOURCES))
all: Top10 HashTable HashBlob HashFile

Top10: Top10.cpp Common.h Common.cpp TopN.h TopN.cpp HashTable.h
	$(CXX) $(CXXFLAGS) Top10.cpp Common.cpp TopN.cpp -o Top10

HashTable: HashTable.cpp Common.h Common.cpp HashTable.h Timer.h
	$(CXX) $(CXXFLAGS) HashTable.cpp Common.cpp -o HashTable

HashBlob: HashBlob.cpp HashTable.h HashBlob.h Common.h Common.cpp Timer.h
	$(CXX) $(CXXFLAGS) HashBlob.cpp Common.cpp -o HashBlob

HashFile: HashFile.cpp HashTable.h HashBlob.h Common.h Timer.h
	$(CXX) $(CXXFLAGS) HashFile.cpp Common.cpp -o HashFile

test: Top10 HashTable HashBlob HashFile BigJunkHtml.txt Top10.correct.txt HashTable.correct.txt
	./Top10 BigJunkHtml.txt > Top10.testout.txt
	diff Top10.correct.txt Top10.testout.txt
	./HashTable BigJunkHtml.txt < BigJunkHtml.txt > HashTable.testout.txt
	diff HashTable.correct.txt HashTable.testout.txt
	./HashBlob BigJunkHtml.txt < BigJunkHtml.txt > HashBlob.testout.txt
	diff HashTable.correct.txt HashBlob.testout.txt
	./HashBlob BigJunkHtml.txt HashBlob.testout.bin < /dev/null
	./HashFile HashBlob.testout.bin < BigJunkHtml.txt > HashFile.testout.txt
	diff HashTable.correct.txt HashFile.testout.txt

clean:
	rm -f Top10 HashTable HashBlob HashFile *.testout.*

debug: CXXFLAGS += -g3 -DDEBUG -fsanitize=address -fsanitize=undefined -D_GLIBCXX_DEBUG
debug: TopN.cpp Top10.cpp Common.cpp
	$(CXX) $(CXXFLAGS) TopN.cpp Top10.cpp Common.cpp  -o Top10_debug

.PHONY: debug