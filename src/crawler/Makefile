CXX = g++ 
CXXFLAGS = --std=c++20 -Wall -pedantic -O3 -ggdb -O0 -I/opt/homebrew/opt/openssl@3/include -I.
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib
LIBS = -ldl -lssl -lcrypto -pthread

crawler: crawler3.cpp ../../HtmlParser/HtmlTags.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o $@

dispatcher: dispatcher.cpp $(CXXFLAGS)

dispatcher2: dispatcher.cpp -lcrypto -pthread --std=c++20 -g -ldl -lssl -o dispatcher

script: script.cpp 

clean:
	rm -rf crawler debug dispatcher

debug: CXXFLAGS += -g3 -DDEBUG -fsanitize=address -fsanitize=undefined -D_GLIBCXX_DEBUG
debug: crawler2.cpp ../../HtmlParser/HtmlTags.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o debug