CC=gcc
CFLAGS=-c -Wall  -Wextra -O3 -fopenmp
LDFLAGS=-fopenmp -mrdrnd -lrt -lm
SOURCES_COMMON=../src/librdrand.h\
               ../src/librdrand.c
OBJECTS_COMMON=$(SOURCES_COMMON:.c=.o)           
               
SOURCES_TEST=       test_throughput.c
OBJECTS_TEST=$(SOURCES_TEST:.c=.o)
EXECUTABLE_TEST=RdRand

SOURCES_GEN=   ../src/rdrand-gen.h\
               ../src/rdrand-gen.c
OBJECTS_GEN=$(SOURCES_GEN:.c=.o)
EXECUTABLE_GEN=rdrand-gen

.PHONY: all RdRand rdrand-gen

all: rdrand rdrand-gen

rdrand: $(OBJECTS_COMMON) $(OBJECTS_TEST)
	$(CC) $(LDFLAGS) $(OBJECTS_COMMON) $(OBJECTS_TEST) -o $(EXECUTABLE_TEST)

rdrand-gen: $(OBJECTS_COMMON) $(OBJECTS_GEN)
	$(CC) $(LDFLAGS) $(OBJECTS_COMMON) $(OBJECTS_GEN) -o $(EXECUTABLE_GEN)
	
# $(EXECUTABLE): $(OBJECTS) 
#	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

run:
	./run-test.sh

#test-gen:
#test-throughput:


clean:
	rm $(EXECUTABLE_TEST) $(EXECUTABLE_GEN) *.o ../src/*.o
