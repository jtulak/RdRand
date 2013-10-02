/* vim: set expandtab cindent fdm=marker ts=2 sw=2: */

#ifndef INTEL_RDRAND_H
#define INTEL_RDRAND_H

#include <stddef.h>
#include <stdint.h>

int rdrand16_step(uint16_t *x);
int rdrand32_step(uint32_t *x);
int rdrand64_step(uint64_t *x);

int rdrand_get_uint16_retry(uint16_t *dest, int retry_limit);
int rdrand_get_uint32_retry(uint32_t *dest, int retry_limit);
int rdrand_get_uint64_retry(uint64_t *dest, int retry_limit);
// size_t rdrand_get_uint16_array_retry(uint32_t *dest, size_t size, int retry_limit);
size_t rdrand_get_uint32_array_retry(uint32_t *dest, size_t size, int retry_limit);
size_t rdrand_get_uint64_array_retry(uint64_t *dest, size_t size, int retry_limit);
size_t rdrand_get_uint8_array_retry(uint8_t *dest, size_t size, int retry_limit);

size_t rdrand_get_bytes_retry(unsigned int count, void *dest, int retry_limit);

#endif

