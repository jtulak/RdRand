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
    Now the legal stuff is done. This file contain the generator using the library.

    Manual compiling:
    gcc -DHAVE_X86INTRIN_H -Wall -Wextra -fopenmp -mrdrnd -lm -I./ -O3 -o rdrand-gen rdrand-gen.c rdrand.c

    Fast performance testing:

    ./rdrand-gen |pv -c >/dev/null
*/

#ifndef RDRAND_GEN_H
#define RDRAND_GEN_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define DEFAULT_THREADS 2
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
	"reseed_delay",
	"reseed_skip",
};

typedef struct cnf {
	/** output file path */
	char* output_filename;
	/** filename for --aes-keys/-k*/
	char* aeskeys_filename;
	/** output file stream */
	FILE* output;
	/** file for --aes-keys/-k*/
	FILE* aeskeys_file;
	/** ENUM of the used method */
	int method;
	/** Flag of --help/-h */
	int help_flag;
	/** Flag of --version */
	int version_flag;
	/** Flag of --verbose/-v */
	int verbose_flag;
	/** flag of --aes-ctr/-a*/
	int aesctr_flag;
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
