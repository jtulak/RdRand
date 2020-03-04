CC=gcc
CFLAGS=-c -Wall  -Wextra  -fopenmp  -fPIC -O0 -g #-DSTUB_RDRAND
LDFLAGS=-fopenmp  -lm -mrdrnd -lcrypto

CUSTOM_LIBS=

UNAME_S := $(shell uname -s)
# OSX support
ifeq ($(UNAME_S),Darwin)
	CC=gcc-mp-4.8
	CFLAGS += -D OSX
	LDFLAGS += -Wl,-rpath,/opt/local/lib/libcrypto.dylib  -L/opt/local/lib
endif
ifeq ($(UNAME_S),Linux)
	LDFLAGS += -lrt 
endif

SOURCES_COMMON=../src/librdrand.h\
               ../src/librdrand-aes.h\
               ../src/librdrand.c\
               ../src/librdrand-aes.c
OBJECTS_COMMON=$(SOURCES_COMMON:.c=.o) 
               
SOURCES_TEST=       test_throughput.c
OBJECTS_TEST=$(SOURCES_TEST:.c=.o)
EXECUTABLE_TEST=RdRand

SOURCES_GEN=   ../src/rdrand-gen.h\
               ../src/rdrand-gen.c
OBJECTS_GEN=$(SOURCES_GEN:.c=.o)
EXECUTABLE_GEN=rdrand-gen

.PHONY: all RdRand rdrand-gen

all: librdrand.so rdrand rdrand-gen

rdrand: $(OBJECTS_TEST)
	$(CC) $(LDFLAGS) -lrdrand  -Wl,-rpath,. -L. $(OBJECTS_TEST) -o $(EXECUTABLE_TEST)

rdrand-gen:  $(OBJECTS_GEN)
	$(CC) $(LDFLAGS) -lrdrand  -Wl,-rpath,. -L. $(OBJECTS_GEN) -o $(EXECUTABLE_GEN)
	
# $(EXECUTABLE): $(OBJECTS) 
#	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

librdrand.so: $(OBJECTS_COMMON)
	$(CC) $(LDFLAGS) -shared $(OBJECTS_COMMON) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

run:
	./run-test.sh

testu01:
	$(CC) -Wall -Wextra -O2 -o TestU01_raw_stdin_input_with_log TestU01_raw_stdin_input_with_log.c -ltestu01


#test-gen:
#test-throughput:


clean:
	-rm $(EXECUTABLE_TEST) $(EXECUTABLE_GEN) *.o ../src/*.o *.so
