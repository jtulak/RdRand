CC=gcc
CFLAGS=-DSTUB_RDRAND -DNO_MAIN -DNO_ERROR_PRINTS -c -Wall -Wextra -O3 -fopenmp  -fPIC
LDFLAGS=-fopenmp -lrt -lm -mrdrnd -lcheck

SRCS=../src/librdrand.c\
     ../src/librdrand-aes.c\
     ../src/rdrand-gen.c

OSRCS=$(SRCS:.c=.o)

all: check

check: check_aes check_rdrand-gen check_rdrand 
	./check_rdrand
	./check_rdrand-gen
	./check_aes

check_aes: $(OSRCS) check_aes.o
	$(CC) $(LDFLAGS)  $(OSRCS) $@.o -o $@


check_rdrand: $(OSRCS) check_rdrand.o
	$(CC) $(LDFLAGS)  $(OSRCS) $@.o -o $@


check_rdrand-gen: $(OSRCS) check_rdrand-gen.o
	$(CC) $(LDFLAGS) $(OSRCS) $@.o -o $@


.c.o: $(OSRCS)
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm check_rdrand check_aes *.o ../src/*.o

