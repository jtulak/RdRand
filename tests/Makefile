CC=gcc
CFLAGS=-c -Wall  -Wextra -O3 -fopenmp
LDFLAGS=-fopenmp -mrdrnd -lrt
SOURCES=test_throughput.c ../src/rdrand.h ../src/rdrand.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=RdRand

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

run:
	./run-test.sh

clean:
	rm $(EXECUTABLE) *.o ../src/*.o
