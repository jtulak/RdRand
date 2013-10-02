/* vim: set expandtab cindent fdm=marker ts=2 sw=2: */

/*
gcc -Wall -Wextra -O2 -fopenmp -mrdrnd [-lrt -lssl -lcrypto (for rng emulation)] -c rdrand.c
*/

#include "rdrand.h"
#include <stddef.h>
#include <string.h>
#include <omp.h>

#include <cpuid.h>

#define RETRY_LIMIT 10



/**
 * Debug options
 * If EMULATE_RNG is 1, openssl is used as a RNG.
 *
 * DEBUG_VERBOSE will print informations.
 * 0 - No informations
 * 9 - all informations (multiple messages from one function)
 *
 */
#define EMULATE_RNG 1
#define DEBUG_VERBOSE 9

#if EMULATE_RNG == 1
#include <openssl/rand.h>
#endif // EMULATE_RNG

#if DEBUG_VERBOSE > 0
#include <stdio.h>
#define DEBUG_PRINT_1(fmt, args...)    fprintf(stderr, fmt, ## args)
#else
#define DEBUG_PRINT_1(fmt, args...)    /* Don't do anything in release builds */
#endif

#if DEBUG_VERBOSE > 8
#define DEBUG_PRINT_9(fmt, args...)    fprintf(stderr, fmt, ## args)
#else
#define DEBUG_PRINT_9(fmt, args...)    /* Don't do anything in release builds */
#endif

#ifdef HAVE_X86INTRIN_H
#include <x86intrin.h>
inline int rdrand16_step(uint16_t *x) { return _rdrand16_step ( (unsigned short*) x ); }
inline int rdrand32_step(uint32_t *x) { return _rdrand16_step ( (unsigned int*) x ); }
inline int rdrand64_step(uint64_t *x) { return _rdrand16_step ( (unsigned long long*) x ); }
#else

/*
16 bits of entropy through RDRAND
The 16 bit result is zero extended to 32 bits
Returns 1 on success, or 0 on underflow
*/

int rdrand16_step(uint16_t *x) {
  unsigned char err;
#if EMULATE_RNG == 0
  asm volatile("rdrand %0 ; setc %1"
	    : "=r" (*x), "=qm" (err));
#else
    RAND_pseudo_bytes((unsigned char *)x, 2);
    err=1;
#endif
	return (int) err;
}

/*
32 bits of entropy through RDRAND
Returns 1 on success, or 0 on undeerflow
*/

int rdrand32_step(uint32_t *x) {
	unsigned char err;
#if EMULATE_RNG == 0
	asm volatile("rdrand %0 ; setc %1"
	    : "=r" (*x), "=qm" (err));
#else
    RAND_pseudo_bytes((unsigned char *)x, 4);
    err=1;
#endif
	return (int) err;
}


/*
64 bits of entropy through RDRAND
Returns 1 on success, or 0 on underflow
*/

int rdrand64_step(uint64_t *x) {
	unsigned char err;
#if EMULATE_RNG == 0
	asm volatile("rdrand %0 ; setc %1"
	    : "=r" (*x), "=qm" (err));
#else
    RAND_pseudo_bytes((unsigned char *)x, 8);
    err=1;
#endif
	return (int) err;
}
#endif /* HAVE_X86INTRIN_H */


/*
Get a 16 bit random number
Will retry up to retry_limit times. Negative retry_limit
implies default retry_limit RETRY_LIMIT
Returns 1 on success, or 0 on underflow
*/

int rdrand_get_uint16_retry(uint16_t *dest, int retry_limit) {
  int rc;
  int count;
  uint16_t x;

  if ( retry_limit < 0 ) retry_limit = RETRY_LIMIT;
  count = 0;
  do {
    rc=rdrand16_step( &x );
    ++count;
  } while((rc == 0) || (count < retry_limit));

  if (rc == 1) {
    *dest = x;
    return 1;
  } else {
    return 0;
  }
}

/*
Get a 32 bit random number
Will retry up to retry_limit times. Negative retry_limit
implies default retry_limit RETRY_LIMIT
Returns 1 on success, or 0 on underflow
*/

int rdrand_get_uint32_retry(uint32_t *dest, int retry_limit) {
  int rc;
  int count;
  uint32_t x;

  if ( retry_limit < 0 ) retry_limit = RETRY_LIMIT;
  count = 0;
  do {
    rc=rdrand32_step( &x );
    ++count;
  } while((rc == 0) || (count < retry_limit));

  if (rc == 1) {
    *dest = x;
    return 1;
  } else {
    return 0;
  }
}

/*
Get a 64 bit random number
Will retry up to retry_limit times. Negative retry_limit
implies default retry_limit RETRY_LIMIT
Returns 1 on success, or 0 on underflow
*/

int rdrand_get_uint64_retry(uint64_t *dest, int retry_limit) {
  int rc;
  int count;
  uint64_t x;

  if ( retry_limit < 0 ) retry_limit = RETRY_LIMIT;
  count = 0;
  do {
    rc=rdrand64_step( &x );
    ++count;
  } while((rc == 0) || (count < retry_limit));

  if (rc == 1) {
    *dest = x;
    return 1;
  } else {
    return 0;
  }
}


/*
Get an array of 32 bit random numbers
Will retry up to retry_limit times. Negative retry_limit
implies default retry_limit RETRY_LIMIT
Returns the number of units successfuly acquired
Uses rdrand64_step for the higher speed
*/

size_t rdrand_get_uint32_array_retry(uint32_t *dest, size_t size, int retry_limit) {
  int rc;
  int retry_count;
  size_t generated_32 = 0;
  size_t generated_64 = 0;

  size_t count_64 = size / 2;;
  size_t count_32 = size - 2 * count_64;
  size_t i;

  uint32_t x_32;
  uint64_t x_64;
  uint64_t* dest_64;

  if ( retry_limit < 0 ) retry_limit = RETRY_LIMIT;

  if ( count_32 > 0 ) {
    retry_count = 0;
    do {
      rc=rdrand32_step( &x_32 );
      ++retry_count;
    } while((rc == 0) || (retry_count < retry_limit));

    if (rc == 1) {
      *dest = x_32;
      ++dest;
      ++generated_32;
    } else {
      return generated_32;
    }
  }

  dest_64 = (uint64_t* ) dest;

  for ( i=0; i<count_64; ++i) {
    retry_count = 0;
    do {
      rc=rdrand64_step( &x_64 );
      ++retry_count;
    } while((rc == 0) || (retry_count < retry_limit));

    if (rc == 1) {
      *dest_64 = x_64;
      ++dest_64;
      ++generated_64;
    } else {
      generated_32 += 2 * generated_64;
      return generated_32;
    }
  }
  generated_32 += 2 * generated_64;
  return generated_32;
}
/*
Get an array of 64 bit random numbers
Will retry up to retry_limit times. Negative retry_limit
implies default retry_limit RETRY_LIMIT
Returns the number of units successfuly acquired
*/


size_t rdrand_get_uint64_array_retry(uint64_t *dest, size_t size, int retry_limit) {
  int rc;
  int retry_count;
  size_t generated_64 = 0;
  size_t i;
  uint64_t x_64;

  if ( retry_limit < 0 ) retry_limit = RETRY_LIMIT;

  for ( i=0; i<size; ++i) {
    retry_count = 0;
    do {
      rc=rdrand64_step( &x_64 );
      ++retry_count;
    } while((rc == 0) || (retry_count < retry_limit));

    if (rc == 1) {
      *dest = x_64;
      ++dest;
      ++generated_64;
    } else {
      return generated_64;
    }
  }
  return generated_64;
}

/*
Get an array of 8 bit random numbers
Will retry up to retry_limit times. Negative retry_limit
implies default retry_limit RETRY_LIMIT
Returns the number of bytes successfuly acquired
Uses rdrand64_step for the higher speed
*/

size_t rdrand_get_uint8_array_retry(uint8_t *dest, size_t size, int retry_limit) {
  int rc;
  int retry_count;
  size_t generated_8 = 0;
  size_t generated_64 = 0;

  size_t count_64 = size / 8;
  size_t count_8 = size % 8;
  size_t i;

  uint64_t x_64;
  uint64_t* dest_64;

  if ( retry_limit < 0 ) retry_limit = RETRY_LIMIT;

  if ( count_8 > 0 ) {
    retry_count = 0;
    do {
      rc=rdrand64_step( &x_64 );
      ++retry_count;
    } while((rc == 0) || (retry_count < retry_limit));

    if (rc == 1) {
#if 0
      for (i=0; i<count_8;++i) {
        *dest = (uint8_t)(x_64 & 0xff);
        x_64 = x_64 >> 8;
        ++dest;
        ++generated_8;
      }
#else
      memcpy((void*) dest, (void*) &x_64, count_8);
      dest += count_8;
      generated_8 = count_8;
#endif
    } else {
      return generated_8;
    }
  }

  dest_64 = (uint64_t* ) dest;

  for ( i=0; i<count_64; ++i) {
    retry_count = 0;
    do {
      rc=rdrand64_step( &x_64 );
      ++retry_count;
    } while((rc == 0) || (retry_count < retry_limit));

    if (rc == 1) {
      *dest_64 = x_64;
      ++dest_64;
      ++generated_64;
    } else {
      generated_8 += 8 * generated_64;
      return generated_8;
    }
  }
  generated_8 += 8 * generated_64;
  return generated_8;
}

/*
Get count bytes of random values.
Will retry up to retry_limit times. Negative retry_limit
implies default retry_limit RETRY_LIMIT
Returns the number of bytes successfuly acquired
Uses rdrand64_step for the higher speed
*/
size_t rdrand_get_bytes_retry(unsigned int count, void *dest, int retry_limit)
{
    uint64_t *start = dest;
    uint64_t *alignedStart;
    uint64_t *restStart;

    unsigned int alignedBytes;
    unsigned int qWords;
    unsigned int offset;
    unsigned int rest;

    size_t generatedBytes=0;

    uint64_t tmpRand;
    unsigned int i;

    if ( retry_limit < 0 ) retry_limit = RETRY_LIMIT;


    /*
        Description of memory:
        -----|OFFSET|QWORDS (aligned to 64bit blocks)|REST|-----
    */

    /* get offset of first 64bit aligned block in the target buffer */
    offset = 8-(unsigned long int)start % (unsigned long int) 8;
    if(offset == 0)
    {
        alignedStart = (uint64_t *)start;
        alignedBytes = count;
        DEBUG_PRINT_9("DEBUG 9: No align needed - start: %p\n", (void *)start);
    }
    else
    {
        alignedStart = (uint64_t *)(((uint64_t)start & ~(uint64_t)7)+(uint64_t)8);
        alignedBytes = count - offset;
        DEBUG_PRINT_9("DEBUG 9:  Aligning needed - start: %p, alignedStart: %p\n", (void *)start, (void *)alignedStart);
    }

    /* get count of 64bit blocks */
    rest = alignedBytes % 8;
    qWords = (alignedBytes - rest) >> 3; // divide by 8;

    DEBUG_PRINT_9("DEBUG 9: offset: %u, qWords: %u, rest: %u\n", offset, qWords,rest);


    /* fill the begining */
    if(offset != 0)
    {
        /* offset is always smaller than one 64 bit number */
        if (rdrand_get_uint64_retry(&tmpRand,retry_limit) == 0)
            return 0;
        memcpy((void*)start,(void*)&tmpRand,offset);
        generatedBytes = offset;
        DEBUG_PRINT_9("DEBUG 9:  Generating offset. Total generated bytes: %u\n", (unsigned int)generatedBytes);

    }
    /* fill the main 64bit blocks */
    for(i=0; i<qWords;i++)
    {
        if (rdrand_get_uint64_retry(&(alignedStart[i]),retry_limit) == 0)
            return 0;
        generatedBytes += 8;
        DEBUG_PRINT_9("DEBUG 9:  Generating 64bit blocks. Total generated bytes: %u\n",  (unsigned int)generatedBytes);

    }

    /* fill the rest */
    if(rest != 0)
    {
        restStart = alignedStart + qWords;
        /* rest is always smaller than one 64 bit number */
        if (rdrand_get_uint64_retry(&tmpRand,retry_limit) == 0)
            return 0;
        memcpy((void*)restStart,(void*)&tmpRand,rest);
        generatedBytes += rest;
        DEBUG_PRINT_9("DEBUG 9:  Generating rest. Total generated bytes: %u\n",  (unsigned int)generatedBytes);

    }

    return generatedBytes;
}




#if 0

/****************************************************************/
/* Uses RdRand to acquire a block of n 32 bit random numbers    */
/*   Will uses RdRand64 and RdRand32 to optimize speed          */
/*   Writes that entropy to (unsigned int *)dest[0+].           */
/*   Will retry up to retry_limit times                         */
/*   Returns the number of units successfuly acquired           */
/****************************************************************/

int _rdrand_get_n_uints_retry(unsigned int n, unsigned int retry_limit, unsigned int *dest)
{
	int qwords;
	int dwords;
	int i;

	unsigned long long int qrand;
	unsigned int drand;

	int success;
	int count;

	int total_uints;

	unsigned long int *qptr;

	total_uints = 0;
	qptr = (unsigned long int*)dest;

	qwords = n/2;
	dwords = n -(qwords*2);

	for (i=0; i<qwords; i++)
	{
		count = 0;
		do
		{
        		success=_rdrand64_step(&qrand);
		} while((success == 0) || (count++ < retry_limit));

		if (success == 1)
		{
			*qptr = qrand;
			qptr++;
			total_uints+=2;
		}
		else (i = qwords);
	}
	if ((qwords > 0) && (success == 0)) return total_uints;

	dest = (unsigned int*)qptr;
        for (i=0; i<dwords; i++)
        {
		count = 0;
                do
                {
                	success=_rdrand32_step(&drand);
                } while((success == 0) || (count++ < retry_limit));

                if (success == 1)
                {
                        *dest = qrand;
			dest++;
                        total_uints++;
                }
		else (i = dwords);
        }
        return total_uints;
}

/**************************************************/
/* Uses RdRand to acquire a 32 bit random number  */
/*   Writes that entropy to (unsigned int *)dest. */
/*   Will not attempt retry on underflow          */
/*   Returns 1 on success, or 0 on underflow      */
/**************************************************/

int _rdrand_get_uint(unsigned int *dest)
{
	int therand;
	if (_rdrand32_step(&therand))
	{
		*dest = therand;
		return 1;
	}
	else return 0;
}

/****************************************************************/
/* Uses RdRand to acquire a block of n 32 bit random numbers    */
/*   Will uses RdRand64 and RdRand32 to optimize speed          */
/*   Writes that entropy to (unsigned int *)dest[0+].           */
/*   Will not attempt retry on underflow                        */
/*   Returns the number of units successfuly acquired           */
/****************************************************************/

int _rdrand_get_n_uints(int n, unsigned int *dest)
{
        int qwords;
        int dwords;
        int i;

        unsigned long long int qrand;
        unsigned int drand;

        int success;

        int total_uints;

        unsigned long int *qptr;

        total_uints = 0;
        qptr = (unsigned long int*)dest;

        qwords = n/2;
        dwords = n -(qwords*2);

        for (i=0; i<qwords; i++)
        {
                if (_rdrand64_step(&qrand))
                {
                        *qptr = qrand;
                        qptr++;
                        total_uints+=2;
                }
		else (i = qwords);
        }
        if ((qwords > 0) && (success == 0)) return total_uints;

        dest = (unsigned int*)qptr;
        for (i=0; i<dwords; i++)
        {
                if (_rdrand32_step(&drand))
                {
                        *dest = drand;
                        dest++;
                        total_uints++;
                }
		else (i = dwords);
        }
        return total_uints;

}

/****************************************************************/
/* Uses RdRand to acquire a block of random bytes               */
/*   Uses RdRand64 to optimize speed                            */
/*   Writes that entropy to (unsigned int *)dest[0+].           */
/*   Internally will retry up to 10 times un underflow.         */
/*   Returns 1 on success, 0 on failure                         */
/****************************************************************/

int _rdrand_get_bytes_step(unsigned int n, unsigned char *dest)
{
unsigned char *start;
unsigned char *residualstart;
unsigned long long int *blockstart;
unsigned int count;
unsigned int residual;
unsigned int startlen;
unsigned long long int i;
unsigned long long int temprand;
unsigned int length;

	/* Compute the address of the first 64 bit aligned block in the destination buffer */
	start = dest;
	if (((unsigned long int)start % (unsigned long int)8) == 0)
	{
		blockstart = (unsigned long long int *)start;
		count = n;
		startlen = 0;
	}
	else
	{
		blockstart = (unsigned long long int *)(((unsigned long long int)start & ~(unsigned long long int)7)+(unsigned long long int)8);
		count = n - (8 - (unsigned int)((unsigned long long int)start % 8));
		startlen = (unsigned int)((unsigned long long int)blockstart - (unsigned long long int)start);
	}

	/* Compute the number of 64 bit blocks and the remaining number of bytes */
	residual = count % 8;
	length = count >> 3;
	if (residual != 0)
	{
		residualstart = (unsigned char *)(blockstart + length);
	}

	/* Get a temporary random number for use in the residuals. Failout if retry fails */
	if (startlen > 0)
	{
		if (_rdrand_get_n_qints_retry(1, 10, (void *)&temprand) == 0) return 0;
	}

	/* populate the starting misaligned block */
	for (i = 0; i<startlen; i++)
	{
		start[i] = (unsigned char)(temprand & 0xff);
		temprand = temprand >> 8;
	}

	/* populate the central aligned block. Fail out if retry fails */
	if (_rdrand_get_n_qints_retry(length, 10, (void *)(blockstart)) == 0) return 0;

	/* populate the final misaligned block */
	if (residual > 0)
	{
		if (_rdrand_get_n_qints_retry(1, 10, (void *)&temprand) == 0) return 0;
		for (i = 0; i<residual; i++)
		{
			residualstart[i] = (unsigned char)(temprand & 0xff);
			temprand = temprand >> 8;
		}
	}

        return 1;
}

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
	for (i=0;i<16;i++)
	{
		key[i]=(unsigned char)i;
		ffv[i]=0;
	}

	/* Perform CBC_MAC over 32 * 128 bit values, with 10us gaps between each 128 bit value        */
	/* The 10us gaps will ensure multiple reseeds within the HW RNG with a large design margin.   */

	for (i=0; i<32;i++)
	{
		usleep(10);
		if(_rdrand_get_n_qints_retry(2,retry_limit,(unsigned long long int*)m) == 0) return 0;
		xor_128(m,ffv,xored);
		aes128k128d(key,xored,ffv);
	}

	for (i=0;i<16;i++) ((unsigned char *)buffer)[i] = ffv[i];
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

	for (i=0;i<16;i++)
	{
		key[i]=(unsigned char)i;
		ffv[i]=0;
	}

	for (i=0; i<2048;i++)
	{
		if(_rdrand_get_n_qints_retry(2,retry_limit,(unsigned long long int*)m) == 0) return 0;
		xor_128(m,ffv,xored);
		aes128k128d(key,xored,ffv);
	}

	for (i=0;i<16;i++) ((unsigned char *)buffer)[i] = ffv[i];
	return 1;
}

#endif
