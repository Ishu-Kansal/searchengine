CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

# Common source files used by all driver*.cpp targets
COMMON_SRC = \
  sequences.cpp \
  query_parser/expression.cpp \
  query_parser/parser.cpp \
  query_parser/tokenstream.cpp 
# All driver*.cpp files in src/
DRIVER_SRCS := $(wildcard driver*.cpp)

# Convert src/driverX.cpp → driverX (executable names)
TARGETS := $(basename $(notdir $(DRIVER_SRCS)))

all: $(TARGETS)

# Each driver target depends on its .cpp file and common code
$(TARGETS): %: %.cpp $(COMMON_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(TARGETS)
