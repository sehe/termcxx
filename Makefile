all:parser

CPPFLAGS+=-std=c++11 -Wall -pedantic
CPPFLAGS+=-g -O2 -pthread
CPPFLAGS+=-isystem /home/sehe/custom/boost

%.o:%.cpp
	$(CXX) $(CPPFLAGS) $^ -c -o $@

%:%.o
	$(CXX) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)
