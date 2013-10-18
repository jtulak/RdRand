/* vim: set expandtab cindent fdm=marker ts=2 sw=2: */

/*
gcc -DRDRAND_LIBRARY -DHAVE_X86INTRIN_H -Wall -Wextra -std=gnu99 -fopenmp -mrdrnd -I../src -O3 -o jh_with_library jh.c ../src/rdrand.c -lssl -lcrypto -lrt -lcurses

gcc -DHAVE_X86INTRIN_H -Wall -Wextra -std=gnu99 -fopenmp -mrdrnd -I../src -O3 -o jh_standalone jh.c ../src/rdrand.c -lssl -lcrypto -lrt -lcurses


./jh_standalone >(pv >/dev/null )
./jh_with_library >(pv >/dev/null )

./jh_standalone /dev/null
./jh_with_library /dev/null
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <omp.h>
#include <time.h>
#include <termios.h>
#include <string.h>
//#include <rdrand.h>
#include "../src/rdrand.h"


#define SIZEOF(a) ( sizeof (a) / sizeof (a[0]) )

#define RDRAND_LIBRARY

#ifndef RDRAND_LIBRARY
#include <x86intrin.h>
int fill(uint64_t* buf, int size, int retry_limit) {
	int j,k;
	int rc;

	for (j=0; j<size; ++j) {
		k = 0;
		do {
			rc = _rdrand64_step ( (long long unsigned*)&buf[j] );
			++k;
		} while ( rc == 0 && k < retry_limit);
		if ( rc == 0 ) return 1;
	}
	return 0;
}
#endif

int getkey() {
	int character;
	struct termios orig_term_attr;
	struct termios new_term_attr;

	/* set the terminal to raw mode */
	tcgetattr(fileno(stdin), &orig_term_attr);
	memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
	new_term_attr.c_lflag &= ~(ECHO|ICANON);
	new_term_attr.c_cc[VTIME] = 0;
	new_term_attr.c_cc[VMIN] = 0;
	tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

	/* read a character from the stdin stream without blocking */
	/*	 returns EOF (-1) if no character is available */
	character = fgetc(stdin);

	/* restore the original terminal attributes */
	tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

	return character;
}


int main(int argc, char **argv) {
	size_t written, total;
	const int threads=2;
	const size_t chunk = 2*1024;
	uint64_t buf[threads*chunk];
	omp_set_num_threads(threads);
	struct timespec t[2];
	double run_time, throughput;
	int key;

	if ( argc != 2) {
		fprintf(stderr, "Usage %s output_file\n", argv[0]);
		return 1;
	}

	FILE *stream = fopen(argv[1], "w");
	if ( !stream ) {
		fprintf(stderr, "Error on fopen for file in argv[1] \"%s\"\n", argv[1]);
		return 1;
	}

	total = 0;
	clock_gettime(CLOCK_REALTIME, &t[0]);

	fprintf(stderr, "Press [Esc] to stop the loop\n");
	do {
#ifdef RDRAND_LIBRARY
		written = 0;
#pragma omp parallel for reduction(+:written)
		for (int i=0; i<threads; ++i) {
			written += rdrand_get_uint8_array_retry((uint8_t*)&buf[i*chunk], chunk, 1);
			//written += rdrand_get_uint32_array_retry((uint32_t*)&buf[i*chunk], chunk, 1);
			//written += rdrand_get_uint64_array_retry(&buf[i*chunk], chunk, 1);
			//written += rdrand_get_bytes_retry(&buf[i*chunk], chunk,1);
		}
		if ( written != SIZEOF(buf) ) {
			fprintf(stderr, "ERROR:rdrand_get_uint64_array_retry	- bytes generated %zu, bytes expected %zu\n", written , SIZEOF(buf));
			break;
		}
#else
		int rc=0;
#pragma omp parallel for reduction(+:rc)
		for (int i=0; i<threads; ++i) {
			rc += fill(&buf[i*chunk], chunk, 1);
		}
		if ( rc > 0 ) {
			fprintf(stderr, "ERROR: \"fill\" function\n");
			break;
		}

#endif
		written = fwrite(buf, sizeof(buf[0]), SIZEOF(buf), stream);
		total += written;
		if ( written <	SIZEOF(buf) ) {
			perror("fwrite");
			fprintf(stderr, "ERROR: fwrite - bytes written %zu, bytes to write %zu\n", sizeof(buf[0]) * written , sizeof(buf));
			break;
		}
		key = getkey();
	} while (key != 0x1B);

	clock_gettime(CLOCK_REALTIME, &t[1]);
	run_time = (double)(t[1].tv_sec) - (double)(t[0].tv_sec) +
		( (double)(t[1].tv_nsec) - (double)(t[0].tv_nsec) ) / 1.0E9;
	throughput = (double) (total) * sizeof(buf[0]) / run_time/1024.0/1024.0;
	fprintf(stderr, "Runtime %g, throughput %gMiB/s\n", run_time, throughput);

	fclose(stream);
	return 1;
}
