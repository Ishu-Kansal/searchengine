CXX = g++
CXXFLAGS = -std=c++20 -Wall -pedantic -O0 -g -fsanitize=address -fsanitize=undefined -I/opt/homebrew/opt/openssl@3/include
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib
LIBS = -ldl -lssl -lcrypto

# Common source files used by all driver*.cpp targets
COMMON_SRC = \
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
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o $@ 

clean:
	rm -f $(TARGETS) debug

debug: CXXFLAGS += -g3 -DDEBUG -D_GLIBCXX_DEBUG
debug: drivertest1.cpp $(COMMON_SRC)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o debug


