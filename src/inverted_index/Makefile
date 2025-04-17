CXX = g++ 
CXXFLAGS = -std=c++20 -Wall -pedantic -I/opt/homebrew/opt/openssl@3/include
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib
LIBS = -ldl -lssl -lcrypto

test: testIndexFile.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o $@


clean:
	rm -rf test debug

debug: CXXFLAGS += -g3 -DDEBUG -fsanitize=address -fsanitize=undefined -D_GLIBCXX_DEBUG
debug: testIndexFile.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o debug
