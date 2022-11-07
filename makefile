CXX=g++
CXXFLAGS=-std=c++17 -g -pedantic -Wall -Wextra -fsanitize=address,undefined -fno-omit-frame-pointer
LDLIBS=

# 0 for output in autograder, 1 for no output in autograder
OUT=1


SRCS=server.cpp client.cpp
DEPS=BoundedBuffer.cpp common.cpp FIFORequestChannel.cpp Histogram.cpp HistogramCollection.cpp
BINS=$(SRCS:%.cpp=%.exe)
OBJS=$(DEPS:%.cpp=%.o)


all: clean $(BINS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.exe: %.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(patsubst %.exe,%,$@) $^ $(LDLIBS)


.PHONY: clean print-var test

clean:
	make -C test-files/ clean
	rm -f server client fifo* data*_* *.tst *.o *.csv received/*

print-var:
	echo $(OUT)

test: all
	cp BoundedBuffer.* test-files/
	make -C test-files/
	chmod u+x pa3-tests.sh
	./pa3-tests.sh
