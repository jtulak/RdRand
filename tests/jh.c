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



#define THREADS     2 // in how many threads to run
#define CYCLES      2 // how many times all methods should be run
#define SECONDS     2 // how long should be each method generating before stopped
#define CHUNK       2*1024 // size of chunk (how many bytes will be generated in each run)


/**
 * List of methods available for testing.
 * THIS IS LIST OF EXISTING METHODS,
 * NOT METHODS USED IN THE TEST!
 */
enum
{
	GET_BYTES,
	GET_UINT8_ARRAY,
	GET_UINT16_ARRAY,
	GET_UINT32_ARRAY,
	GET_UINT64_ARRAY,

	GET_RDRAND16_STEP,
	GET_RDRAND32_STEP,
	GET_RDRAND64_STEP,

	GET_RDRAND16_RETRY,
	GET_RDRAND32_RETRY,
	GET_RDRAND64_RETRY,

	GET_RESEED64_DELAY,
	GET_RESEED64_SKIP,

	// helper constants
	METHODS_COUNT
};
/**
 * List of names of methods for printing.
 * Has to be in the same order as in the enum.
 */
const char *METHOD_NAMES[] =
{
	"get_bytes",
	"get_uint8_array",
	"get_uint16_array",
	"get_uint32_array",
	"get_uint64_array",

	"rdrand16_step",
	"rdrand32_step",
	"rdrand64_step",

	"rdrand16_retry",
	"rdrand32_retry",
	"rdrand64_retry",

	"uint64_reseed_delay",
	"uint64_reseed_skip_1024",
};

const int tested_methods[METHODS_COUNT+1] =
/************************************************************************
* What method should be tested - GET_BYTES, ...                        *
* Set -1 as the last item so the program can find where the list ends  *
************************************************************************/
{

	GET_BYTES,
	GET_UINT8_ARRAY,
	GET_UINT16_ARRAY,
	GET_UINT32_ARRAY,
	GET_UINT64_ARRAY,

	GET_RDRAND16_STEP,
	GET_RDRAND32_STEP,
	GET_RDRAND64_STEP,

	GET_RDRAND16_RETRY,
	GET_RDRAND32_RETRY,
	GET_RDRAND64_RETRY,


	GET_RESEED64_DELAY,
	GET_RESEED64_SKIP,

	-1,
};

/************************************************************************
*                           METHODS DEFINITIONS                        *
************************************************************************/

/** ***************************************************************
 * fill with _step functions
 */
int fill(uint64_t* buf, const int method, int size, int retry_limit)
{
	int j,k;
	int rc;

	for (j=0; j<size; ++j)
	{
		k = 0;
		do
		{
			switch(method)
			{
			case GET_RDRAND16_STEP:
				rc = rdrand16_step ((uint16_t*) &buf[j] );
				break;
			case GET_RDRAND32_STEP:
				rc = rdrand32_step ((uint32_t*) &buf[j] );
				break;
			case GET_RDRAND64_STEP:
				rc = rdrand64_step ( &buf[j] );
				break;
			}
			++k;
		}
		while ( rc == 0 && k < retry_limit);
	}
	return size;
}

/** ***************************************************************
 * fill with _retry functions
 */
int fill_retry(uint64_t* buf, const int method, int size, int retry_limit)
{
	int j;
	int rc;

	for (j=0; j<size; ++j)
	{
		switch(method)
		{
		case GET_RDRAND16_RETRY:
			rc = rdrand_get_uint16_retry ((uint16_t*) &buf[j], retry_limit);
			break;
		case GET_RDRAND32_RETRY:
			rc = rdrand_get_uint32_retry ((uint32_t*) &buf[j], retry_limit);
			break;
		case GET_RDRAND64_RETRY:
			rc = rdrand_get_uint64_retry ( &buf[j], retry_limit);
			break;
		}
		if (rc == RDRAND_FAILURE)
			break;
	}
	return size;
}


/** ***************************************************************
 * get pressed key
 */
int getkey()
{
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

/**
 * \param threads - number of threads to run in
 * \param chunk - size of chunk to generate
 * \param stop_after - seconds of generating the chunk. \
 *          Zero to generate only once, negative to infinite (until ESC is pressed)
 * \param stream - stream to write data
 * \param type - which function should be used? GET_UINT8_ARRAY, ...
 * \return throughput in MB/s
 */
double test_throughput(const int threads, const size_t chunk, int stop_after, FILE *stream, const int type)
{


	size_t written, total;
	uint64_t buf[threads*chunk];
	omp_set_num_threads(threads);
	struct timespec t[2];
	double run_time, throughput;
	int key;

	total = 0;
	clock_gettime(CLOCK_REALTIME, &t[0]);

	fprintf(stderr, "Press [Esc] to stop the loop");
	do
	{
		written = 0;
#pragma omp parallel for reduction(+:written)
		for (int i=0; i<threads; ++i)
		{
			/****************************************************************
			*                      TESTED METHODS INSERT HERE              *
			****************************************************************/
			switch(type)
			{
			case GET_BYTES:
				written += rdrand_get_bytes_retry((uint8_t*)&buf[i*chunk], chunk*8,1)/8;
				break;
			case GET_UINT8_ARRAY:
				written += rdrand_get_uint8_array_retry((uint8_t*)&buf[i*chunk], chunk*8, 1)/8;
				break;
			case GET_UINT16_ARRAY:
				written += rdrand_get_uint16_array_retry((uint16_t*)&buf[i*chunk], chunk*4, 1)/4;
				break;
			case GET_UINT32_ARRAY:
				written += rdrand_get_uint32_array_retry((uint32_t*)&buf[i*chunk], chunk*2, 1)/2;
				break;
			case GET_UINT64_ARRAY:
				written += rdrand_get_uint64_array_retry(&buf[i*chunk], chunk, 1);
				break;


			case GET_RDRAND16_STEP:
			case GET_RDRAND32_STEP:
			case GET_RDRAND64_STEP:
				written += fill(&buf[i*chunk], type, chunk, 1);
				break;
			case GET_RDRAND16_RETRY:
			case GET_RDRAND32_RETRY:
			case GET_RDRAND64_RETRY:
				written += fill_retry(&buf[i*chunk], type, chunk, 1);
				break;

			case GET_RESEED64_DELAY:
				written += rdrand_get_uint64_array_reseed_delay(&buf[i*chunk], chunk, 1);
				break;
			case GET_RESEED64_SKIP:
				written += rdrand_get_uint64_array_reseed_skip(&buf[i*chunk], chunk, 1);
				break;
			}
		}
		/* test generated amount */
		if ( written != SIZEOF(buf) )
		{
			fprintf(stderr, "ERROR: bytes generated %zu, bytes expected %zu\n", written, SIZEOF(buf));
			break;
		}

		/* Test written amount */
		written = fwrite(buf, sizeof(buf[0]), SIZEOF(buf), stream);
		total += written;
		if ( written <  SIZEOF(buf) )
		{
			perror("fwrite");
			fprintf(stderr, "ERROR: fwrite - bytes written %zu, bytes to write %zu\n", sizeof(buf[0]) * written, sizeof(buf));
			break;
		}

		/* Stopping */
		key = getkey();

		if (stop_after >= 0)
		{
			clock_gettime(CLOCK_REALTIME, &t[1]);
			run_time = (double)(t[1].tv_sec) - (double)(t[0].tv_sec) +
				   ( (double)(t[1].tv_nsec) - (double)(t[0].tv_nsec) ) / 1.0E9;
			if(run_time > stop_after)
				key = 0x1B;
		}
	}
	while (key != 0x1B);

	if(stop_after < 0) // compute only if it wasn't already computed
	{
		clock_gettime(CLOCK_REALTIME, &t[1]);
		run_time = (double)(t[1].tv_sec) - (double)(t[0].tv_sec) +
			   ( (double)(t[1].tv_nsec) - (double)(t[0].tv_nsec) ) / 1.0E9;
	}
	throughput = (double) (total) * sizeof(buf[0]) / run_time/1024.0/1024.0;
	fprintf(stderr, "\r  Runtime %.2f sec, throughput %.3f MiB/s\n", run_time, throughput);

	return throughput;
}

/** ***************************************************************
 * Compute amount of tests according to content of methods array
 */
int compute_tests_amount(const int *methods)
{
	int amount;
	for(amount=0; amount < METHODS_COUNT; amount++)
	{
		if(methods[amount] == -1)
			break;
	}
	return amount;
}

/** ***************************************************************
 * MAIN
 */
int main(int argc, char **argv)
{
	const int threads=THREADS;
	const size_t chunk = CHUNK;
	const int test_length = SECONDS;  /* how many seconds before stopping generation
	                                   * negative to infinite (until ESC) */
	const int cycles = CYCLES; // how many times to run generators to get average throughput




	double throughputs [cycles][METHODS_COUNT];
	double averages [METHODS_COUNT];
	double sum, max_throughput = 0;
	int max_throughput_method;
	const int tested_methods_count = compute_tests_amount(tested_methods);

	if ( argc != 2)
	{
		fprintf(stderr, "Usage %s output_file\n", argv[0]);
		return 1;
	}

	FILE *stream = fopen(argv[1], "w");
	if ( !stream )
	{
		fprintf(stderr, "Error on fopen for file in argv[1] \"%s\"\n", argv[1]);
		return 1;
	}

	fprintf(stderr,"This test will run %d methods for %d times in %d threads.\n", tested_methods_count, CYCLES, THREADS);
	fprintf(stderr,"Each method will run for %d seconds, the overall time of this test is %d seconds.\n",
		SECONDS, SECONDS*CYCLES*tested_methods_count );
	fprintf(stderr,"-------------------------------------------------------------------\n");

	/************** DO THE TESTING ******************************************/
	/* run all methods in required count */
	for (int cycle = 0; cycle < cycles; cycle++)
	{
		fprintf(stderr,"\nDoing %d. run:\n",cycle+1);
		/* run all methods */
		for(int method = 0; method < tested_methods_count; method++)
		{
			fprintf(stderr,"%s:\n",METHOD_NAMES[tested_methods[method]]);
			throughputs[cycle][method] = test_throughput(threads, chunk, test_length, stream, tested_methods[method]);
		}
	}

	/************** PRINT OVERALL RESULTS **********************************/

	fprintf(stderr,"\n-------------------------------------------------------------------\n");
	for(int method = 0; method< tested_methods_count; method++)
	{
		sum = 0;
		for(int cycle = 0; cycle < cycles; cycle++)
		{
			sum += throughputs[cycle][method];
		}
		averages[method] = sum/CYCLES;
		// find the maximum throughput
		if(max_throughput < averages[method])
		{
			max_throughput = averages[method];
			max_throughput_method = tested_methods[method];
		}
	}
	fprintf(stderr,"The fastest method (%g MiB/s) was: %s\n",max_throughput, METHOD_NAMES[max_throughput_method]);

	fprintf(stderr,"Average throughputs in %d runs:\n", cycles);
	for(int method = 0; method< tested_methods_count; method++)
	{
		fprintf(stderr,"  (%.2f %%) Method %s: %.3f MiB/s  \n",

			(averages[method]/max_throughput)*100, // percents of max throughput
			METHOD_NAMES[tested_methods[method]],
			averages[method]
			);
	}



	fclose(stream);
	return 1;
}
