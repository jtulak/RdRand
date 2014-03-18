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
    Now the legal stuff is done. This file contain the library itself.
*/


#include "./librdrand.h"
#include <stddef.h>
#include <string.h>
#include <omp.h>
#include <stdio.h>
#include <unistd.h> // usleep


// Delay for enforcing reseed in the rdrand_get_uint64_array_reseed_delay
// method.
// Value is in usec.
#define RESEED_DELAY 20

#define RETRY_LIMIT 10


#if defined(__X86_64__) || defined(_WIN64) || defined(_LP64)
# define _X86_64
#endif


/**
 * Mask for CPUINFO result. RdRand support on Intel CPUs is
 * declared on 30th bit in ECX register.
 */
#define RDRAND_MASK     0x40000000

//#define PRINT_IF_UNDERFLOW(rc, line) if(rc == RDRAND_FAILURE) fprintf(stderr,"ERROR: UNDERFLOW on line %d!!!\n",line)
#define PRINT_IF_UNDERFLOW(rc, line)

/**
 * Debug options
 *
 * DEBUG_VERBOSE will print informations.
 * 0 - No informations
 * 9 - all informations (multiple messages from one function)
 *
 */
#define DEBUG_VERBOSE 0


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

/** ********************************************************************
 *                         TESTING NAMES
 * For testing purposes, real generating functions can be renamed
 * and all library will use stub functions with predetermined behaviour.
 * 
 * When STUB_RDRAND is defined, any rdrand calls will set all bits 
 * to 1. For access to the real rdrand, use rdrandXX_step_native()
 */

//#define STUB_RDRAND

#ifdef STUB_RDRAND
	#define RDRAND16_STEP rdrand16_step_native
	#define RDRAND32_STEP rdrand32_step_native
	#define RDRAND64_STEP rdrand64_step_native

	inline int rdrand16_step(uint16_t *x)
	{
		*x=~(*x & 0);
		return  RDRAND_SUCCESS;
	}
	inline int rdrand32_step(uint32_t *x)
	{
		*x=~(*x & 0);
		return  RDRAND_SUCCESS;
	}
	inline int rdrand64_step(uint64_t *x)
	{
		*x=~(*x & 0);
		return  RDRAND_SUCCESS;
	}
#else
	#define RDRAND16_STEP rdrand16_step
	#define RDRAND32_STEP rdrand32_step
	#define RDRAND64_STEP rdrand64_step
#endif // STUB_RDRAND


#if defined(HAVE_RDRAND_IN_GCC) && defined(HAVE_X86INTRIN_H)
        #include <x86intrin.h>
        inline int RDRAND16_STEP(uint16_t *x)
        {
            return _rdrand16_step ( (unsigned short*) x );
        }
        inline int RDRAND32_STEP(uint32_t *x)
        {
            return _rdrand32_step ( (unsigned int*) x );
        }
        inline int RDRAND64_STEP(uint64_t *x)
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
        int RDRAND16_STEP(uint16_t *x)
        {
            unsigned char err = 1;
            asm volatile (".byte 0x66; .byte 0x0f; .byte 0xc7; .byte 0xf0; setc %1"
                      : "=a" (*x), "=qm" (err));
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
        int RDRAND32_STEP(uint32_t *x)
        {
            unsigned char err = 1;
            asm volatile (".byte 0x0f; .byte 0xc7; .byte 0xf0; setc %1"
                      : "=a" (*x), "=qm" (err));
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
        int RDRAND64_STEP(uint64_t *x)
        {
            unsigned char err = 1;
            /* support for 32bit architecture */
            #ifdef _X86_64
                asm volatile (".byte 0x48; .byte 0x0f; .byte 0xc7; .byte 0xf0; setc %1"
                          : "=a" (*x), "=qm" (err));
              //  asm volatile("rdrand %0; setc %1":"=r"(*x), "=qm"(err));
            #else
                uint32_t *x32;
                x32=(uint32_t*)x;
                asm volatile (".byte 0x0f; .byte 0xc7; .byte 0xf0; setc %1"
                          : "=a" (*x32), "=qm" (err));
                /* test after the first call*/
                if(err != 1)
                {
                    return RDRAND_FAILURE;
                }

                asm volatile (".byte 0x0f; .byte 0xc7; .byte 0xf0; setc %1"
                          : "=a" (*(x32+1)), "=qm" (err));

            #endif

            if(err == 1)
            {
                return RDRAND_SUCCESS;
            }
            return RDRAND_FAILURE;
        }
#endif /* HAVE_X86INTRIN_H && HAVE_RDRAND_IN_GCC*/

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
#ifdef _X86_64
	asm volatile ("cpuid"
			      : "=a" (result->eax),
			      "=b" (result->ebx),
			      "=c" (result->ecx),
			      "=d" (result->edx)
			      : "a"  (eax)
			      : "memory");

#else // 32-bit
        // http://newbiz.github.io/cpp/2010/12/20/Playing-with-cpuid.html
        asm volatile (
          "pushl %%ebx   \n\t" // Backup %ebx
          "cpuid         \n\t" // Call cpuid
          "movl %%ebx, %1\n\t" // Copy the %ebx result elsewhere
          "popl %%ebx    \n\t" // Restore %ebx
          : "=a"(result->eax), "=r"(result->ebx), "=c"(result->ecx), "=d"(result->edx)
          : "a"(eax)
          : "cc"
        );
#endif // _X86_64
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
		retry_count = 0;
		do
		{
			rc=rdrand64_step( &x_64 );
			++retry_count;
		}
		while((rc == RDRAND_FAILURE) && (retry_count < retry_limit));
		PRINT_IF_UNDERFLOW (rc, __LINE__);

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
		retry_count = 0;
		do
		{
			rc=rdrand64_step( &x_64 );
			++retry_count;
		}
		while((rc == RDRAND_FAILURE) && (retry_count < retry_limit));
		PRINT_IF_UNDERFLOW (rc, __LINE__);
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
		usleep(RESEED_DELAY);
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

