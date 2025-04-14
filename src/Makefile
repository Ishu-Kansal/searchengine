CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

# Common source files used by all driver*.cpp targets
COMMON_SRC = \
  globals.cpp \
  ../HtmlParser/HtmlTags.cpp \
  query_parser/parser.cpp \
  query_parser/tokenstream.cpp 
# All driver*.cpp files in src
DRIVER_SRCS := $(wildcard driver*.cpp)

# Convert src/driverX.cpp → driverX (executable names)
TARGETS := $(basename $(notdir $(DRIVER_SRCS)))

all: $(TARGETS)

# Each driver target depends on its .cpp file and common code
$(TARGETS): %: %.cpp $(COMMON_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lssl -lcrypto

clean:
	rm -f $(TARGETS)
