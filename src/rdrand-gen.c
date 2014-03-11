/* vim: set expandtab cindent fdm=marker ts=4 sw=4: */
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
    Now the legal stuff is done. This file contain the generator using the library.

    Fast performance testing:
    ./rdrand-gen |pv -c >/dev/null
*/

// {{{ INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h> // usleep
#include <getopt.h>
#include <string.h>
#include <math.h>       /* floor */
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include "./librdrand.h"
#include "./librdrand-aes.h"
//#include <rdrand-0.1/rdrand.h>
#include "./rdrand-gen.h"
// }}} INCLUDES

// {{{ IFDEFs
#ifdef _OPENMP
    #include <omp.h>
#endif

#ifndef NO_ERROR_PRINTS
    #define EPRINT(...) fprintf(stderr,__VA_ARGS__)
#else
    #define EPRINT(...) ((void)0)
#endif


#if defined(__X86_64__) || defined(_WIN64) || defined(_LP64)
#define _X86_64
#endif
// }}} IFDEFs

// {{{ macros
#define SIZEOF(a) ( sizeof (a) / sizeof (a[0]) )

#define RETRY_LIMIT 10
#define SLOW_RETRY_LIMIT_CYCLES 100
#define SLOW_RETRY_LIMIT 1000
#define SLOW_RETRY_DELAY 1000 // 1 ms

#define VERSION "1.1.0"
// }}} macros

/**
 * List of names of methods for printing.
 * Has to be in the same order as in the enum.
 */
// {{{ METHOD_NAMES
const char *METHOD_NAMES[] =
{
	"get_bytes",
  "get_bytes_aes",
	"reseed_delay",
	"reseed_skip"
};
// }}} METHOD_NAMES

// {{{ HELP_TEXT
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
	"  --aes-keys   -k FILE Use given key file for the AES method instead of random one.\n"
	"  --verbose    -v      Be verbose (will print on stderr).\n"
	"  --version    -V      Print version.\n"
	"\n";
// }}} HELP_TEXT

// {{{ VERSION_TEXT
static const char* VERSION_TEXT =
	"rdrand-gen, librdrand %s\n"
	"Copyright (C) 2014 Jan Tulak <jan@tulak.me>\n"
	"License LGPLv2.1+: Lesser GNU GPL version 2.1 or newer <http://www.gnu.org/licenses/lgpl-2.1.html>\n"
	"This is free software: you are free to change and redistribute it.\n"
	"There is NO WARRANTY, to the extent permitted by law.\n";
// }}} VERSION_TEXT

// {{{ print_version
void print_version(FILE* stream){
    fprintf(stream, VERSION_TEXT, VERSION);
}
// }}} print_version

// {{{ print_available_methods
void print_available_methods(FILE* stream){
    int i=0;
    fprintf(stream,"\nAvailable methods:\n");
    for(i = 0; i<METHODS_COUNT; i++)
    {
        if(i == DEFAULT_METHOD)
        {
            fprintf(stream,"  %s [default]\n",METHOD_NAMES[i]);
        }
        else
        {
            fprintf(stream,"  %s\n",METHOD_NAMES[i]);
        }
    }
}
// }}} print_available_methods

/** Compute the size of a chunk:
 *  Total bytes / 8 = number of 64bit blocks.
 *  No. 64bit blocks / threads = size of chunk
 */
// {{{ compute_chunk_size
void compute_chunk_size(cnf_t * config){
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
// }}} compute_chunk_size

/**
 * Parse arguments and save flags/values to cnf_t* config.
 */
// {{{ parse_args
int parse_args(int argc, char** argv, cnf_t* config)
{
	int i;
	char optC;
	double size_as_double;
	char *size_suffix;

	static struct option long_options[] =
	{
		{"help", no_argument,       0, 'h'},
		{"verbose",  no_argument, 0, 'v'},
		{"version",  no_argument, 0, 'V'},
		{"amount",    required_argument, 0, 'n'},
		{"method",  required_argument, 0, 'm'},
		{"output",  required_argument, 0, 'o'},
		{"threads",  required_argument, 0, 't'},
		{"aes-keys",  required_argument, 0, 'k'},
		{0, 0, 0, 0}
	};

	opterr = 0;
	while (1)
	{

		/* getopt_long stores the option index here. */
		int option_index = 0;

		optC = getopt_long (argc, argv, "hakvVn:m:o:t:",
				    long_options, &option_index);

		/* Detect the end of the options. */
		if (optC == -1)
			break;

		switch (optC)
		{
        case 'V':
            config->version_flag = 1;
            break;
        case 'v':
            config->verbose_flag = 1;
            break;
		case 'h':
			config->help_flag = 1;
			break;

		case 'k':
			config->aeskeys_filename = optarg;
			break;

		case 'n':
      // {{{ parse amount
			size_as_double = strtod(optarg,&size_suffix);
			if ((optarg == size_suffix) ||
             errno == ERANGE ||
             (size_as_double < 0) || 
             (size_as_double >= UINT64_MAX) ){
			    #ifdef _X86_64
                    EPRINT("Size has to be in range <0, %lu>!\n",UINT64_MAX);
			    #else
                    EPRINT("Size has to be in range <0, %llu>!\n",UINT64_MAX);
			    #endif // _X86_64
                //exit(EXIT_FAILURE);
                return EXIT_FAILURE;
			}
			if(strlen(size_suffix) > 0)
			{
				switch(*size_suffix)
				{
				case 't': case 'T':
					size_as_double *= pow(2,40);
					break;
				case 'g': case 'G':
					size_as_double *= pow(2,30);
					break;
				case 'm': case 'M':
					size_as_double *= pow(2,20);
					break;
				case 'k': case 'K':
					size_as_double *= pow(2,10);
					break;
				default:
					EPRINT("Unknown suffix %s when parsing %s.\n",
                        size_suffix, 
                        optarg);
                    return EXIT_FAILURE;
				}
			}
      // }}} parse amount
			config->bytes = (size_t)floor(size_as_double);
			break;

		case 't':
      // {{{ parse threads
		    {
                char*p;
                config->threads=strtoul(optarg,&p,10);
                if((p ==optarg)||(*p !=0)
                   ||errno ==ERANGE
                   ||(config->threads <1)
                   ||(config->threads >=INT_MAX)
                   ||(config->threads>16384))
                {
                    EPRINT("Invalid threads parameter!\n");
                    return EXIT_FAILURE;
                }
		    }
			//config->threads = strtoumax(optarg, NULL, 10);
      // }}} parse threads

			break;

		case 'o':
			config->output_filename = optarg;
			break;

		case 'm':                 // method
      // {{{ parse method
		    config->method = METHODS_COUNT;
			for(i = 0; i<METHODS_COUNT; i++)
			{
				if(strcmp(optarg,METHOD_NAMES[i]) == 0)
				{
					config->method = i;
					break;
				}
			}
			// test default value
			if(config->method == METHODS_COUNT)
			{
				  EPRINT("Error: Unknown method to use!\n");
				  print_available_methods(stderr);
				  //exit(EXIT_FAILURE);
              return EXIT_FAILURE;
			}
			break;
      // }}} parse method

		case '?':
		    EPRINT("An unknown parameter.\n");
			//exit(EXIT_FAILURE);
            return EXIT_FAILURE;
		default:
		    EPRINT("An unknown parameter %c.\n",optC);
			//exit(EXIT_FAILURE);
            return EXIT_FAILURE;
			//abort ();
		}
	}

	  compute_chunk_size(config);


    return EXIT_SUCCESS;
}
// }}} parse_args

/**
 * call specific generating method according to config
 * @param config
 * @param buf
 * @param blocks
 * @param retry
 * @return generated bytes
 */
// {{{ generate_with_metod
size_t generate_with_metod(cnf_t *config,uint64_t *buf, unsigned int blocks, int retry)
{

	switch(config->method)
	{
	case GET_BYTES:
		return rdrand_get_bytes_retry((uint8_t*)buf, blocks*8,retry)/8;
		break;
  case GET_BYTES_AES:
    return rdrand_get_bytes_aes_ctr((unsigned char*)buf, blocks*8,retry)/8;

	case GET_RESEED64_DELAY:
		return rdrand_get_uint64_array_reseed_delay(buf, blocks, retry);
		break;
	case GET_RESEED64_SKIP:
		return rdrand_get_uint64_array_reseed_skip(buf, blocks, retry);
		break;
	}
	return 0;
}
// }}} generate_with_metod


/**
 * Fill chunks with random data
 * Return number of generated bytes
 */
// {{{ generate_chunk
size_t generate_chunk(cnf_t *config)
{
	unsigned int i, n, retry;
	size_t written, written_total,buf_size;
	uint64_t buf[config->chunk_size*config->threads];

	buf_size = SIZEOF(buf);
	written_total = 0;
	// for all chunks (or indefinitely if bytes are set to 0)
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
			written += generate_with_metod(
          config, 
          &buf[i*config->chunk_size], 
          config->chunk_size, 
          RETRY_LIMIT);
		}

		if ( written != buf_size )
		{
			/* if we can't lower threads count anymore */
			if ( config->threads == 1 )
			{
				if(config->printedWarningFlag == 0)
				{
					config->printedWarningFlag++;
					//EPRINT( "Warning: %zu bytes generated, but %zu bytes expected. Trying to get randomness with slower speed.\n", written, buf_size);
					EPRINT( "Warning: Less than expected amount of bytes was generated. "
              "Trying to get randomness with slower speed.\n");
				}
				// reset the retry - LIMIT should work work for each run independently
				// and also the delay should be as small as possible
				retry = 0;
				while(written != buf_size && retry++ < SLOW_RETRY_LIMIT_CYCLES)
				{
					usleep(retry*SLOW_RETRY_DELAY);
					// try to generate the rest
					written += generate_with_metod(
              config, 
              buf+written, 
              config->threads*config->chunk_size-written, 
              SLOW_RETRY_LIMIT);
				}
				if( written != buf_size )
				{
					EPRINT( "Error:  %zu bytes generated, but %zu bytes expected. "
              "Probably there is a hardware problem with your CPU.\n", 
              written, 
              buf_size);
					break;
				}
			}
			else
			{
				/* try to lower threads count to avoid underflow */
				config->threads--;
				EPRINT( "Warning: %zu bytes generated, but %zu bytes expected. "
            "Probably slow internal generator "
            "- decreaseing threads count by one to %d to avoid problems.\n", 
            written, 
            buf_size, 
            config->threads);
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
			EPRINT( "ERROR: %zu bytes written, but %zu bytes to write\n", 
          sizeof(buf[0]) * written, 
          buf_size);
      break;
		}
	}
	return written_total*8;
}
// }}} generate_chunk


/**
 * Fill the ending bytes with random data
 * Return number of generated bytes
 */
// {{{ generate_ending
size_t generate_ending(cnf_t *config)
{
	size_t written_total;
  uint8_t buf[MAX_CHUNK_SIZE];
  written_total = generate_with_metod(config, (uint64_t*)buf, MAX_CHUNK_SIZE/8, RETRY_LIMIT)*8;
	/* test generated amount */
	if ( written_total != MAX_CHUNK_SIZE )
	{
		EPRINT( "ERROR: bytes generated %zu, bytes expected %d\n", written_total, MAX_CHUNK_SIZE);
		return written_total;
	}
	written_total = fwrite(buf, sizeof(buf[0]), config->ending_bytes, config->output);
	if ( written_total !=  config->ending_bytes )
	{
		perror("fwrite");
		EPRINT( "ERROR: fwrite - bytes written %zu, bytes to write %zu\n", written_total, config->ending_bytes);
		return written_total;

	}
	return written_total;
}
// }}} generate_ending

/**
 * Generate requested amount of bytes
 * Return amount of bytes truly generated.
 */
// {{{ generate
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
// }}} generate

/*****************************************************************************/
// {{{ MAIN
#ifndef NO_MAIN // for testing
int main(int argc, char** argv)
{
	size_t generated;
	cnf_t config = DEFAULT_CONFIG_SETTING;
    // {NULL, NULL, stdout, NULL, DEFAULT_METHOD, 0, 0, 0, 0, 0, DEFAULT_THREADS, DEFAULT_BYTES,0,0,0,0};
	if( parse_args(argc, argv,&config) == EXIT_FAILURE) {
        exit(EXIT_FAILURE);
    }

	if(config.help_flag)
	{
		printf(HELP_TEXT,argv[0],METHOD_NAMES[DEFAULT_METHOD],DEFAULT_THREADS);
		print_available_methods(stdout);
		exit(EXIT_SUCCESS);
	}
	if(config.version_flag)
    {
        print_version(stdout);
        exit (EXIT_SUCCESS);
	}

	if(config.output_filename != NULL)
	{
		config.output = fopen(config.output_filename, "wb");
		if( config.output == NULL )
		{
			EPRINT("ERROR: Can't open file %s!\n", config.output_filename);
			exit(EXIT_FAILURE);
		}
	}

  // if AES is used
  if(config.method == GET_BYTES_AES) {
    // if key filename is given
    if(config.aeskeys_filename != NULL) {
      config.aeskeys_file = fopen(config.output_filename, "r");
      if( config.aeskeys_file == NULL )
      {
        EPRINT("ERROR: Can't open file %s!\n", config.aeskeys_filename);
        exit(EXIT_FAILURE);
      }
      // TODO: test if it contain keys
    } else {
      // key filename is not set, generate keys
      rdrand_set_aes_random_key();

    }
  }
    if(rdrand_testSupport() == RDRAND_SUPPORTED)
    {

        if(config.verbose_flag)
        {
            if(config.bytes)
            {
                EPRINT( "Generating %zu bytes using %s method and %u working threads.\n",
                      config.bytes,
                      METHOD_NAMES[config.method],
                      config.threads);
            }
            else
            {
                EPRINT( "Generating infinite bytes using %s method and %u working threads.\n",
                      METHOD_NAMES[config.method],
                      config.threads);
            }

        }
        generated=generate(&config);
        if(config.verbose_flag)
        {
            // TODO print it also on ^C
            EPRINT( "Generated %zu bytes.\n", generated);
        }

    }
    else
    {
        EPRINT("ERROR: The CPU of this machine do not have RdRand!\n");
        exit(EXIT_FAILURE);
    }

	fclose(config.output);

  if(config.method == GET_BYTES_AES) {
    rdrand_clean_aes();
  }

	exit(EXIT_SUCCESS);
}
#endif // NO_MAIN
// }}} MAIN
