
/* gcc -DHAVE_X86INTRIN_H -Wall -Wextra -fopenmp -mrdrnd -lm -I./ -O3 -o gen rdrand-gen.c rdrand.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h> // usleep
#include <getopt.h>
#include <string.h>
#include <math.h>       /* floor */
#include <errno.h>
#include <inttypes.h>
#include "./librdrand.h"
//#include <rdrand-0.1/rdrand.h>
#include "./rdrand-gen.h"

#ifdef _OPENMP
    #include <omp.h>
#endif



#if defined(__X86_64__) || defined(_WIN64) || defined(_LP64)
# define _X86_64
#endif



#define SIZEOF(a) ( sizeof (a) / sizeof (a[0]) )

#define RETRY_LIMIT 10
#define SLOW_RETRY_LIMIT_CYCLES 100
#define SLOW_RETRY_LIMIT 1000
#define SLOW_RETRY_DELAY 1000 // 1 ms

static const char* HELP_TEXT =
	"Usage: %s [OPTIONS]\n"
	"If no output file is specified, the program will print random values to STDOUT.\n\n"
	"OPTIONS\n"
	"  --help       -h      Print this help.\n"
	"  --amount     -n NUM  Generate given amount of bytes. Suffixes: K, M, G, T.\n"
	"                       Without the option or when 0, generate unlimited amount.\n"
	"  --method     -m NAME Use method NAME (default is %s).\n"
	"  --output     -o FILE Save the generated data to the file.\n"
	"  --threads    -t NUM  Run the generator in NUM threads (default %u).\n"
	"\n";

/**
 * Parse arguments and save flags/values to cnf_t* config.
 */
void parse_args(int argc, char** argv, cnf_t* config)
{
	int i;
	char optC;
	double size_as_double;
	char *size_suffix;

	static struct option long_options[] =
	{
		{"help", no_argument,       0, 'h'},
		{"amount",    required_argument, 0, 'n'},
		{"method",  required_argument, 0, 'm'},
		{"output",  required_argument, 0, 'o'},
		{"threads",  required_argument, 0, 't'},
		{0, 0, 0, 0}
	};

	opterr = 0;
	while (1)
	{

		/* getopt_long stores the option index here. */
		int option_index = 0;

		optC = getopt_long (argc, argv, "hn:m:o:t:",
				    long_options, &option_index);

		/* Detect the end of the options. */
		if (optC == -1)
			break;

		switch (optC)
		{

		case 'h':
			config->help_flag = 1;
			break;

		case 'n':
			size_as_double = strtod(optarg,&size_suffix);
			if ((optarg == size_suffix) || errno == ERANGE || (size_as_double < 0) || (size_as_double >= UINT64_MAX) )
			{
			    #ifdef _X86_64
                    fprintf(stderr, "Size has to be in range <0, %lu>!\n",UINT64_MAX);
			    #else
                    fprintf(stderr, "Size has to be in range <0, %llu>!\n",UINT64_MAX);
			    #endif // _X86_64
			}
			if(strlen(size_suffix) > 0)
			{
				switch(*size_suffix)
				{
				case 't': case 'T':
					size_as_double *= pow(10,12);
					break;
				case 'g': case 'G':
					size_as_double *= pow(10,9);
					break;
				case 'm': case 'M':
					size_as_double *= pow(10,6);
					break;
				case 'k': case 'K':
					size_as_double *= pow(10,3);
					break;
				default:
					fprintf(stderr,"Unknown suffix %s when parsing %s.\n",size_suffix, optarg);
				}
			}

			config->bytes = (size_t)floor(size_as_double);
			break;

		case 't':
			config->threads = strtoumax(optarg, NULL, 10);

			break;

		case 'o':
			config->output_filename = optarg;
			break;

		case 'm':                 // method
			for(i = 0; i<METHODS_COUNT; i++)
			{
				if(strcmp(optarg,METHOD_NAMES[i]) == 0)
				{
					config->method = i;
					break;
				}
			}
			if(i == METHODS_COUNT)
			{
				fprintf (stderr,"Error: Unknown method to use!\n");
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

	/** Compute the size of a chunk:
	 *  Total bytes / 8 = number of 64bit blocks.
	 *  No. 64bit blocks / threads = size of chunk
	 */
	if(config->bytes > 0)
	{
		// number of 64bit blocks
		config->blocks = config->bytes / 8;
		// size of one chunk - max size is limited to MAX_CHUNK_SIZE
		config->chunk_size = config->blocks / config->threads;
		if(config->chunk_size > MAX_CHUNK_SIZE)
			config->chunk_size = MAX_CHUNK_SIZE;

		// if there are some chunks (so at least 64 bytes will be generated)
		if(config->chunk_size > 0)
		{
			// get how many iterations in all threads is needed to generate all the data.
			config->chunk_count = config->blocks / (config->chunk_size*config->threads);
			// And how many bytes is left, because they could fit into chunks (just few bytes)
			config->ending_bytes = config->bytes % (config->chunk_size*config->chunk_count*config->threads*8);
		}
		else
		{
			// there is not enough bytes to generate to fill a single chunk
			config->ending_bytes = config->bytes;
		}


		//printf("Will generate %u of 64bit blocks using %u chunks of size %u blocks per thread, ending: %u bytes.\n", (uint)config->blocks, (uint)config->chunk_count, (uint)config->chunk_size, (uint)config->ending_bytes);
	}
	else if(config->bytes == 0)
    {
        config->chunk_size = MAX_CHUNK_SIZE;
    }



}

size_t generate_with_metod(cnf_t *config,uint64_t *buf, unsigned int blocks, int retry)
{
	switch(config->method)
	{
	case GET_BYTES:
		return rdrand_get_bytes_retry((uint8_t*)buf, blocks*8,retry)/8;
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
 * Fill chunks with random data
 * Return number of generated bytes
 */
size_t generate_chunk(cnf_t *config)
{
	unsigned int i, n, retry;
	size_t written, written_total,buf_size;
	uint64_t buf[config->chunk_size*config->threads];

	buf_size = SIZEOF(buf);
	written_total = 0;
	for(n = 0; n < config->chunk_count || config->bytes == 0; n++)
	{
		written = 0;
		/** At first fill chunks in all parallel threads */

#ifdef _OPENMP
		omp_set_num_threads(config->threads);
#pragma omp parallel for reduction(+:written)
#endif // _OPENMP
		for ( i=0; i < config->threads; ++i)
		{
			written += generate_with_metod(config,&buf[i*config->chunk_size], config->chunk_size, RETRY_LIMIT);
		}

		if ( written != buf_size )
		{
			/* if we can't lower threads count anymore */
			if ( config->threads == 1 )
			{
				if(config->printedWarningFlag == 0)
				{
					config->printedWarningFlag++;
					//fprintf(stderr, "Warning: %zu bytes generated, but %zu bytes expected. Trying to get randomness with slower speed.\n", written, buf_size);
					fprintf(stderr, "Warning: Less than expected amount of bytes was generated. Trying to get randomness with slower speed.\n");
				}
				// reset the retry - LIMIT should work work for each run independently
				// and also the delay should be as small as possible
				retry = 0;
				while(written != buf_size && retry++ < SLOW_RETRY_LIMIT_CYCLES)
				{
					usleep(retry*SLOW_RETRY_DELAY);
					// try to generate the rest
					written += generate_with_metod(config,buf+written, config->threads*config->chunk_size-written, SLOW_RETRY_LIMIT);
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
				config->threads--;
				fprintf(stderr, "Warning: %zu bytes generated, but %zu bytes expected. Probably slow internal generator - decreaseing threads count by one to %d to avoid problems.\n", written, buf_size, config->threads);
				buf_size -= config->chunk_size;

				/* run this iteration again */
				n--;
				continue;
			}
		}

		written = fwrite(buf, sizeof(buf[0]), buf_size, config->output);
		written_total += written;

		if ( written !=  buf_size)
		{
			perror("fwrite");
			fprintf(stderr, "ERROR: %zu bytes written, but %zu bytes to write\n", sizeof(buf[0]) * written, buf_size);
			break;
		}
	}
	return written_total*8;
}


/**
 * Fill the ending bytes with random data
 * Return number of generated bytes
 */
size_t generate_ending(cnf_t *config)
{
	size_t written_total;
	uint8_t buf[config->ending_bytes];

	written_total = rdrand_get_bytes_retry(&buf[0], config->ending_bytes,1);
	/* test generated amount */
	if ( written_total != SIZEOF(buf) )
	{
		fprintf(stderr, "ERROR: bytes generated %zu, bytes expected %zu\n", written_total, SIZEOF(buf));
		return written_total;
	}
	written_total = fwrite(buf, sizeof(buf[0]), SIZEOF(buf), config->output);
	if ( written_total !=  SIZEOF(buf) )
	{
		perror("fwrite");
		fprintf(stderr, "ERROR: fwrite - bytes written %zu, bytes to write %zu\n", sizeof(buf[0]) * written_total, sizeof(buf));
		return written_total;

	}
	return written_total;
}

/**
 * Generate requested amount of bytes
 * Return amount of bytes truly generated.
 */
size_t generate(cnf_t *config)
{
	size_t written;
	written = 0;
	/** At first fill chunks in all parallel threads.
	 *  If no size is specified, then the program
	 *  will never get over this.
	 */
	written = generate_chunk(config);

	/** Then fill the few ending bytes in one thread. */
	written += generate_ending(config);
	return written;
}

/*****************************************************************************/

int main(int argc, char** argv)
{
	int i;
	cnf_t config = {NULL, stdout, DEFAULT_METHOD, 0, 0, DEFAULT_THREADS, DEFAULT_BYTES,0,0,0,0};
	parse_args(argc, argv,&config);
	if(config.help_flag)
	{
		printf(HELP_TEXT,argv[0],METHOD_NAMES[DEFAULT_METHOD],DEFAULT_THREADS);
		fprintf(stdout,"\nAccessible methods:\n");
		for(i = 0; i<METHODS_COUNT; i++)
		{
			if(i == DEFAULT_METHOD)
			{
				fprintf(stdout,"  %s [default]\n",METHOD_NAMES[i]);
			}
			else
			{
				fprintf(stdout,"  %s\n",METHOD_NAMES[i]);
			}
		}
		exit(EXIT_SUCCESS);
	}

	if(config.output_filename != NULL)
	{
		config.output = fopen(config.output_filename, "wb");
		if( config.output == NULL )
		{
			fprintf(stderr,"ERROR: Can't open file %s!\n", config.output_filename);
			exit(EXIT_FAILURE);
		}
	}

    if(rdrand_testSupport() == RDRAND_SUPPORTED)
    {
        generate(&config);
    }
    else
    {
        fprintf(stderr,"ERROR: The CPU of this machine do not have RdRand!\n");
        exit(EXIT_FAILURE);
    }

	fclose(config.output);

	exit(EXIT_SUCCESS);
}
