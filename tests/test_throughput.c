/* vim: set expandtab cindent fdm=marker ts=2 sw=2: */
/*
 * Copyright (C) 2013  Jan Tulak <jan@tulak.me>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
    Now the legal stuff is done. This file contain a performance test for the library.

    Manual compiling:
    gcc -DHAVE_X86INTRIN_H -Wall -Wextra -fopenmp -mrdrnd -I../src -O3 -o RdRand jh.c ../src/rdrand.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <omp.h>
#include <time.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // usleep
#include <inttypes.h>
//#include <rdrand.h>
#include "../src/librdrand.h"
//#include <rdrand-0.1/rdrand.h>


#define SIZEOF(a) ( sizeof (a) / sizeof (a[0]) )

#define THREADS     2 // in how many threads to run
#define CYCLES      2 // how many times all methods should be run
#define SECONDS     3 // how long should be each method generating before stopped
#define CHUNK       2*1024 // size of chunk (how many bytes will be generated in each run)


#define RETRY_LIMIT 10
#define SLOW_RETRY_LIMIT_CYCLES 100
#define SLOW_RETRY_LIMIT 1000
#define SLOW_RETRY_DELAY 1000 // 1 ms

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
	"rdrand_get_bytes_retry",
	"rdrand_get_uint8_array_retry",
	"rdrand_get_uint16_array_retry",
	"rdrand_get_uint32_array_retry",
	"rdrand_get_uint64_array_retry",

	"rdrand16_step",
	"rdrand32_step",
	"rdrand64_step",

	"rdrand_get_uint16_retry",
	"rdrand_get_uint32_retry",
	"rdrand_get_uint64_retry",

	"rdrand_get_uint64_array_reseed_delay",
	"rdrand_get_uint64_array_reseed_skip",
};

int TESTED_METHODS[METHODS_COUNT+1] =
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

static const char* HELP_TEXT =
	"Usage: %s [OUTPUT_FILE] [OPTIONS]\n"
	"If no OUTPUT_FILE is specified, the program will print numbers to stdout.\n\n"
	"OPTIONS\n"
	"  --help       -h      Print this help\n"
	"  --verbose    -v      Be verbose\n"
	"  --print      -p      Do really print the generated numbers, instead just measure the speed.\n"
	"  --numbers    -n      Print generated values as numbers, not raw, MAY IMPACT PERFORMANCE (default will print raw)\n"
	"  --method     -m NAME Will test only method NAME (default all)\n"
	"  --threads    -t NUM  Will run the generator in NUM threads (default %u)\n"
	"  --duration   -d NUM  Each tested method will run for NUM seconds (default %u)\n"
	"  --repetition -r NUM  All tests will be run for NUM times (default %u)\n"
	"  --chunk-size -c NUM  The size of the chunk generated at once as count of 64 bit numbers (default %u)\n"
	"";
static int verbose_flag = 0;
static int print_numbers_flag = 0;
static int no_print_flag = 1;

/************************************************************************
*                           METHODS DEFINITIONS                        *
************************************************************************/
/**
 * Print data given in ptr with size of bytes as a hexa number
 */
unsigned int print_numbers(FILE*destination, char * ptr, size_t bytes)
{
	size_t i;
	//printf("GUUUUUUUUUUUUU: %u\n",(unsigned int)bytes);
	for(i=bytes; i>0; i--)
	{
		fprintf(destination,"%hhx",ptr[i]);
	}
	return bytes-i;
}

/** ***************************************************************
 * fill with _step functions
 */
int fill_uint16_step(uint16_t* buf, int size, int retry_limit)
{
	int j,k;
	int rc;

	for (j=0; j<size; ++j)
	{
		k = 0;
		do
		{
			rc = rdrand16_step ( &buf[j] );
			++k;
		}
		while ( rc == RDRAND_FAILURE && k < retry_limit);
	}
	return size;
}
int fill_uint32_step(uint32_t* buf, int size, int retry_limit)
{
	int j,k;
	int rc;

	for (j=0; j<size; ++j)
	{
		k = 0;
		do
		{
			rc = rdrand32_step (&buf[j] );
			++k;
		}
		while ( rc == RDRAND_FAILURE && k < retry_limit);
	}
	return size;
}
int fill_uint64_step(uint64_t* buf, int size, int retry_limit)
{
	int j,k;
	int rc;

	for (j=0; j<size; ++j)
	{
		k = 0;
		do
		{
			rc = rdrand64_step (&buf[j] );
			++k;
		}
		while ( rc == RDRAND_FAILURE && k < retry_limit);
	}
	return size;
}

/** ***************************************************************
 * fill with _retry functions
 */
// TODO rewrite, too high overhead
int fill_uint16_retry(uint16_t* buf,  int size, int retry_limit)
{
	int j;
	int rc;

	for (j=0; j<size; ++j)
	{
		rc = rdrand_get_uint16_retry ( &buf[j], retry_limit);
		if (rc == RDRAND_FAILURE)
			break;
	}
	return size;
}
int fill_uint32_retry(uint32_t* buf, int size, int retry_limit)
{
	int j;
	int rc;

	for (j=0; j<size; ++j)
	{
		rc = rdrand_get_uint32_retry ( &buf[j], retry_limit);
		if (rc == RDRAND_FAILURE)
			break;
	}
	return size;
}
int fill_uint64_retry(uint64_t* buf, int size, int retry_limit)
{
	int j;
	int rc;

	for (j=0; j<size; ++j)
	{
		rc = rdrand_get_uint64_retry ( &buf[j], retry_limit);
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


size_t generate_with_metod( int type, uint64_t *buf, unsigned int blocks, int retry)
{
	switch(type)
	{
	case GET_BYTES:
		return rdrand_get_bytes_retry((uint8_t*)buf, blocks*8,retry)/8;
		break;
	case GET_UINT8_ARRAY:
		return rdrand_get_uint8_array_retry((uint8_t*)buf, blocks*8, retry)/8;
		break;
	case GET_UINT16_ARRAY:
		return rdrand_get_uint16_array_retry((uint16_t*)buf, blocks*4, retry)/4;
		break;
	case GET_UINT32_ARRAY:
		return rdrand_get_uint32_array_retry((uint32_t*)buf, blocks*2, retry)/2;
		break;
	case GET_UINT64_ARRAY:
		return rdrand_get_uint64_array_retry(buf, blocks, retry);
		break;


	case GET_RDRAND16_STEP:
		return fill_uint16_step((uint16_t *)buf, blocks*4, retry)/4;
		break;
	case GET_RDRAND32_STEP:
		return fill_uint32_step((uint32_t *)buf, blocks*2, retry)/2;
		break;
	case GET_RDRAND64_STEP:
		return fill_uint64_step(buf, blocks, retry);
		break;

	case GET_RDRAND16_RETRY:
		return fill_uint16_retry((uint16_t *)buf, blocks*4, retry)/4;
		break;
	case GET_RDRAND32_RETRY:
		return fill_uint32_retry((uint32_t *)buf, blocks*2, retry)/2;
		break;
	case GET_RDRAND64_RETRY:
		return fill_uint64_retry(buf, blocks, retry);
		break;

	case GET_RESEED64_DELAY:
		return rdrand_get_uint64_array_reseed_delay(buf, blocks, retry);
		break;
	case GET_RESEED64_SKIP:
		return rdrand_get_uint64_array_reseed_skip(buf, blocks, retry);
		break;
	}
	return 0;
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
double test_throughput( int threads, const size_t chunk, int stop_after, FILE *stream, const int type)
{
	size_t written, total,buf_size;
	uint64_t buf[threads*chunk];
    //char* buf_ptr;
    //uint64_t* buf;
    //size_t size;
	omp_set_num_threads(threads);
	struct timespec t[2];
	double run_time, throughput;
	int key,i;
	unsigned int retry;

	//buf_ptr = malloc(threads*chunk*sizeof(uint64_t)+10);
    //buf = (uint64_t*)&(buf_ptr[3]);
	run_time = 0;
	total = 0;
	clock_gettime(CLOCK_REALTIME, &t[0]);
	if(verbose_flag)
		fprintf(stderr, "Press [Esc] to stop the loop");

	key = 0;
	// SIZE threads*chunk;
	buf_size = SIZEOF(buf);
	do
	{
		written = 0;
#if 0
#pragma omp parallel for reduction(+:written)
		for ( i=0; i<threads; ++i)
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
				written += fill_uint16_step((uint16_t *)&buf[i*chunk], chunk*4, 1)/4;
				break;
			case GET_RDRAND32_STEP:
				written += fill_uint32_step((uint32_t *)&buf[i*chunk], chunk*2, 1)/2;
				break;
			case GET_RDRAND64_STEP:
				written += fill_uint64_step(&buf[i*chunk], chunk, 1);
				break;

			case GET_RDRAND16_RETRY:
				written += fill_uint16_retry((uint16_t *)&buf[i*chunk], chunk*4, 1)/4;
				break;
			case GET_RDRAND32_RETRY:
				written += fill_uint32_retry((uint32_t *)&buf[i*chunk], chunk*2, 1)/2;
				break;
			case GET_RDRAND64_RETRY:
				written += fill_uint64_retry(&buf[i*chunk], chunk, 1);
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

		if(!no_print_flag)
		{
			/* Test written amount */
			if(print_numbers_flag == 1)
			{
				written = print_numbers(stream, (char *)buf,sizeof(buf))/8;
				// written = fwrite(, sizeof(buf[0]), SIZEOF(buf), stream);
			}
			else
			{
				written = fwrite(buf, sizeof(buf[0]), SIZEOF(buf), stream);
			}
			total += written;
			if ( written !=  SIZEOF(buf) )
			{
				perror("fwrite");
				fprintf(stderr, "ERROR: fwrite - bytes written %zu, bytes to write %zu\n", sizeof(buf[0]) * written, sizeof(buf));
				break;
			}
		}
		else
		{
			total += written;
		}
#else
		#pragma omp parallel for reduction(+:written)
		for ( i=0; i<threads; ++i)
		{
			written += generate_with_metod(type,&buf[i*chunk], chunk, RETRY_LIMIT);


		}

		if ( written != buf_size )
		{
			/* if we can't lower threads count anymore */
			if ( threads == 1 )
			{
				fprintf(stderr, "Warning: %zu bytes generated, but %zu bytes expected. Trying to get randomness with slower speed.\n", written, buf_size);
				retry = 0;
				while(written != buf_size && retry++ < SLOW_RETRY_LIMIT_CYCLES)
				{
					usleep(retry*SLOW_RETRY_DELAY);
					// try to generate the rest
					written +=generate_with_metod(type,buf+written, chunk-written, SLOW_RETRY_LIMIT);
				}
				if( written != buf_size )
				{
					fprintf(stderr, "Error:  %zu bytes generated, but %zu bytes expected. Probably there is a hardware problem with your CPU.\n", written, buf_size);
					break;
				}
			}
			else
			{
				/* try to lower threads count to avoid underflow */
				threads--;
				fprintf(stderr, "Warning: %zu bytes generated, but %zu bytes expected. Probably slow internal generator - decreaseing threads count by one to %d to avoid problems.\n", written, buf_size, threads);
				buf_size -= chunk;

				continue;
			}
		}
		if(!no_print_flag)
		{

			/* Test written amount */
			if(print_numbers_flag == 1)
			{
				written = print_numbers(stream, (char *)buf,sizeof(buf))/8;
				// written = fwrite(, sizeof(buf[0]), SIZEOF(buf), stream);
			}
			else
			{
				written = fwrite(buf, sizeof(buf[0]), SIZEOF(buf), stream);
			}
			total += written;
			if ( written !=  SIZEOF(buf) )
			{
				perror("fwrite");
				fprintf(stderr, "ERROR: fwrite - bytes written %zu, bytes to write %zu\n", sizeof(buf[0]) * written, sizeof(buf));
				break;
			}
		}
		else
		{
			total += written;
		}




#endif // 0

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
	if(verbose_flag)
	{

		if(print_numbers_flag == 1)
			fprintf(stderr,"\n");
		fprintf(stderr, "\r  Runtime %.2f sec, throughput %.3f MiB/s\n", run_time, throughput);
	}

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
 * get opts
 */

void get_opts(int argc,
	      char **argv,
	      unsigned int* threads,
	      unsigned int* chunk,
	      unsigned int* test_length,
	      unsigned int* cycles, FILE **stream,
	      int* tested_methods_count
	      )
{
	int i;
	char optC;
	static int help_flag;
	static struct option long_options[] =
	{
		/* These options set a flag. */
		{"verbose", no_argument,       0, 'v'},
		{"numbers", no_argument,       0, 'n'},
		{"help", no_argument,       0, 'h'},
		{"print", no_argument,       0, 'p'},
		{"method",  required_argument, 0, 'm'},
		{"threads",  required_argument, 0, 't'},
		{"duration",  required_argument, 0, 'd'},
		{"repetition",    required_argument, 0, 'r'},
		{"chunk-size",    required_argument, 0, 'c'},
		{0, 0, 0, 0}
	};

	opterr = 0;
	while (1)
	{

		/* getopt_long stores the option index here. */
		int option_index = 0;

		optC = getopt_long (argc, argv, "vhnpt:d:r:c:m:",
				    long_options, &option_index);

		/* Detect the end of the options. */
		if (optC == -1)
			break;

		switch (optC)
		{

		case 'v':
			verbose_flag = 1;
			break;

		case 'h':
			help_flag = 1;
			break;

		case 'p':
			no_print_flag = 0;
			break;

		case 'n':
			print_numbers_flag = 1;
			break;

		case 't':
			*threads = strtoumax(optarg, NULL, 10);

			break;

		case 'd':
			*test_length = strtoumax(optarg, NULL, 10);
			break;

		case 'r':
			*cycles = strtoumax(optarg, NULL, 10);
			break;

		case 'c':
			*chunk = strtoumax(optarg, NULL, 10);
			break;


		case 'm': // method
			for(i = 0; i<METHODS_COUNT; i++)
			{
				if(strcmp(optarg,METHOD_NAMES[i]) == 0)
				{
					TESTED_METHODS[0] = i;
					*tested_methods_count = 1;
					break;
				}
			}
			if(i == METHODS_COUNT)
			{
				fprintf (stderr,"Error: Unknown method to test!\n");
				exit(EXIT_FAILURE);
			}
			break;

		case '?':
			/* getopt_long already printed an error message. */
			break;

		default:
			abort ();
		}
	}

	/* test set values */
	if(threads == 0 || test_length == 0 || cycles == 0 || chunk == 0)
	{
		fprintf (stderr,"Error: Option arguments has to be an unsigned numbers!\n");
		exit(EXIT_FAILURE);
	}

	if (help_flag)
	{
		fprintf(stderr,HELP_TEXT,argv[0],THREADS,SECONDS,CYCLES,CHUNK);
		fprintf(stderr,"\nPossible methods to test:\n");
		for(i = 0; i<METHODS_COUNT; i++)
		{
			fprintf(stderr,"  %s\n",METHOD_NAMES[i]);
		}
		exit(EXIT_SUCCESS);
	}

	/* handle file name */
	if (argc-optind == 1)
	{
		*stream = fopen(argv[argc-1], "w");
		if(verbose_flag)
			fprintf(stderr, "Data will be saved to %s.\n", argv[argc-1]);
		if ( !*stream )
		{
			fprintf(stderr, "Error on fopen for file in \"%s\"\n", argv[argc-1]);
			exit (EXIT_FAILURE);
		}
	}
	else if (argc-optind > 1)
	{
		fprintf(stderr,"Too much parameters\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		*stream = stdout;
		fprintf(stderr, "Data will be printed to stdout if -p is specified.\n");
	}
}




/** ***************************************************************
 * MAIN
 */
int main(int argc, char **argv)
{
	unsigned int threads=THREADS;
	unsigned int chunk = CHUNK;
	unsigned int test_length = SECONDS;  /* how many seconds before stopping generation
	                                      * negative to infinite (until ESC) */
	unsigned int cycles = CYCLES; // how many times to run generators to get average throughput
	unsigned int cycle;

	double throughputs [cycles][METHODS_COUNT];
	double averages [METHODS_COUNT];
	double sum, max_throughput;
	int max_throughput_method, method;
	int tested_methods_count = compute_tests_amount(TESTED_METHODS);

	FILE *stream;

	max_throughput = 0;
	max_throughput_method = 0;

	if(rdrand_testSupport() == RDRAND_UNSUPPORTED)
	{
		fprintf(stderr,"FATAL ERROR: RdRand is not supported on this CPU!\n");
		exit (EXIT_FAILURE);
	}

	get_opts(argc,
		 argv,
		 &threads,
		 &chunk,
		 &test_length,
		 &cycles,
		 &stream,
		 &tested_methods_count);

	if(verbose_flag)
	{
		fprintf(stderr,"This test will run %d methods for %d times in %d threads.\n", tested_methods_count, cycles, threads);
		fprintf(stderr,"Each method will run for %d seconds, the overall time of this test is %d seconds.\n",
			test_length, test_length*cycles*tested_methods_count );
		fprintf(stderr,"The random values are generated in chunks of size %u 64 bit values (per thread).\n",chunk);
		if(print_numbers_flag == 0)
			fprintf(stderr,"Generated values will be print raw.\n");
		else
			fprintf(stderr,"Generated values will be print as readable numbers.\n");

		if(no_print_flag)
			fprintf(stderr,"Will not print generates values.\n");
		fprintf(stderr,"-------------------------------------------------------------------\n");
	}

	/************** DO THE TESTING ******************************************/
	/* run all methods in required count */
	for ( cycle = 0; cycle < cycles; cycle++)
	{
		if(verbose_flag)
			fprintf(stderr,"\nDoing %d. run:\n",cycle+1);
		/* run all methods */
		for( method = 0; method < tested_methods_count; method++)
		{
			if(verbose_flag)
				fprintf(stderr,"%s:\n",METHOD_NAMES[TESTED_METHODS[method]]);

			throughputs[cycle][method] = test_throughput(threads, chunk, test_length, stream, TESTED_METHODS[method]);
		}
	}

	/************** PRINT OVERALL RESULTS **********************************/
	if(print_numbers_flag == 1)
		fprintf(stderr,"\n"); // to break line after numbers

	if(verbose_flag)
		fprintf(stderr,"\n-------------------------------------------------------------------\n");

	for( method = 0; method< tested_methods_count; method++)
	{
		sum = 0;
		for( cycle = 0; cycle < cycles; cycle++)
		{
			sum += throughputs[cycle][method];
		}
		averages[method] = sum/cycles;
		// find the maximum throughput
		if(max_throughput < averages[method])
		{
			max_throughput = averages[method];
			max_throughput_method = TESTED_METHODS[method];
		}
	}

	if(verbose_flag)
	{
		fprintf(stderr,"The fastest method (%g MiB/s) was: %s\n",max_throughput, METHOD_NAMES[max_throughput_method]);
		fprintf(stderr,"Average throughputs in %d runs:\n", cycles);
	}

	for( method = 0; method< tested_methods_count; method++)
	{
		if(verbose_flag)
		{
			fprintf(stderr,"  (%.2f %%) Method %s: %.3f MiB/s\n",

				(averages[method]/max_throughput)*100, // percents of max throughput
				METHOD_NAMES[TESTED_METHODS[method]],
				averages[method]
				);
		}
		else
		{

			fprintf(stderr,"%s %.3f MiB/s %.2f %%\n",
				METHOD_NAMES[TESTED_METHODS[method]],
				averages[method],
				(averages[method]/max_throughput)*100 // percents of max throughput
				);
		}
	}



	fclose(stream);
	return 1;
}
