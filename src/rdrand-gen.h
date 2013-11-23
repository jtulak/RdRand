
#ifndef RDRAND_GEN_H
#define RDRAND_GEN_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define DEFAULT_THREADS 4
#define DEFAULT_METHOD GET_BYTES
#define DEFAULT_BYTES 0

#define MAX_CHUNK_SIZE 2048


/**
 * List of methods available for testing.
 * THIS IS LIST OF EXISTING METHODS,
 * NOT METHODS USED IN THE TEST!
 */
enum
{
	GET_BYTES,
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
	"get_uint64_array_reseed_delay",
	"get_uint64_array_reseed_skip",
};

typedef struct cnf {
	/** output file path */
	char* output_filename;
	/** output file stream */
	FILE* output;
	/** ENUM of the used method */
	int method;
	/** Flag of --help/-h */
	int help_flag;
	/** Flag of printed warning about underflow, when only one thread is running */
	int printedWarningFlag;
	/** number of threads */
	unsigned int threads;
	/** number of bytes to generate */
	size_t bytes;
	/** amount of 64bit blocks */
	size_t blocks;
	/** size of chunks to use */
	size_t chunk_size;
	/** count of chunks */
	size_t chunk_count;
	/** amount of bytes to be generated at last */
	size_t ending_bytes;

} cnf_t;

#endif // RDRAND_GEN_H
