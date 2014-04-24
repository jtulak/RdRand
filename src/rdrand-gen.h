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
#define MAX_KEYS 128

/* Macro for default config settings.
 * Please note, changing of values not marked as CAN CHANGE
 * can has undefined result.
 */
#define DEFAULT_CONFIG_SETTING \
{\
    .output=stdout, \
    .method=DEFAULT_METHOD, \
    .threads=DEFAULT_THREADS, \
    .bytes=DEFAULT_BYTES\
}
#if 0
{ \
    NULL, /* output filename */ \
    NULL, /* aes filename */ \
    stdout, /* output stream - CAN CHANGE*/ \
    DEFAULT_METHOD, /* default generating method - CAN CHANGE*/ \
    0, /* help flag*/ \
    0, /* version flag*/ \
    0, /* verbose flag */ \
    0, /* printWarning flag */ \
    0, /* aes flag */ \
    DEFAULT_THREADS, /* default number of threads - CAN CHANGE*/ \
    DEFAULT_BYTES, /* default amount of bytes generated - CAN CHANGE */ \
    0, /* blocks */ \
    0, /* chunk_size */ \
    0, /* chunk_count */ \
    0 /* ending_bytes */ \
}
#endif

/**
 * List of methods available for testing.
 * THIS IS LIST OF EXISTING METHODS,
 * NOT METHODS USED IN THE TEST!
 */
enum {
    GET_BYTES,
    GET_BYTES_AES,
    GET_RESEED64_DELAY,
    GET_RESEED64_SKIP,

    // helper constants
    METHODS_COUNT
};

enum FILE_ERRORS {
  E_OK,
  E_EOF,
  E_ERROR,
  E_KEY_NONCE_BAD_LENGTH,
  E_KEY_INVALID_CHARACTER,

};

/**
 * List of names of methods for printing.
 * Has to be in the same order as in the enum.
 */

extern const char *METHOD_NAMES[];

typedef struct cnf {
    /** output file path */
    char* output_filename;
    /** filename for --aes-keys/-k*/
    char* aeskeys_filename;
    /** output file stream */
    FILE* output;
    /** ENUM of the used method */
    int method;
    /** Flag of --help/-h */
    int help_flag;
    /** Flag of --version */
    int version_flag;
    /** Flag of --verbose/-v */
    int verbose_flag;
    /** Flag of printed warning about underflow, when only one thread is running */
    int printedWarningFlag;
    /** Flag for --aes-ctr */
    int aes_flag;
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

/**
 * Parse arguments and save flags/values to cnf_t* config.
 */
int parse_args(int argc, char** argv, cnf_t* config);



/** load keys from file saved in config into AES_CFG
 *
 * @param config
 *
 * @return  FILE_ERRORS enum
 */
int load_keys(cnf_t * config);

/** Load a single line from given file. Key and nonce will be returned
 *  by parameter.
 *
 * @param file
 * @param key
 * @param nonce
 *
 * @return FILE_ERRORS enum
 */
int load_key_line(
          FILE*file,
          unsigned char ** key,
          unsigned int *key_len,
          unsigned char ** nonce,
          unsigned int *nonce_len
          );  

#endif  // RDRAND_GEN_H
