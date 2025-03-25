CXX = g++
CXXFLAGS = -std=c++11 -g

all: LinuxGetSsl LinuxGetUrl

LinuxGetSsl: LinuxGetSsl.cpp
	$(CXX) $(CXXFLAGS) $^ -lssl -lcrypto -lz -o LinuxGetSsl

LinuxGetSsl2: LinuxGetSsl2.cpp
	$(CXX) $(CXXFLAGS) $^ -lssl -lcrypto -lz -o LinuxGetSsl2
LinuxGetSsl3: LinuxGetSsl3.cpp
	$(CXX) $(CXXFLAGS) $^ -lssl -lcrypto -lz -o LinuxGetSsl3

LinuxGetUrl: LinuxGetUrl.cpp
	$(CXX) $(CXXFLAGS) $^ -o LinuxGetUrl

.PHONY: clean

clean:
	rm -f LinuxGetSsl LinuxGetUrl LinuxGetSsl2 LinuxGetSsl3
