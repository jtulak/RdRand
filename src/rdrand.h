/* vim: set expandtab cindent fdm=marker ts=2 sw=2: */

#ifndef INTEL_RDRAND_H
#define INTEL_RDRAND_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


/**
 * Returned by function if a random number(s) was generated correctly.
 */
#define RDRAND_SUCCESS      1

/**
 * Returned by function if a random number(s) was NOT generated correctly.
 */
#define RDRAND_FAILURE      -1

/**
 * Returned by support-test function if the CPU support rdrand
 */
#define RDRAND_SUPPORTED    2
/**
 * Returned by support-test function if the CPU doesn't know rdrand
 */
#define RDRAND_UNSUPPORTED  -2

/**
 * 16 bits of entropy through RDRAND
 *
 * The 16 bit result is zero extended to 32 bits.
 * Returns 1 on success, or 0 on underflow.
 */
int rdrand16_step(uint16_t *x);

/**
 * 32 bits of entropy through RDRAND
 *
 * Returns 1 on success, or 0 on underflow.
 */
int rdrand32_step(uint32_t *x);

/**
 * 64 bits of entropy through RDRAND
 *
 * Returns 1 on success, or 0 on underflow.
 */

int rdrand64_step(uint64_t *x);

/**
 * Get a 16 bit random number
 *
 * The 16 bit result is zero extended to 32 bits.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT.
 * Returns 1 on success, or 0 on underflow.
 */
int rdrand_get_uint16_retry(uint16_t *dest, int retry_limit);

/**
 * Get a 32 bit random number
 *
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT.
 * Returns 1 on success, or 0 on underflow.
 */
int rdrand_get_uint32_retry(uint32_t *dest, int retry_limit);

/**
 * Get a 64 bit random number
 *
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT.
 * Returns 1 on success, or 0 on underflow.
 */
int rdrand_get_uint64_retry(uint64_t *dest, int retry_limit);
size_t rdrand_get_uint32_array_retry(uint32_t *dest, size_t size, int retry_limit);
size_t rdrand_get_uint64_array_retry(uint64_t *dest, size_t size, int retry_limit);
size_t rdrand_get_uint8_array_retry(uint8_t *dest, size_t size, int retry_limit);


/**
 * Get bytes of random values.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfuly acquired.
 * Uses rdrand64_step for the higher speed.
 */
size_t rdrand_get_bytes_retry(unsigned int count, void *dest, int retry_limit);

/**
 * Write count bytes of random data to a file.
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfuly acquired.
 */
size_t rdrand_fwrite(FILE *f, size_t count,int retry_limit);


#endif

