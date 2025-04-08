CXX = g++
CXXFLAGS = -pthread -Wall -Wextra -std=c++17

SOURCES = LinuxTinyServer.cpp engine.cpp
HEADERS = List.h Mutex.h Plugin.h
TARGET = engine

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

