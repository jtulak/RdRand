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
#include <time.h> // debug
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


// just for debug
//#include "./librdrand-aes.private.h"
//extern aes_cfg_t AES_CFG;
//void memDump_gen(unsigned char *mem, unsigned int length) {
//        unsigned i;
//            for (i=0; length > i; i++){
//                        fprintf(stderr,"%02x",mem[i]);
//                            }
//                fprintf(stderr,"\n");
//}


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

// {{{
struct timespec time_diff(struct timespec * start, struct timespec * end) {
   struct timespec diff;
   diff.tv_sec = end->tv_sec - start->tv_sec;
   if(end->tv_nsec < start->tv_sec){
       diff.tv_sec--;
       diff.tv_nsec = start->tv_sec - end->tv_nsec;
   }else{
       diff.tv_nsec = end->tv_nsec - start->tv_sec;
   }
   return diff;
}
void printTimer(int thread, struct timespec diff){
    fprintf(stderr,"Time difference (%d): %lld secs, %lld nsecs\n",
            thread,
            (long long) diff.tv_sec,
            (long long) diff.tv_nsec);
}

// }}}


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
#ifndef NO_MAIN // for testing
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
    "  --aes-ctr    -a      Encrypt the output with AES-CTR.\n"
	"  --aes-keys   -k FILE Use given key file for the AES encryption instead of random one.\n"
	"  --verbose    -v      Be verbose (will print on stderr).\n"
	"  --version    -V      Print version.\n"
	"\n";
#endif // NO_MAIN
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

// {{{ hex2byte
int hexToByte(const char *hex, size_t hex_length, unsigned char* byte, size_t byte_length) {
  size_t i;
  int rc;
  unsigned int n;
  if (hex_length!=2*byte_length) return 0;
    
  for (i=0; i<byte_length;++i) {
      rc = sscanf(hex, "%02x", &n);
      if ( rc != 1 ) {
        fprintf(stderr, "Error during sscanf\n");
        fprintf(stderr, "Read %d bytes\n",rc);
        fprintf(stderr, "%x", *byte);
        return 0;
      }
      *byte = (unsigned char) n;
      hex+=2;
      byte+=1;
  }
  return 1;
}

// }}}


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
            config->ending_bytes = config->bytes % 
                (config->chunk_size*config->chunk_count*config->threads*8);
        }
        else
        {
            // there is not enough bytes to generate to fill a single chunk
            config->ending_bytes = config->bytes;
        }

        /*
        printf( "Will generate %u of 64bit blocks using %u chunks of size %u "
                "blocks per thread, ending: %u bytes.\n", 
                (uint)config->blocks, 
                (uint)config->chunk_count, 
                (uint)config->chunk_size, 
                (uint)config->ending_bytes);
                // */
    }
    else if(config->bytes == 0)
    {
        // infinite generation
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
		{"aes-ctr",  no_argument, 0, 'a'},
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

		optC = getopt_long (argc, argv, "hak:vVn:m:o:t:",
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
		case 'a':
			config->aes_flag = 1;
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
size_t generate_with_metod(cnf_t *config,uint8_t *buf, unsigned int blocks, int retry)
{
    size_t res = 0;
	switch(config->method)
	{
	case GET_BYTES:
		res= rdrand_get_bytes_retry((uint8_t*)buf, blocks,retry);
		break;
	case GET_RESEED64_DELAY:
		res= rdrand_get_uint64_array_reseed_delay((uint64_t*)buf, blocks/8, retry)*8;
		break;
	case GET_RESEED64_SKIP:
		res= rdrand_get_uint64_array_reseed_skip((uint64_t*)buf, blocks/8, retry)*8;
		break;
	}
	return res;
}
// }}} generate_with_metod


/**
 * Fill chunks with random data
 * Return number of generated bytes
 */
// {{{ generate_chunk
size_t generate_chunk(cnf_t *config)
{
	unsigned int i, n, retry, first_run=1, aes_thread=0;
	size_t written, written_total,buf_size, buf_size_bytes;
    // NOTE: chunk_size is count of 64bit blocks!
	uint64_t buf[config->chunk_size*config->threads],
             gen_buf1[config->chunk_size*config->threads],
             gen_buf2[config->chunk_size*config->threads],
             *gen_current, *gen_previous;

// EPRINT("key: ");
// memDump_gen(AES_CFG.keys.key_current, AES_CFG.keys.key_length);
// EPRINT("nonce: ");
// memDump_gen(AES_CFG.keys.nonce_current, AES_CFG.keys.key_length);

	buf_size = config->chunk_size*config->threads;
	buf_size_bytes = buf_size*8;
	written_total = 0;

    // decide whether aes is used and thus one more thread will run or not
    if(config->aes_flag) {
        // when aes is used, two buffers are needed.
        // While one is filled by rdrand, the other one is being encrypted.
        // Swaping them is faster than copying.
        gen_current = gen_buf1;
        gen_previous = gen_buf2;
        aes_thread=1;
    }else {
        gen_current = buf;
    }
    
    #ifdef _OPENMP
        omp_set_num_threads(config->threads+aes_thread);
    #endif // _OPENMP
	// for all chunks (or indefinitely if bytes are set to 0)
	for(n = 0; n < config->chunk_count+aes_thread || config->bytes == 0; n++)
	{
		written = 0;
		/** At first fill chunks in all parallel threads */
    #ifdef _OPENMP
        #pragma omp parallel for reduction(+:written)
    #endif // _OPENMP
		for ( i=0; i < config->threads+aes_thread; ++i)
		{
            //EPRINT("XXX - thread: %u, n: %u\n", i,n);
            // all threads set in settings will generate values
            // but one more will encrypt them if needed
            //if(i < config->threads && (n < config->chunk_count || config->bytes == 0) ) {
            if (i < config->threads) {
                if(n < config->chunk_count || config->bytes == 0 ) {
                    //fprintf(stderr,"  Generating thread: %u, n: %u\n",i,n);
                    written += generate_with_metod(
                        config, 
                        (uint8_t*)&gen_current[i*config->chunk_size], 
                        config->chunk_size*8, 
                        RETRY_LIMIT)/8;
                }
            } else if (!first_run ){
                // running just in single thread if aes is used
                // and don't run in first cycle
                // - no data in buffers.
                
                //fprintf(stderr,"  Encrypting thread: %u, n: %u\n",i,n);
                rdrand_enc_buffer(buf, gen_previous, buf_size_bytes);
            }

		}

        // if not enough data was generated, try to slow down and print an error
        // {{{ error handling
		if ( written != buf_size && n < config->chunk_count)
		{
			/* if we can't lower threads count anymore */
			if ( config->threads == 1 )
			{
				if(config->printedWarningFlag == 0)
				{
					config->printedWarningFlag++;
					EPRINT( "Warning: %zu bytes was generated, "
                            "but %zu was expected. "
                            "Trying to get randomness with slower speed.\n",
                            written, buf_size);
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
                        (uint8_t*)gen_current+written, 
                        config->threads*config->chunk_size-written*8, 
                        SLOW_RETRY_LIMIT)/8; 
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
                EPRINT("n: %u\n",n);
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
        // }}} error handling

        // If the chunk was generated ok (we are here),
        // move it to the encryption buffer.
        // From there it will be moved in next round to the output.
        if( config->aes_flag){
            //memcpy(enc_buf, gen_buf, buf_size_bytes);
            //Swap current and previous buffer
            { uint64_t*tmp;
                tmp = gen_current;
                gen_current = gen_previous;
                gen_previous = tmp;
            }
            if (first_run) {
                // if it is first run, skip it - no data ready
                first_run = 0;
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

	} // for all chunks
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
  uint8_t buf[config->ending_bytes];
  uint8_t enc_buf[config->ending_bytes];

  written_total = generate_with_metod(config, enc_buf, config->ending_bytes, RETRY_LIMIT);
	/* test generated amount */
	if ( written_total != config->ending_bytes )
	{
		EPRINT( "1ERROR: bytes generated %zu, bytes expected %zu\n", written_total, config->ending_bytes);
		return written_total;
	}
    
    if(config->aes_flag) {
        //fprintf(stderr,"Encrypting tail\n");
        // encrypt the previous-run buffer
        rdrand_enc_buffer(buf, enc_buf, config->ending_bytes);
    } else {
        //fprintf(stderr,"Just memcpy tail\n");
        // this will run in single thread
        memcpy(buf, enc_buf, config->ending_bytes);
    }

	written_total = fwrite(buf, sizeof(buf[0]), config->ending_bytes, config->output);
	if ( written_total !=  config->ending_bytes )
	{
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


/** load keys from file saved in config into AES_CFG
 *
 * @param config
 *
 * @return  FILE_ERRORS enum
 */
// {{{ load_keys
int load_keys(cnf_t * config) {
    FILE*file;
    unsigned char *key, *nonce, *keys[MAX_KEYS], *nonces[MAX_KEYS];
    unsigned int key_len, nonce_len, first_len=0, amount =0, i;
    int res;

    file=fopen(config->aeskeys_filename,"r");
    if(file == NULL){
      EPRINT("ERROR: Can't open file %s!\n", config->aeskeys_filename);
      exit(EXIT_FAILURE);
    }
    
    //while( (res = load_key_line(file, &key, &key_len, &nonce, &nonce_len)) != E_EOF) {
    while( 1 ) {
        key=malloc(sizeof(char)*RDRAND_MAX_KEY_LENGTH);
        nonce=malloc(sizeof(char)*RDRAND_MAX_KEY_LENGTH);
        if( key==NULL || nonce==NULL)
            return E_ERROR;

        res = load_key_line(file, &key, &key_len, &nonce, &nonce_len);
        if(res == E_EOF){
            free(key);
            free(nonce);
            break;
        }
        if(res == E_OK) {
            // skip empty line
            if(key_len == 0) {
                continue;
            }
            // no need for nonce_len check, because it is computed from key length
            if(key_len > RDRAND_MAX_KEY_LENGTH || key_len < RDRAND_MIN_KEY_LENGTH)
                return E_KEY_NONCE_BAD_LENGTH;
            // initialize key length on first key
            if(first_len == 0)
                first_len = key_len;
            // if some other key length is different, it is ERROR
            if(first_len != key_len)
                return E_KEY_NONCE_BAD_LENGTH;
            // now everything is checked, add the Key
            keys[amount] = key;
            nonces[amount]=nonce;
            amount++;
        }else {
            return res;
        }

    }
    fclose(file);
    if(amount == 0)
        return E_KEY_NONCE_BAD_LENGTH;
    
    if(rdrand_set_aes_keys(amount, first_len, keys, nonces) == 0)
        return E_KEY_NONCE_BAD_LENGTH;

    // free memory, keys are copied into AES_CFG
    for(i=0; amount > i; i++) {
        // set memory to zero at first
        memset(keys[i], 0, RDRAND_MAX_KEY_LENGTH*sizeof(char));
        memset(nonces[i], 0, RDRAND_MAX_KEY_LENGTH*sizeof(char));
        //free 
        free(keys[i]);
        free(nonces[i]);
    }

    return E_OK;
}
// }}}
/** Load a single line from given file. Key and nonce will be returned
 *  by parameter.
 *
 * @param file
 * @param key
 * @param nonce
 *
 * @return FILE_ERRORS enum
 */
// {{{ load_key_line
int load_key_line(
        FILE*file, 
        unsigned char ** key, 
        unsigned int *key_len,  
        unsigned char ** nonce, 
        unsigned int *nonce_len
        ) {
    
    char c;
    unsigned int i;
    char buf[2*RDRAND_MAX_KEY_LENGTH] = {};

    
    i=0;
    while ( (c=getc(file)) != EOF && c != '\n') {
        if (
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F') ||
            (c >= '0' && c <= '9')
            ) {
            
            if(i > RDRAND_MAX_KEY_LENGTH*2)
                return E_KEY_NONCE_BAD_LENGTH;
            buf[i] = c;
        } else {
            printf("Unknown: %c (%02x)\n",c,c);
            return E_KEY_INVALID_CHARACTER;
        }
        i++;
    }
    // nonce is half of key, so key is first 2/3 of line and nonce the last 1/3
    *key_len = (i/3)*2;
    *nonce_len = i/3;

    // special cases, when there is no need for malloc
    if(*key_len + *nonce_len != i)
        return E_KEY_NONCE_BAD_LENGTH;
    if(c == EOF)
        return E_EOF;
    if(*key_len == 0)
        return E_OK;

    // convert to bytes 
    hexToByte(buf, *key_len, *key, *key_len/2);
    hexToByte(buf+*key_len, *nonce_len, *nonce, *nonce_len/2);
    // set key/nonce length to bytes
    *nonce_len=*nonce_len/2;
    *key_len = *key_len/2;
    
    return E_OK;
}
// }}}

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
  if(config.aes_flag) {
    // if key filename is given
    if(config.aeskeys_filename != NULL) {
        switch( load_keys(&config)){
            case E_KEY_NONCE_BAD_LENGTH:
                EPRINT("ERROR: File %s has incorrect syntax!\n"
                        "All keys has to be the same length.\n",
                        config.aeskeys_filename);
                exit(EXIT_FAILURE);
                break;
            case E_KEY_INVALID_CHARACTER:
                EPRINT("ERROR: Keys has to be saved as hexa strings.\n");
                exit(EXIT_FAILURE);
                break;
            case E_ERROR:
                EPRINT("ERROR: An error happened when opening file %s.\n",
                        config.aeskeys_filename);
                exit(EXIT_FAILURE);
                break;
        }
        
    } else {
      // key filename is not set, generate keys
      rdrand_set_aes_random_key();

    }
  }
  // FIXME valgrind...
    #ifdef STUB_RDRAND
    if(1)
    #else 
    if(rdrand_testSupport() == RDRAND_SUPPORTED)
    #endif // STUB_RDRAND
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

  if(config.aes_flag) {
    rdrand_clean_aes();
  }

	exit(EXIT_SUCCESS);
}
#endif // NO_MAIN
// }}} MAIN
