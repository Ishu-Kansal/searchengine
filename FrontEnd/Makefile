CXX = g++
CXXFLAGS = -pthread -Wall -Wextra -std=c++17 \
    -I/opt/homebrew/opt/openssl@3/include

LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto

SOURCES = LinuxTinyServer.cpp engine.cpp ../src/driver.cpp ../HtmlParser/HtmlTags.cpp
HEADERS = \
    List.h \
    Mutex.h \
    Plugin.h \
    ../HtmlParser/HtmlTags.h \
    ../HtmlParser/HtmlParser.h \
    ../src/crawler/sockets.h

TARGET = engine

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
