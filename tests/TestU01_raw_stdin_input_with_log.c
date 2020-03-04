/* vim: set expandtab cindent fdm=marker ts=2 sw=2: */

/* Compilation and usage example
 * gcc -Wall -Wextra -O2 -o TestU01_raw_stdin_input_with_log TestU01_raw_stdin_input_with_log.c -ltestu01
 * 
 * gcc -Wall -Wextra -g -O2 -c -o TestU01_raw_stdin_input_with_log.o TestU01_raw_stdin_input_with_log.c
 * gcc -Wall -Wextra -g -O2  -o TestU01_raw_stdin_input_with_log TestU01_raw_stdin_input_with_log.o -ltestu01
 * 
 * Memory checks
 * gcc -Wall -Wextra -g -O2  -o TestU01_raw_stdin_input_with_log TestU01_raw_stdin_input_with_log.o -ltestu01 -lmcheck
 * 
 * Non-standard location of TestU01 libraries at /dev/shm/A/include and /dev/shm/A/lib:
 * gcc -I /dev/shm/A/include -Wall -g -O2 -c -o TestU01_raw_stdin_input_with_log.o TestU01_raw_stdin_input_with_log.c
 * gcc -L /dev/shm/A/lib -Wall -g -O2  -o TestU01_raw_stdin_input_with_log TestU01_raw_stdin_input_with_log.o -ltestu01 -Wl,-rpath=/dev/shm/A/lib
 * 
 * Usage:
 * Run Normal Crush battery of tests
 * csprng-generate | ./TestU01_raw_stdin_input_with_log -n  > normal.txt
 * 
 * ./TestU01_raw_stdin_input_with_log -s </dev/urandom 
 */

/* {{{ Copyright notice
 * Copyright (C) 2011, 2012 Jirka Hladky
 * 
 * This file is part of CSRNG http://code.google.com/p/csrng/
 * 
 * CSRNG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * CSRNG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with CSRNG.  If not, see <http://www.gnu.org/licenses/>.
 * }}} */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <argp.h>
#include <limits.h>

#include <unif01.h>
#include <bbattery.h>
#include <arpa/inet.h>

//Allow files bigger than 2GB on 32-bit Linux systems
#define _FILE_OFFSET_BITS 64

#define HANDLE_ENDIANES
//Assuming that input is in BIG ENDIAN byte order

#define NUMBER_OF_SMALL_CRUSH_TESTS 11
#define NUMBER_OF_CRUSH_TESTS 97
#define NUMBER_OF_BIG_CRUSH_TESTS 107

#define GCC_VERSION (__GNUC__ * 10000 \
+ __GNUC_MINOR__ * 100 \
+ __GNUC_PATCHLEVEL__)


typedef struct {
  unsigned char* buf;                //Buffer to pass values
  unsigned int total_size;           //Total size of buffer
  unsigned char* buf_start;          //Start of valid data
  unsigned int valid_data_size;      //Size of valid data
  unsigned int read_size;            //How many bytes are attemted to be read during one fread call
  int eof_detected;                  //EOF has been detected
} rng_buf_type;

typedef struct {
  unsigned long long number_of_32_bits;
  unsigned long long number_of_bytes_written_to_file;
  FILE* fd_out;
  char* output_filename;
  char* output_filename_template;
  int overwrite;
  int length_of_output_filename;   //Number of bytes - including the trailing null byte ('\0') reserved for output_filename
  unsigned int counter;            //New filename is constructed as  snprintf(output_filename, length_of_output_filename, "%s_%06d", output_filename_template, counter)
  unsigned int counter_max;
  unsigned int counter_error;      //Count number of errors when opening an output file. We will quit if more than 200 file open errors will occur
  char error_message[200];
  int errsv;
} output_sta_type;

typedef struct {
  rng_buf_type rng_buf;
  output_sta_type output_sta;
} STDIN_state_type;


void
dump_hex_byte_string (const unsigned char* data, const unsigned int size, const char* message) {
  unsigned int i;
  if (message)
    fprintf(stderr,"%s",message);
  
  for (i=0; i<size; ++i) {
    fprintf(stderr,"%02x",data[i]);
  }
  fprintf(stderr,"\n");
}

void *util_Malloc (size_t size)
{
  void *p;
  errno = 0;
  p = malloc (size);
  if (p == NULL) {
    error (EXIT_FAILURE, errno, "\nERROR: malloc failed:\n");
    return NULL;     /* to eliminate a warning from the compiler */
  } else
    return p;
}

void *util_Calloc (size_t count, size_t esize)
{
  void *p;
  errno = 0;
  p = calloc (count, esize);
  if (p == NULL) {
    error (EXIT_FAILURE, errno, "\nERROR: calloc failed:\n");
    return NULL;     /* to eliminate a warning from the compiler */
  } else
    return p;
}

void *util_Free (void *p)
{
  if (p == NULL)
    return NULL;
  free (p);
  return NULL;
}

/*
 * Open file for writing. Two different modes:
 * 1) overwrite = 0 (false) => open file specified under "path"
 * 2) overwrite = 1 (true)  => try to generate filename from template pattern
 *   In this mode: new filename is returned in path pointer
 *   size is length (inclusing null byte) of memory reserved for path
 *   counter => current current to try out. New value will be returned
 *   counter_max => maximum allowed counter value. Currently "999999"
 *
 */ 
FILE* open_for_writing_with_lock(const int overwrite, char *path, const int size, const char *template, unsigned int *counter, const unsigned int counter_max, unsigned int* counter_error) {
  int fd;
  int rc;
  FILE* FILEP=NULL;
  int flags;
  const unsigned int counter_error_max = 200;
  
  if ( overwrite ) {
    flags = O_WRONLY  | O_CREAT | O_TRUNC;
  } else {
    flags = O_WRONLY  | O_CREAT | O_EXCL;
  }
  
  start:
  
  do {
    if ( ! overwrite ) {
      snprintf(path, size, "%s_%06d",template, *counter);
      ++ *counter;
    }
    fd = open( path, flags, 0600);
    if( fd == -1 )  {
      if (  ! overwrite ) ++ *counter_error;
      error(0, errno,"Cannot open file '%s' for writing.\n", path);
    }
  } while ( (! overwrite) && *counter<=counter_max && *counter_error < counter_error_max && fd == -1 );
  
  if ( fd == -1 ) return NULL;
  
  rc = flock(fd, LOCK_EX | LOCK_NB); // grab exclusive lock, fail if can't obtain.
  if (rc ) {
    close( fd );
    error(0, errno,"Cannot get exclusive lock on file '%s'\n", path);
    if (  ! overwrite ) ++ *counter_error;
    if ( ! overwrite && *counter<=counter_max && *counter_error < counter_error_max ) {
      goto start;
    } else {
      return NULL;
    }
  }
  
  FILEP = fdopen ( fd, "w" );
  if ( FILEP == NULL ) {
    close ( fd );
    error(0, errno,"Cannot fdopen for writing file descriptor %d associated with file '%s'\n", fd, path);
    if (  ! overwrite ) ++ *counter_error;
    if ( ! overwrite && *counter<=counter_max && *counter_error < counter_error_max ) {
      goto start;
    } else {
      return NULL;
    }
  }
  
  return FILEP;
  
}

int close_file (output_sta_type* output_sta) {
  int return_status = 0;
  
  if ( output_sta->output_filename != NULL ) {
    fprintf(stderr, "%llu bytes were written to the file \"%s\". This corresponds to ",  output_sta->number_of_bytes_written_to_file,  output_sta->output_filename);
    fprintf(stderr, "%llu MiB values.\n",  output_sta->number_of_bytes_written_to_file/1048576LLU );
    if ( output_sta -> fd_out != NULL) {
      if ( fclose(output_sta->fd_out) ) { 
        error(0, errno,"Error when closing file \"%s\"\n", output_sta->output_filename);
        return_status = 1;
      }
      output_sta -> fd_out = NULL;
    }
  }
  return return_status;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  init_buf
 *  Description:  read_size => maximum read_size used for fread
 *                Maximum amount of data requested form buffer: 4 bytes
 *                Worst case scenario: we need 4 bytes from buffer but buffer has only 3 bytes remaining
 *                We will fill the buffer with read_size bytes => minimum size of buffer is 3 + read_size 
 * =====================================================================================
 */
int
init_buf ( rng_buf_type* data, int read_size )
{
  const int max_number_of_bytes_requested_at_one_time = 4;
  data->read_size = read_size;
  data->buf     = (unsigned char*) malloc ( read_size + max_number_of_bytes_requested_at_one_time );
  if (  data->buf==NULL ) {
    fprintf ( stderr, "\nDynamic memory allocation failed\n" );
    return (1);
  }
  data->total_size = read_size + max_number_of_bytes_requested_at_one_time;
  data->valid_data_size = 0;
  data->buf_start = data->buf;
  data->eof_detected = 0;
  return 0;
}               /* -----  end of function init_buf  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  destroy_buf
 *  Description:  
 * =====================================================================================
 */
int
destroy_buf ( rng_buf_type* data )
{
  free (data->buf);
  data->buf     = NULL;
  data->buf_start = NULL;
  data->total_size = 0;
  data->valid_data_size = 0;
  data->read_size = 0;
  return 0;
}               /* -----  end of function destroy_buf  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  buf_to_file
 *  Description:  It will write containts of buffer to FILE
 *  offset is needed in scenarios when buffer was refilled (rewind has occured) which means that some data has been written to file already
 *  When disk is full we can either 1) close file 2) exit wit error
 */
int buf_to_file(rng_buf_type* data, unsigned int offset, output_sta_type* output_sta ) {
  unsigned char* new_data;
  int new_data_size, bytes_written;
  
  assert( offset < data->valid_data_size);
  assert(  output_sta->fd_out != NULL );
  
  new_data = data->buf_start + offset;
  new_data_size = data->valid_data_size - offset;
  bytes_written = fwrite (new_data, 1, new_data_size, output_sta->fd_out);
  output_sta->number_of_bytes_written_to_file += bytes_written;
  if ( bytes_written <  new_data_size )  {
    output_sta->errsv = errno;
    error(0, output_sta->errsv,"Error when writing data to file \"%s\"\n", output_sta->output_filename);
    snprintf( output_sta->error_message, sizeof(output_sta->error_message), "Error when writing data to file \"%s\"\n", output_sta->output_filename);
    close_file(output_sta);
    return -1;
  }
  
  if ( data->eof_detected && output_sta -> fd_out != NULL) {
    close_file(output_sta);
    return -1;
  }
  return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  fill_buf
 *  Description:  
 * =====================================================================================
 */
int
fill_buf ( rng_buf_type* data, unsigned int min_length_of_valid_data, output_sta_type* output_sta )
{
  unsigned int bytes_read;
  unsigned int bytes_requested;
  unsigned int bytes_to_fill_the_buffer;
  unsigned int offset;
  
  int exit_status=0;
  
  assert(min_length_of_valid_data <= data->total_size);
  if ( data->eof_detected ) {
    if ( data->valid_data_size < min_length_of_valid_data ) {
      return(1);
    } else {
      return(0);
    }
  }
  
  // 1. Rewind buffer
  if ( data->valid_data_size ) {
    memmove(data->buf, data->buf_start, data->valid_data_size);
  }
  data->buf_start =  data->buf;
  offset = data->valid_data_size; //How many bytes are coming from previous buffer (and are already saved on disk)
  
  // 2. Fill buffer
  while ( data->valid_data_size < min_length_of_valid_data ) {
    bytes_to_fill_the_buffer = data->total_size - data->valid_data_size;
    //Minimum of data->read_size and bytes_to_fill_the_buffer
    bytes_requested = data->read_size < bytes_to_fill_the_buffer ? data->read_size : bytes_to_fill_the_buffer;
    //memcpy(data->buf_start + data->valid_data_size, output, bytes_requested);
    bytes_read = fread(data->buf_start + data->valid_data_size, 1, bytes_requested, stdin);
    if ( bytes_read == 0 ) {
      if (feof(stdin)) {
        fprintf(stderr,"# stdin_input_raw(): EOF detected\n");
        data->eof_detected = 1;
      } else {
        fprintf(stderr,"# stdin_input_raw(): Error: %s\n", strerror(errno));
      }
      if ( data->valid_data_size < min_length_of_valid_data ) {
        exit_status = 1;
        break;
      } else {
        break;
      }
    }
    data->valid_data_size += bytes_read;
  }
  
  if ( output_sta->fd_out != NULL ) {
    if ( offset < data->valid_data_size ) buf_to_file (data, offset, output_sta);
  }
  return exit_status;
}               /* -----  end of function csprng_fill_buf  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_unsigned_int_buf
 *  Description:  
 * =====================================================================================
 */
unsigned int
get_unsigned_int_buf ( rng_buf_type* data, output_sta_type* output_sta )
{
  unsigned char* temp;
  static const unsigned int bytes_needed=4;   //TestU01 requires exactly 32bits
  unsigned int result;
  
  if (  bytes_needed > data->total_size ) {
    fprintf ( stderr, "\nBuffer does not support such big data sizes\n" );
    fprintf ( stderr, "\nBytes requested: %u, buffer size: %u\n", bytes_needed, data->total_size);
    if ( output_sta -> fd_out != NULL) {
      close_file (output_sta);
    }
    exit (EXIT_FAILURE);
  }
  
  if ( bytes_needed > data->valid_data_size ) {
    fill_buf (data, data->read_size > bytes_needed ? data->read_size : bytes_needed, output_sta );
    if ( bytes_needed > data->valid_data_size ) {
      fprintf ( stderr, "\nRequested to fill the buffer has failed.\n" );
      fprintf ( stderr, "Bytes requested: %u, bytes available: %u\n", bytes_needed, data->valid_data_size);
      if ( output_sta -> fd_out != NULL) {
        close_file (output_sta);
      }
      exit (EXIT_FAILURE);
    }
  }
  
  data->valid_data_size -= bytes_needed;
  temp = data->buf_start;
  data->buf_start += bytes_needed;
  
  #ifdef HANDLE_ENDIANES
  result = ((unsigned int) (*temp) ) << 24;
  temp += 1;
  result |= ((unsigned int) (*temp) ) << 16;
  temp += 1;
  result |= ((unsigned int) (*temp) ) << 8;
  temp += 1;
  result |= ((unsigned int) (*temp) );
  #else
  result = *((unsigned int*) temp );
  #endif
  
  //dump_hex_byte_string (temp-3, sizeof(unsigned int), "Big endian number:\t");
  //dump_hex_byte_string ((unsigned char *)&result, sizeof(unsigned int), "32-bit integer:\t");
  
  return(result);
  
}               /* -----  end of function get_unsigned_int_buf  ----- */


static rng_buf_type internal_status;

unsigned int get_unsigned_int_void ( ) {
  return get_unsigned_int_buf(&internal_status, NULL);
}

static unsigned long STDIN_U01_Bits (void *param, void *state) {
  unsigned int result;
  //int return_code;
  
  STDIN_state_type* STDIN_state = state;
  
  result = get_unsigned_int_buf( &STDIN_state->rng_buf, &STDIN_state->output_sta);
  ++STDIN_state->output_sta.number_of_32_bits;
  
  #if 0
  return_code = fwrite (&result, 4, 1, STDIN_state->fd_out);
  if ( return_code <  1 )  {
    error(EXIT_FAILURE, errno, "ERROR: fwrite '%s'", STDIN_state->output_filename);
}
#endif
return result;
}

static double STDIN_U01_Double (void *param, void *state) {
  return STDIN_U01_Bits (param, state) / 4294967296.0;
}

static void STDIN_U01_State (void *state) {
  
  STDIN_state_type* STDIN_state = state;
  output_sta_type* output_sta = &STDIN_state->output_sta;
  
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("%llu 32-bits random numbers were used for the test. This corresponds to\n",  output_sta->number_of_32_bits);
  printf("%llu bytes values and ",  output_sta->number_of_32_bits * 4LLU);
  printf("%llu MiB values.\n",  output_sta->number_of_32_bits/262144LLU);
  
  if ( output_sta->output_filename != NULL ) {
    printf("%llu bytes were written to the file \"%s\". This corresponds to ",  output_sta->number_of_bytes_written_to_file,  output_sta->output_filename);
    printf("%llu MiB values.\n",  output_sta->number_of_bytes_written_to_file/1048576LLU );
    if ( output_sta->error_message[0] != '\0' ) {
      printf("Following error has occured during the test:\n");
      printf("%s%s\n", output_sta->error_message,  strerror(output_sta->errsv));
      memset( output_sta->error_message , '\0', sizeof(output_sta->error_message));
      output_sta->errsv = 0;
    }
  }
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  
  
  #if 0
  long long int check = output_sta->number_of_bytes_written_to_file -  output_sta->number_of_32_bits * 4LLU - (long long) (STDIN_state->rng_buf.valid_data_size);
  printf("output_sta->number_of_bytes_written_to_file -  output_sta->number_of_32_bits * 4LLU - STDIN_state->rng_buf.valid_data_size\n%llu - %llu - %d = %lld\n",
  output_sta->number_of_bytes_written_to_file,
  output_sta->number_of_32_bits * 4LLU,
  STDIN_state->rng_buf.valid_data_size,
  check);
  #endif
  
  if ( output_sta->fd_out != NULL ) {
    assert(output_sta->number_of_bytes_written_to_file ==  output_sta->number_of_32_bits * 4LLU + STDIN_state->rng_buf.valid_data_size);
    if ( fclose(output_sta->fd_out) ) error(0, errno,"Error when closing file \"%s\"\n",  output_sta->output_filename);
  }
  
  output_sta->number_of_32_bits = 0;
  output_sta->number_of_bytes_written_to_file = 0;
  
  if ( output_sta->output_filename != NULL ) {
    
    output_sta->fd_out = open_for_writing_with_lock( output_sta->overwrite, 
                                                     output_sta->output_filename, output_sta->length_of_output_filename,
                                                     output_sta->output_filename_template, &output_sta->counter, output_sta->counter_max,
                                                     &output_sta->counter_error);
    
    if ( output_sta->fd_out == NULL ) {
      error(EXIT_FAILURE, 0,"Opening of file '%s' has failed.\n", output_sta->output_filename);
    }
    
    //Write remaining data from buffer to the new file
    if (  STDIN_state->rng_buf.valid_data_size > 0 ) buf_to_file(&STDIN_state->rng_buf, 0, output_sta);
  }
  
}

/*
 * if ( filename == NULL ) => no output
 *overwrite == 1 => each test will overwrite filename
 *overwrite == 0 => filename is used as template, random bits are stored in file filename_00000x
 */

unif01_Gen *create_STDIN_U01 (const char* filename, int overwrite)
{
  unif01_Gen *gen;
  STDIN_state_type *state;
  output_sta_type* output_sta;
  size_t leng;
  char name[160];
  
  
  gen = util_Malloc (sizeof (unif01_Gen));
  gen->state = state = util_Malloc (sizeof (STDIN_state_type));
  output_sta = &state->output_sta;
  
  if ( filename != NULL ) {
    output_sta->overwrite = overwrite ? 1 : 0;
    leng = strlen (filename);
    if ( ! overwrite ) {
      output_sta->output_filename_template = util_Calloc (leng + 1, sizeof (char));
      output_sta->length_of_output_filename = leng + 1 + 7;
      output_sta->output_filename = util_Calloc (leng + 1 + 7, sizeof (char));  //7 Characters to hold number of file. Format "_DDDDDD"
      output_sta->counter_max = (unsigned int) (1e6 - 1);
      strncpy (output_sta->output_filename_template, filename, leng);
    } else {
      output_sta->length_of_output_filename = leng + 1;
      output_sta->output_filename = util_Calloc (leng + 1, sizeof (char));
      strncpy (output_sta->output_filename, filename, leng);
      output_sta->output_filename_template = NULL;
      output_sta->counter_max = 0;
    }
  } else {
    output_sta->output_filename_template = NULL;
    output_sta->output_filename = NULL;
    output_sta->overwrite = 1;
    output_sta->length_of_output_filename = 0;
  }
  
  output_sta->counter = 0;
  output_sta->number_of_32_bits = 0;
  output_sta->number_of_bytes_written_to_file = 0;
  output_sta->counter_error = 0;
  memset( output_sta->error_message , '\0', sizeof(output_sta->error_message));
  output_sta->errsv = 0;
  init_buf(&state->rng_buf, 8192);
  
  if ( filename != NULL ) {
    output_sta->fd_out = open_for_writing_with_lock( output_sta->overwrite, 
                                                     output_sta->output_filename, output_sta->length_of_output_filename,
                                                     output_sta->output_filename_template, &output_sta->counter, output_sta->counter_max,
                                                     &output_sta->counter_error);
    if ( output_sta->fd_out == NULL ) {
      error(EXIT_FAILURE, 0,"Opening of file '%s' has failed.\n", output_sta->output_filename);
    }
  } else {
    output_sta->fd_out = NULL;
  }
  
  gen->param = NULL;
  gen->Write = STDIN_U01_State;
  gen->GetU01 = STDIN_U01_Double;
  gen->GetBits = STDIN_U01_Bits;
  
  if ( filename != NULL ) {
    if ( output_sta->overwrite ) {
      snprintf(name, sizeof(name), "STDIN with log of the random data to the file %s,\noverwritten by every test", output_sta->output_filename);
    } else {
      snprintf(name, sizeof(name), "STDIN with log of the random data to the file template %s", output_sta->output_filename_template);
    }
  } else {
    snprintf(name, sizeof(name), "STDIN");
  }
  
  leng = strlen (name);
  gen->name = util_Calloc (leng + 2, sizeof (char));
  strncpy (gen->name, name, leng);
  
  return gen;
}

void delete_STDIN_U01 (unif01_Gen * gen)
{
  STDIN_state_type *state = gen->state;
  output_sta_type* output_sta = &state->output_sta;
  
  if ( output_sta->fd_out != NULL ) fclose(output_sta->fd_out);
  
  if ( ! output_sta->overwrite ) {
    if ( output_sta->output_filename !=NULL ) output_sta->output_filename = util_Free ( output_sta->output_filename );
    if ( output_sta->output_filename_template !=NULL ) output_sta->output_filename = util_Free ( output_sta->output_filename_template );
    output_sta->overwrite = 1;
    output_sta->length_of_output_filename = 0;
  } else {
    if ( output_sta->output_filename !=NULL ) output_sta->output_filename = util_Free ( output_sta->output_filename );
  }
  
  
  destroy_buf (&state->rng_buf);
  
  gen->state = util_Free (state);
  gen->name = util_Free (gen->name);
  util_Free (gen);
}

//{{{ double double_pow(double x, uint8_t e)
//Computes x^e
double double_pow(double x, uint8_t e)
{
  if (e == 0) return 1;
  if (e == 1) return x;
  
  double tmp = double_pow(x, e/2);
  if (e%2 == 0) return tmp * tmp;
  else return x * tmp * tmp;
}
//}}}

#if GCC_VERSION > 40500
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

static struct argp_option options[] = {
  { 0,                                0,      0,  0,  "\33[4mTests\33[m" },
  {"small",                         's',      0,  0,  "Small Crush Battery of Randomness tests" },
  { 0,                                0, 0,       0,  "" },
  {"normal",                        'n',      0,  0,  "Crush Battery of Randomness tests" },
  { 0,                                0, 0,       0,  "" },
  {"big",                           'b',      0,  0,  "Big Crush Battery of Randomness tests"},
  { 0,                                0, 0,       0,  "" },
  {"repeat",                        'r',"TEST:COUNT",  0,  "Repeat test number \"TEST\" from Crush Battery of Randomness tests \"COUNT\" times"}, 
  { 0,                                0, 0,       0,  "" },
  {"repeat-small",                  700,"TEST:COUNT",  0,  "Repeat test number \"TEST\" from Small Crush Battery of Randomness tests \"COUNT\" times"}, 
  { 0,                                0, 0,       0,  "" },
  {"repeat-big",                    701,"TEST:COUNT",  0,  "Repeat test number \"TEST\" from Big Crush Battery of Randomness tests \"COUNT\" times"}, 
  { 0,                                0, 0,       0,  "" },
  {"Rabbit",                        800,  "EXP",  0,       "Applies the Rabbit battery of tests to the STDIN using at most 2^EXP bits for each test. Minimum value 13 (~1KiB)" },
  { 0,                                0, 0,       0,  "" },
  {"Alphabit",                      801,  "EXP:R:S",  0,       "Applies the Alphabit battery of tests at most 2^EXP bits for each test. "
    "The bits themselves are processed as blocks of 32 bits (unsigned integers). For each block of 32 "
    "bits, the R most significant bits are dropped, and the test is applied on the S following bits. If one "
    "wants to test all bits of the stream, one should set R = 0 and S = 32. If one wants to test only 1 "
    "bit out of 32, one should set S = 1."}, 
    { 0,                                0, 0,       0,  "" },
    {"time",                          't',  "NUM",  OPTION_ARG_OPTIONAL,  "Run speed test of RNG, using NUM 32-bit random numbers. NUM is parsed as double number. Default: 1e8"},
    { 0,                                0,      0,  0,  "\33[4mOutput to file\33[m" },
    {"output",                        'o', "FILENAME", 0, "Output the randomn data used for test to the file FILENAME.\n"
      "If OVERWRITE is ON (default), the file will be overwritten each time the new test will start.\n"
      "For overwrite mode please consider using RAM FS (for example /dev/shm).\n"
      "If OVERWRITE is OFF, for each test a new output file will be created using name pattern FILENAME_NUMBER.\n"
      "Please note that amount of random data for one test can be ~2GB and the data can quickly fill the whole available disk space.\n"},
      {"no-overwrite",                  600,      0,  0,  "OVERWRITE OFF. "
        "For each test a new output file will be created using name pattern FILENAME_NUMBER, for example FILENAME_000001\n"},
        {"overwrite",                     601,      0, OPTION_HIDDEN,  "OVERWRITE ON"},
        { 0 }
};

#if GCC_VERSION > 40500
#pragma GCC diagnostic pop
#endif


typedef struct {
  int exp;
  int r;
  int s;
} rs_type;  //See Alphabit test


/* Used by main to communicate with parse_opt. */
struct arguments {
  int sflag;
  int nflag;
  int bflag;
  int rflag_s;
  int rflag_n;
  int rflag_b;
  rs_type aflag;
  int rabbitflag;
  long int tflag;
  int rep_n[NUMBER_OF_CRUSH_TESTS];
  int rep_s[NUMBER_OF_SMALL_CRUSH_TESTS];
  int rep_b[NUMBER_OF_BIG_CRUSH_TESTS];
  int overwrite_output_files;
  char* output_filename;
};

static struct arguments arguments = {
  .sflag = 0,
  .nflag = 0,
  .bflag = 0,
  .rflag_s = 0,
  .rflag_n = 0,
  .rflag_b = 0,
  .aflag = {0, 0, 0},
  .rabbitflag = 0,
  .tflag = 0,
  .overwrite_output_files = 1,
  .output_filename = NULL,
}; 

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
  /* Get the input argument from argp_parse, which we
   *     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;
  int limit;
  int* flag_pointer;
  int* rep_pointer;
  char* name[3];
  name[0]="repeat-small";
  name[1]="repeat";
  name[2]="repeat-big";
  char* name_to_print;
  
  switch (key) {
    case 's':
      arguments->sflag = 1;
      break;
    case 'n':
      arguments->nflag = 1;
      break;
    case 'b':
      arguments->bflag = 1;
      break;
    case 800: {
      long l;
      char *p;
      l = strtol(arg, &p, 10);
      if ((p == arg) || (*p != 0) || errno == ERANGE || (l < 13) || (l > 50) )
        argp_error(state, "ERROR when parsing argument of --Rabbit option \"%s\". "
        "Expecting number in range < 13 - 50 >. Number is expected as integer, see \"man strtol\" for details.", arg);
      arguments->rabbitflag = l;
      break;
    }
    case 801: {
      long int n;
      char *p, *p1, *p2;
      n =  strtol(arg, &p, 10);
      if ((p == arg) || (*p != ':') || errno == ERANGE || (n < 13) || (n > 50) )
        argp_error(state, "ERROR when parsing argument EXP of --Alphabit option \"%s\". "
        "Expecting number in range < 13 - 50 >. Format is EXP:R:S. Substring is \"%s\"", arg, p);
      arguments->aflag.exp = n;
      
      p++; //Skip ':'
      n = strtol(p, &p1, 10);
      if ((p1 == p) || (*p1 != ':') || errno == ERANGE || (n < 0) || (n > 31) )
        argp_error(state, "ERROR when parsing argument R of --Alphabit option \"%s\". "
        "Expecting number in range < 0 - 31 >. Format is EXP:R:S. Substrings are \"%s\" \"%s\"", arg, p, p1);
      arguments->aflag.r = n;
      
      p1++; //Skip ':'
      n = strtol(p1, &p2, 10);
      if ((p2 == p1) || (*p2 != 0) || errno == ERANGE || (n < 0) || (n > 32) )
        argp_error(state, "ERROR when parsing argument S of --Alphabit option \"%s\". "
        "Expecting number in range < 0 - 32 >. Format is EXP:R:S. Substrings are \"%s\" \"%s\" \"%s\"", arg, p, p1, p2);
      arguments->aflag.s = n;
      
      if ( arguments->aflag.r + arguments->aflag.s > 32 ) {
        argp_error(state, "ERROR when parsing arguments R & S of --Alphabit option \"%s\". "
        "R + S has to be <= 32. Provided values are R: %d and S %d\n", arg, arguments->aflag.r, arguments->aflag.s);
        
      }
      break;
    }          
    case 700:
      limit = NUMBER_OF_SMALL_CRUSH_TESTS - 2;
      flag_pointer = &arguments->rflag_s;
      rep_pointer = arguments->rep_s;
      name_to_print = name[0];
    case 701:
      if ( key == 701 ) {  
        limit = NUMBER_OF_BIG_CRUSH_TESTS - 2;
        flag_pointer = &arguments->rflag_b; 
        rep_pointer = arguments->rep_b;
        name_to_print = name[2];
      }
    case 'r': {
      long int n,value;
      char *p;
      char *p1;
      if ( key == 'r') {
        limit = NUMBER_OF_CRUSH_TESTS - 2;
        flag_pointer = &arguments->rflag_n; 
        rep_pointer = arguments->rep_n;
        name_to_print = name[1];
      }
      n = strtol(arg, &p, 10);
      if ( n<0 || n > (NUMBER_OF_CRUSH_TESTS - 2) ) {
        argp_error(state, "Test number has to be in range 0 - %d. Processing input: \"%s\"", limit, arg);
        break;
      }
      if ((p == arg) || (*p != ':')) {
        argp_error(state, "--%s takes the format N:M where N is number of the test and M is count. Processing input: \"%s\", substring is \"%s\"", name_to_print, arg, p);
        break;
      }
      p++;
      value = strtol(p, &p1, 10);
      if ((p1 == p) || (*p1 != 0) || errno == ERANGE || (value < 0) || (value >= INT_MAX) || ( value > 1000 ) ) {
        argp_error(state, "Number of tests has to be in range 0 - %d. Processing input: \"%s\", substring is \"%s\"", 1000, arg, p);
        break;
      }
      rep_pointer[n] = value;
      *flag_pointer = 1;
      break;
    }
    case 't':
      if (arg == NULL) {
        arguments->tflag = 100000000;
      } else {
        double d;
        long l;
        char *p;
        d = strtod(arg, &p);
        if ((p == arg) || (*p != 0) || errno == ERANGE || (d < 1.0) || (d > LONG_MAX + 0.49) )
          argp_error(state, 
                     "ERROR when parsing argument of -t option \"%s\". Expecting number in range < 1 - %ld >. Number is expected in double notation, see \"man strtod\" for details.", arg, LONG_MAX);
          else {
            l = (long int ) ( d + 0.5);
            if ( l < 1 ) 
              argp_error(state,  "ERROR when parsing -t during conversion from double %.16g value to long int %ld\n value. Expecting number long int >= 1\n", d, l);
            else 
              arguments->tflag = l;
          } 
      }
      break;
      
    case 600:
      arguments->overwrite_output_files = 0;
      break;
      
    case 601:
      arguments->overwrite_output_files = 1;
      break;
      
    case 'o':
      arguments->output_filename = arg;
      break;
      
    case ARGP_KEY_ARG:
      argp_error(state,  "No arguments are supported, only options\n");
      break;
    case ARGP_KEY_END:
      if ( arguments->tflag == 0 &&
        arguments->rflag_s == 0 &&
        arguments->rflag_n == 0 &&
        arguments->rflag_b == 0 &&
        arguments->sflag == 0 &&
        arguments->nflag == 0 &&
        arguments->bflag == 0 &&
        arguments->aflag.exp == 0 &&
        arguments->rabbitflag == 0) {
        argp_error(state,  "At least one of the options [-s] [-n] [-b] [-t[NUM]] \n"
        "[--repeat=TEST:COUNT] [--repeat-small=TEST:COUNT] [--repeat-big=TEST:COUNT] has to be used\n");
        }
        if ( arguments->overwrite_output_files == 0 && arguments->output_filename == NULL ) {
          argp_error(state,  "You have specified option \"no-overwrite\" but no output filename template. Please use option \"-o\" to specify output filename template\n");
        } 
        break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

const char *argp_program_version = "Version 1.0\nCopyright (c) 2011-2012 by Jirka Hladky";
const char *argp_program_bug_address = "< hladky DOT jiri AT gmail DOT com >";
static char doc[] ="\33[1m\33[4mExecute TestU01 tests of randomness reading data from the standard input\33[m";

/* Our argp parser. */
#if GCC_VERSION > 40500
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
static struct argp argp = { options, parse_opt, 0, doc };
#if GCC_VERSION > 40500
#pragma GCC diagnostic pop
#endif


int main (int argc, char **argv) 
{
  int i;
  for(i=0;i<NUMBER_OF_CRUSH_TESTS;++i) {
    arguments.rep_n[i] = 0;
  }
  for(i=0;i<NUMBER_OF_SMALL_CRUSH_TESTS;++i) {
    arguments.rep_s[i] = 0;
  }
  for(i=0;i<NUMBER_OF_BIG_CRUSH_TESTS;++i) {
    arguments.rep_b[i] = 0;
  }
  
  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  unif01_Gen *gen1;
  gen1 = create_STDIN_U01 (arguments.output_filename, arguments.overwrite_output_files);
  
  if (arguments.tflag) unif01_TimerSumGenWr (gen1, arguments.tflag, TRUE);
  
  if (arguments.rflag_s) bbattery_RepeatSmallCrush (gen1, arguments.rep_s); 
  if (arguments.rflag_n) bbattery_RepeatCrush (gen1, arguments.rep_n); 
  if (arguments.rflag_b) bbattery_RepeatBigCrush (gen1, arguments.rep_b); 
  
  if (arguments.sflag) bbattery_SmallCrush (gen1);
  if (arguments.nflag) bbattery_Crush (gen1);
  if (arguments.bflag) bbattery_BigCrush (gen1);
  if (arguments.rabbitflag )  bbattery_Rabbit (gen1, double_pow(2.0,arguments.rabbitflag));
  if (arguments.aflag.exp )  bbattery_Alphabit (gen1, double_pow(2.0,arguments.aflag.exp), arguments.aflag.r, arguments.aflag.s);
  
  
  delete_STDIN_U01(gen1);
  
  return 0;
}


