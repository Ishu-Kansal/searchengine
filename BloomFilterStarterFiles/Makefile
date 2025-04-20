CXX = g++ 
CXXFLAGS = -std=c++17 -Wall -Werror -Wno-deprecated-declarations -pedantic -O3 -I/opt/homebrew/opt/openssl@3/include
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib
LIBS = -ldl -lssl -lcrypto

bf: main2.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf bf debug

debug: CXXFLAGS += -g3 -DDEBUG -fsanitize=address -fsanitize=undefined -D_GLIBCXX_DEBUG
debug: main.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o debug
