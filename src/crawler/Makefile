# crawler: crawler.cpp ../../BloomFilterStarterFiles/BloomFilter.h 
# 	../../HtmlParser/HtmlParser.h 
# 	../inverted_index/Index.h 
# 	../../utils/pthread_lock_guard.h
# 	../../utils/unrolled_linked_list.h
# 	../../utils/cunique_ptr.h
# 	../../HashTable/HashTableStarterFiles/HashTable.h
# 	g++ -std=c++20 crawler.cpp -o crawler


# CC+=-g -Wall -fno-builtin -std=c++20




CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -MMD -MP

SRC = crawler2.cpp ../../HtmlParser/HtmlTags.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)

all: crawler

crawler: $(OBJ)
	$(CXX) -o crawler $(OBJ) -lssl -lcrypto -lz 

crawler2: $(OBJ)
	$(CXX) -o crawler2 $(OBJ) -lssl -lcrypto -lz 

-include $(DEP)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.d crawler crawler2
