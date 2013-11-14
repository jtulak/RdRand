/* vim: set expandtab cindent fdm=marker ts=2 sw=2: */

/*
   gcc -Wall -Wextra -O2 -fopenmp -mrdrnd [-lrt -lssl -lcrypto (for rng emulation)] -c rdrand.c
 */

#include "rdrand.h"
#include <stddef.h>
#include <string.h>
#include <omp.h>
#include <stdio.h>
#include <unistd.h> // usleep
#include <crypt.h>

#include <cpuid.h>

#define RETRY_LIMIT 10

/**
 * Mask for CPUINFO result. RdRand support on Intel CPUs is
 * declared on 30th bit in ECX register.
 */
#define RDRAND_MASK     0x40000000

#define PRINT_IF_UNDERFLOW(rc, line) if(rc == RDRAND_FAILURE) fprintf(stderr,"ERROR: UNDERFLOW on line %d!!!\n",line)
//#define PRINT_IF_UNDERFLOW(rc, line)

/**
 * Debug options
 * If EMULATE_RNG is 1, openssl is used as a RNG.
 *
 * DEBUG_VERBOSE will print informations.
 * 0 - No informations
 * 9 - all informations (multiple messages from one function)
 *
 */
#define EMULATE_RNG 0
#define DEBUG_VERBOSE 0

#if EMULATE_RNG == 1
//#include <openssl/rand.h>
#endif // EMULATE_RNG

#if DEBUG_VERBOSE > 0
#define DEBUG_PRINT_1(fmt, args ...)    fprintf(stderr, fmt, ## args)
#else
#define DEBUG_PRINT_1(fmt, args ...)    /* Don't do anything in release builds */
#endif

#if DEBUG_VERBOSE > 8
#define DEBUG_PRINT_9(fmt, args ...)    fprintf(stderr, fmt, ## args)
#else
#define DEBUG_PRINT_9(fmt, args ...)    /* Don't do anything in release builds */
#endif

#ifdef HAVE_X86INTRIN_H
#include <x86intrin.h>
inline int rdrand16_step(uint16_t *x)
{
	return _rdrand16_step ( (unsigned short*) x );
}
inline int rdrand32_step(uint32_t *x)
{
	return _rdrand32_step ( (unsigned int*) x );
}
inline int rdrand64_step(uint64_t *x)
{
	return _rdrand64_step ( (unsigned long long*) x );
}
#else



/**
 * 16 bits of entropy through RDRAND
 *
 * The 16 bit result is zero extended to 32 bits.
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand16_step(uint16_t *x)
{
	unsigned char err = 1;
#if EMULATE_RNG == 0
	asm volatile ("rdrand %0 ; setc %1"
		      : "=r" (*x), "=qm" (err));
#else
	RAND_pseudo_bytes((unsigned char *)x, 2);
#endif
	if(err == 1)
	{
		return RDRAND_SUCCESS;
	}
	return RDRAND_FAILURE;
}


/**
 * 32 bits of entropy through RDRAND
 *
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand32_step(uint32_t *x)
{
	unsigned char err = 1;
#if EMULATE_RNG == 0
	asm volatile ("rdrand %0 ; setc %1"
		      : "=r" (*x), "=qm" (err));
#else
	RAND_pseudo_bytes((unsigned char *)x, 4);
#endif
	if(err == 1)
	{
		return RDRAND_SUCCESS;
	}
	return RDRAND_FAILURE;
}


/**
 * 64 bits of entropy through RDRAND
 *
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand64_step(uint64_t *x)
{
	unsigned char err = 1;
#if EMULATE_RNG == 0
	asm volatile ("rdrand %0 ; setc %1"
		      : "=r" (*x), "=qm" (err));
#else
	RAND_pseudo_bytes((unsigned char *)x, 8);
#endif
	if(err == 1)
	{
		return RDRAND_SUCCESS;
	}
	return RDRAND_FAILURE;
}
#endif /* HAVE_X86INTRIN_H */

struct cpuid
{
	uint32_t eax,ebx,ecx,edx;
};
typedef struct cpuid cpuid_t;

/**
 *  cpuid ASM wrapper
 */
void cpuid(cpuid_t *result,uint32_t eax)
{
	__asm__ __volatile__ ("cpuid"
			      : "=a" (result->eax),
			      "=b" (result->ebx),
			      "=c" (result->ecx),
			      "=d" (result->edx)
			      : "a"  (eax)
			      : "memory");
}

/**
 * Detect if the CPU support RdRand instruction.
 * Returns RDRAND_SUPPORTED  or RDRAND_UNSUPPORTED.
 */
int rdrand_testSupport()
{
	cpuid_t reg;
	// test if an Intel CPU
	cpuid(&reg,0); // get vendor

	if(reg.ebx == 0x756e6547 && // Genu
	   reg.ecx == 0x6c65746e && // ntel
	   reg.edx == 0x49656e69 ) // ineI
	{
		// If yes, test if know RdRand
		cpuid(&reg,1); // get feature bits
		if( reg.ecx & RDRAND_MASK )
		{
			return RDRAND_SUPPORTED;
		}
	}


	return RDRAND_UNSUPPORTED;
}



/**
 * Get a 16 bit random number
 *
 * The 16 bit result is zero extended to 32 bits.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT.
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand_get_uint16_retry(uint16_t *dest, int retry_limit)
{
	int rc;
	int count;
	uint16_t x;

	if ( retry_limit < 0 )
		retry_limit = RETRY_LIMIT;
	count = 0;
	do
	{
		rc=rdrand16_step( &x );
		++count;
	}
	while((rc == 0) && (count < retry_limit));
	PRINT_IF_UNDERFLOW (rc, __LINE__);

	if(rc == RDRAND_SUCCESS)
	{
		*dest = x;
		return RDRAND_SUCCESS;
	}
	return RDRAND_FAILURE;
}


/**
 * Get a 32 bit random number
 *
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT.
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand_get_uint32_retry(uint32_t *dest, int retry_limit)
{
	int rc;
	int count;
	uint32_t x;

	if ( retry_limit < 0 )
		retry_limit = RETRY_LIMIT;
	count = 0;
	do
	{
		rc=rdrand32_step( &x );
		++count;
	}
	while((rc == RDRAND_FAILURE) && (count < retry_limit));
	PRINT_IF_UNDERFLOW (rc, __LINE__);

	if(rc == RDRAND_SUCCESS)
	{
		*dest = x;
		return RDRAND_SUCCESS;
	}
	return RDRAND_FAILURE;
}


/**
 * Get a 64 bit random number
 *
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT.
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand_get_uint64_retry(uint64_t *dest, int retry_limit)
{
	int rc;
	int count;
	uint64_t x;

	if ( retry_limit < 0 )
		retry_limit = RETRY_LIMIT;
	count = 0;
	do
	{
		rc=rdrand64_step( &x );
		++count;
	}
	while((rc == RDRAND_FAILURE) && (count < retry_limit));
	PRINT_IF_UNDERFLOW (rc, __LINE__);

	if(rc == RDRAND_SUCCESS)
	{
		*dest = x;
		return RDRAND_SUCCESS;
	}
	return RDRAND_FAILURE;
}


/**
 * Get an array of 16 bit random numbers
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired
 * For higher speed, uses 64bit generating when possible.
 */
unsigned int rdrand_get_uint16_array_retry(uint16_t *dest,  const unsigned int count, int retry_limit)
{
	int rc;
	int retry_count;
	unsigned int generated_16 = 0;
	unsigned int generated_64 = 0;

	unsigned int count_64 = count / 4;
	unsigned int count_16 = count - 4 * count_64;

	uint16_t x_16;
	uint64_t* dest_64;

	if ( retry_limit < 0 )
		retry_limit = RETRY_LIMIT;

	if ( count_16 > 0 )
	{
		retry_count = 0;
		do
		{
			rc=rdrand16_step( &x_16 );
			++retry_count;
		}
		while((rc == RDRAND_FAILURE) && (retry_count < retry_limit));
		PRINT_IF_UNDERFLOW (rc, __LINE__);

		if (rc == RDRAND_SUCCESS)
		{
			*dest = x_16;
			++dest;
			++generated_16;
		}
		else
		{
			return generated_16;
		}
	}

	dest_64 = (uint64_t* ) dest;

	generated_64 = rdrand_get_uint64_array_retry(dest_64, count_64, retry_limit);

	generated_16 += 4 * generated_64;
	return generated_16;
}


/**
 * Get an array of 32 bit random numbers
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired
 * For higher speed, uses 64bit generating when possible.
 */
unsigned int rdrand_get_uint32_array_retry(uint32_t *dest,  const unsigned int count, int retry_limit)
{
	int rc;
	int retry_count;
	unsigned int generated_32 = 0;
	unsigned int generated_64 = 0;

	unsigned int count_64 = count / 2;;
	unsigned int count_32 = count - 2 * count_64;

	uint32_t x_32;
	uint64_t* dest_64;

	if ( retry_limit < 0 )
		retry_limit = RETRY_LIMIT;

	if ( count_32 > 0 )
	{
		retry_count = 0;
		do
		{
			rc=rdrand32_step( &x_32 );
			++retry_count;
		}
		while((rc == RDRAND_FAILURE) && (retry_count < retry_limit));
		PRINT_IF_UNDERFLOW (rc, __LINE__);

		if (rc == RDRAND_SUCCESS)
		{
			*dest = x_32;
			++dest;
			++generated_32;
		}
		else
		{
			return generated_32;
		}
	}

	dest_64 = (uint64_t* ) dest;

	generated_64 = rdrand_get_uint64_array_retry(dest_64, count_64, retry_limit);

	generated_32 += 2 * generated_64;
	return generated_32;
}


/**
 * Get an array of 64 bit random numbers
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired
 */
unsigned int rdrand_get_uint64_array_retry(uint64_t *dest, const unsigned int count, int retry_limit)
{
	int rc;
	int retry_count;
	unsigned int generated_64 = 0;
	unsigned int i;
	uint64_t x_64;

	if ( retry_limit < 0 )
		retry_limit = RETRY_LIMIT;

	for ( i=0; i<count; ++i)
	{
#if 1
		retry_count = 0;
		do
		{
			rc=rdrand64_step( &x_64 );
			++retry_count;
		}
		while((rc == RDRAND_FAILURE) && (retry_count < retry_limit));
		PRINT_IF_UNDERFLOW (rc, __LINE__);

#else
		rc = rdrand_get_uint64_retry(&x_64, retry_limit);
#endif
		if (rc == RDRAND_SUCCESS)
		{
			*dest = x_64;
			++dest;
			++generated_64;
		}
		else
		{
			break;
		}
	}

	return generated_64;
}


/**
 * Get an array of 8 bit random numbers
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired
 * For higher speed, uses 64bit generating when possible.
 */
unsigned int rdrand_get_uint8_array_retry(uint8_t *dest,  const unsigned int count, int retry_limit)
{
	int rc;
	int retry_count;
	unsigned int generated_8 = 0;
	unsigned int generated_64 = 0;

	unsigned int count_64 = count / (unsigned int)8;
	unsigned int count_8 = count % (unsigned int)8;


	uint64_t x_64;
	uint64_t* dest_64;
	//printf("count 8b overall: %u, count 64b: %u, count 8b: %u\n",count,count_64, count_8);

	if ( retry_limit < 0 )
		retry_limit = RETRY_LIMIT;

	if ( count_8 > 0 )
	{
/* TODO: decide what of the two following variant should be used in all functions above.
 * With calling the _retry function, the performance is about 6-7 percent lower than
 * with _step.
 * Possibly solution: use macro for the shared code?
 */
#if 1 // little faster, but making duplicities in every function
		retry_count = 0;
		do
		{
			rc=rdrand64_step( &x_64 );
			++retry_count;
		}
		while((rc == RDRAND_FAILURE) && (retry_count < retry_limit));
		PRINT_IF_UNDERFLOW (rc, __LINE__);
#else // little slower, but cleaner code
		rc = rdrand_get_uint64_retry(&x_64, retry_limit);
#endif
		if (rc == RDRAND_SUCCESS)
		{
			memcpy((void*) dest, (void*) &x_64, count_8);
			dest += count_8;
			generated_8 = count_8;
		}
		else
		{
			return generated_8;
		}
	}

	dest_64 = (uint64_t* ) dest;

	generated_64 = rdrand_get_uint64_array_retry(dest_64, count_64, retry_limit);

	generated_8 += 8 * generated_64;
	return generated_8;
}


/**
 * Get bytes of random values.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired.
 * For higher speed, uses 64bit generating when possible.
 */
size_t rdrand_get_bytes_retry(void *dest, const size_t size, int retry_limit)
{
	uint64_t *start = dest;
	uint64_t *alignedStart;
	uint64_t *restStart;

	unsigned int alignedBytes;
	unsigned int qWords;
	unsigned int offset;
	unsigned int rest;

	size_t generatedBytes=0;


	if ( retry_limit < 0 )
		retry_limit = RETRY_LIMIT;

	/**
	 *   Description of memory:
	 *   -----|OFFSET|QWORDS (aligned to 64bit blocks)|REST|-----
	 */

	if(size < 8)
	{
		offset=0;
		alignedStart = (uint64_t *)start;
		alignedBytes = size;
	}
	else
	{
		/* get offset of first 64bit aligned block in the target buffer */
		offset = 8-(unsigned long int)start % (unsigned long int) 8;
		if(offset == 0)
		{
			alignedStart = (uint64_t *)start;
			alignedBytes = size;
			DEBUG_PRINT_9("DEBUG 9: No align needed - start: %p\n", (void *)start);
		}
		else
		{
			alignedStart = (uint64_t *)(((uint64_t)start & ~(uint64_t)7)+(uint64_t)8);
			alignedBytes = size - offset;
			DEBUG_PRINT_9("DEBUG 9:  Aligning needed - start: %p, alignedStart: %p\n", (void *)start, (void *)alignedStart);
		}
	}


	/* get count of 64bit blocks */
	rest = alignedBytes % 8;
	qWords = (alignedBytes - rest) >> 3; // divide by 8;

	DEBUG_PRINT_9("DEBUG 9: offset: %u, qWords: %u, rest: %u\n", offset, qWords,rest);

	/* fill the begining */
	if(offset != 0)
	{
		generatedBytes += rdrand_get_uint8_array_retry((uint8_t *)start,offset,retry_limit);
	}

    /* fill the main 64bit blocks */
	if(qWords)
    {
        generatedBytes += 8*rdrand_get_uint64_array_retry(alignedStart,qWords, retry_limit);
    }

	/* fill the rest */
	if(rest != 0)
	{
		restStart = alignedStart+qWords;
		generatedBytes += rdrand_get_uint8_array_retry((uint8_t *)restStart,rest,retry_limit);
	}

	return generatedBytes;
}


/**
 * Write count bytes of random data to a file.
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired.
 */
size_t rdrand_fwrite(FILE *f, const size_t count, int retry_limit)
{
	uint64_t tmprand;
	size_t count64;
	size_t count8;
	size_t generated = 0;

	generated = 0;

	count64 = count >> 3; // divide by 8
	count8 = count % 8;

	// generate 64bit blocks
	for(; count64 > 0; count64--)
	{
		if(!rdrand_get_uint64_retry(&tmprand, retry_limit))
			return generated;
		fwrite(&tmprand,sizeof(uint64_t),1,f);
		generated += 8;
	}

	// generate the rest unaligned bytes
	if(count8)
	{
		if(!rdrand_get_uint64_retry(&tmprand, retry_limit))
			return generated;
		fwrite(&tmprand,sizeof(uint8_t),count8,f);
		generated += count8;
	}

	return generated;
}


#if 0
/***************************************************************/
/* Two methods of computing a reseed key                       */
/*   The first takes multiple random numbers through RdRand    */
/*   with intervening delays to ensure reseeding and performs  */
/*   AES-CBC-MAC over the data to compute the seed value.      */
/*                                                             */
/*   The second takes multiple random numbers through RdRand   */
/*   without  intervening delays to ensure reseeding and       */
/*   performs AES-CBC-MAC over the data to compute the seed    */
/*   value. More values are gathered than the first method,to  */
/*   ensure reseeding without needing delays.                  */
/*                                                             */
/* Note that these algorithms ensure the output value to be a  */
/* 'True Random Number' rather than a 'Cryptographically       */
/* Secure Random Number' which is what RdRand produces         */
/* natively.                                                   */
/***************************************************************/

/************************************************************************************/
/* CBC-MAC together 32 128 bit values, gathered with delays between, to guarantee   */
/* some interveneing reseeds.                                                       */
/* Creates a random value that is fully forward and backward prediction resistant,  */
/* suitable for seeding a NIST SP800-90 Compliant, FIPS 1402-2 certifiable SW DRBG  */
/************************************************************************************/

int _rdrand_get_seed128_retry(unsigned int retry_limit, void *buffer)
{
	unsigned char m[16];
	unsigned char key[16];
	unsigned char ffv[16];
	unsigned char xored[16];
	unsigned int i;

	/* Chose an arbitary key and zero the feed_forward_value (ffv) */
	for (i=0; i<16; i++)
	{
		key[i]=(unsigned char)i;
		ffv[i]=0;
	}

	/* Perform CBC_MAC over 32 * 128 bit values, with 10us gaps between each 128 bit value        */
	/* The 10us gaps will ensure multiple reseeds within the HW RNG with a large design margin.   */

	for (i=0; i<32; i++)
	{
		usleep(10);
		if(rdrand_get_uint64_array_retry((unsigned long long int*)m, 2,retry_limit) == RDRAND_FAILURE)
			return 0;
		xor_128(m,ffv,xored);
		aes128k128d(key,xored,ffv);
	}

	for (i=0; i<16; i++)
		((unsigned char *)buffer)[i] = ffv[i];
	return 1;
}


/************************************************************************************/
/* CBC-MAC together 2048 128 bit values, to exceed the reseed limit, to guarantee   */
/* some interveneing reseeds.                                                       */
/* Creates a random value that is fully forward and backward prediction resistant,  */
/* suitable for seeding a NIST SP800-90 Compliant, FIPS 1402-2 certifiable SW DRBG  */
/************************************************************************************/

int _rdrand_get_seed128_method2_retry(unsigned int retry_limit, void *buffer)
{
	unsigned char m[16];
	unsigned char key[16];
	unsigned char ffv[16];
	unsigned char xored[16];
	unsigned int i;

	for (i=0; i<16; i++)
	{
		key[i]=(unsigned char)i;
		ffv[i]=0;
	}

	for (i=0; i<2048; i++)
	{
		if(rdrand_get_uint64_array_retry((unsigned long long int*)m, 2,retry_limit) == RDRAND_FAILURE)
			return 0;
		xor_128(m,ffv,xored);
		aes128k128d(key,xored,ffv);
	}

	for (i=0; i<16; i++)
		((unsigned char *)buffer)[i] = ffv[i];
	return 1;
}


#endif


/**
 * Get an array of 64 bit random values.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired.
 *
 * Force reseed by waiting few microseconds before each generating.
 */
unsigned int rdrand_get_uint64_array_reseed_delay(uint64_t *dest, const unsigned int count, int retry_limit)
{
	int rc;
	unsigned int generated_64 = 0;
	unsigned int i;
	uint64_t x_64;

	if ( retry_limit < 0 )
		retry_limit = RETRY_LIMIT;

	for ( i=0; i<count; ++i)
	{
		usleep(10);
		rc = rdrand_get_uint64_retry(&x_64, retry_limit);

		if (rc == RDRAND_SUCCESS)
		{
			*dest = x_64;
			++dest;
			++generated_64;
		}
		else
		{
			break;
		}
	}

	return generated_64;
}


/**
 * Get an array of 64 bit random values.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired.
 *
 * Force reseed by generating and throwing away 1024 values per one saved.
 */
unsigned int rdrand_get_uint64_array_reseed_skip(uint64_t *dest, const unsigned int count, int retry_limit)
{
	int rc;
	unsigned int generated_64 = 0;
	unsigned int i,n;
	uint64_t x_64;

	if ( retry_limit < 0 )
		retry_limit = RETRY_LIMIT;

	for ( i=0; i<count; ++i)
	{

		// load 1024 numbers to force reseed
		for(n=0; n< 1024; n++)
		{
			rdrand64_step( &x_64 );
		}
		// load unique number
		rc = rdrand_get_uint64_retry(&x_64, retry_limit);

		if (rc == RDRAND_SUCCESS)
		{
			*dest = x_64;
			++dest;
			++generated_64;
		}
		else
		{
			break;
		}
	}

	return generated_64;
}

