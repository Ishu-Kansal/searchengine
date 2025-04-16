CXX = g++ 
CXXFLAGS = -std=c++20 -Wall -pedantic -O3 -I/opt/homebrew/opt/openssl@3/include
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib
LIBS = -ldl -lssl -lcrypto

crawler: crawler3.cpp ../../HtmlParser/HtmlTags.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o $@

dispatcher: dispatcher.cpp $(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf crawler debug dispatcher

debug: CXXFLAGS += -g3 -DDEBUG -fsanitize=address -fsanitize=undefined -D_GLIBCXX_DEBUG
debug: crawler2.cpp ../../HtmlParser/HtmlTags.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o debug
