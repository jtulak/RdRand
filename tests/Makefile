CC=gcc
CFLAGS=-c -Wall  -Wextra -O3 
LDFLAGS=-fopenmp -mrdrnd 
SOURCES=test_throughput.c ../src/rdrand.h ../src/rdrand.c
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=RdRand

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

run:
	./run-test.sh
