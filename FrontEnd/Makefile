CXX = g++
CXXFLAGS = -pthread -Wall -Wextra -std=c++17 -O3 \
    -I/opt/homebrew/opt/openssl@3/include

LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto

SOURCES = \
    LinuxTinyServer.cpp \
    engine.cpp \
    ../src/driver.cpp \
    ../HtmlParser/HtmlTags.cpp \
    ../src/query_parser/tokenstream.cpp \
    ../src/query_parser/parser.cpp \


TARGET = engine

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
