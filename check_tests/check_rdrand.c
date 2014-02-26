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
    Now the legal stuff is done. This file contain the tests for Check 
    unit testing.
    
    http://check.sourceforge.net/doc/check_html/check_3.html 
    
*/
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <check.h>
#include "../src/librdrand.h"

#define DEST_SIZE 9
#define ARRAY_SIZE 65
#define RETRY_LIMIT 3

#define TRUE 1
#define FALSE 0


// Set to 0 to disable printing in test_zeros/test_ones functions 
// if failure is expected.
// Don't forget to set it again to 1 after the specific test call!
int TEST_BYTES_PRINT=1;

// Test if given bytes in array are set to 0
int test_zeros(unsigned char *arr, unsigned int arr_size, unsigned int from, unsigned int to)
{
	unsigned int i;
	unsigned int errs=0;
	if( from > to || to > arr_size) 
	{
		fprintf(stderr,"Error test_zeros: invalid range! From: %u, To: %u, array size: %u.\n",from, to,DEST_SIZE);
		return FALSE;
	}
	for (i=from; i<to; i++)
	{
		if (arr[i] != 0x00 ) 
		{
			errs++;
		}
		
	}
	if(errs) 
	{
		if(TEST_BYTES_PRINT)
			fprintf(stderr,"Error: %u non-zero byte(s)!\n",errs);
		return FALSE;
	}
	return TRUE;
}

// Test if given bytes in array have all bits set to 1
// If changing unsigned char *arr to another type, change also 0xFF
// constant!
int test_ones(unsigned char *arr, unsigned int arr_size, unsigned int from, unsigned int to)
{
	unsigned int i;
	unsigned int errs=0;
	if( from > to || to > arr_size) 
	{
		fprintf(stderr,"Error test_ones: invalid range! From: %u, To: %u, array size: %u.\n",from, to,DEST_SIZE);
		return FALSE;
	}
	for (i=from; i<to; i++)
	{
		if (arr[i] != 0xFF) 
		{
			if(TEST_BYTES_PRINT)
				fprintf(stderr,"different byte: %u is %X\n",i, arr[i]);
			errs++;
		}
		
	}
	if(errs) return FALSE;
	return TRUE;
}

/** ******************************************************************/
/**                      stub  steps                                 */
/** ******************************************************************/

START_TEST (rdrand_step_16_stub)
{
  unsigned char dst[DEST_SIZE]={0};
  ck_assert_int_eq (rdrand16_step((uint16_t *)&dst),RDRAND_SUCCESS);
  // test if it wrote just into the place it should
  ck_assert(test_zeros(dst, DEST_SIZE, 2, DEST_SIZE));
  // test if it set all to 1
  ck_assert(test_ones(dst,DEST_SIZE, 0, 2));
}
END_TEST

START_TEST (rdrand_step_32_stub)
{
  unsigned char dst[DEST_SIZE] = {0};
  ck_assert_int_eq (rdrand32_step((uint32_t *)&dst),RDRAND_SUCCESS);
  
  // test if it wrote just into the place it should
  ck_assert(test_zeros(dst, DEST_SIZE, 4, DEST_SIZE));
  
  // test if it set all to 1
  ck_assert(test_ones(dst, DEST_SIZE, 0, 4));
}
END_TEST


START_TEST (rdrand_step_64_stub)
{
  unsigned char dst[DEST_SIZE] = {0};
  ck_assert_int_eq (rdrand64_step((uint64_t *)&dst),RDRAND_SUCCESS);
  
  // test if it wrote just into the place it should
  ck_assert(test_zeros(dst, DEST_SIZE, 8, DEST_SIZE));
  
  // test if it set all to 1
  ck_assert(test_ones(dst, DEST_SIZE, 0, 8));
}
END_TEST

Suite *
rdrand_stub_methods_suite (void)
{
  Suite *s = suite_create ("Stub methods suite");

  TCase *tc_steps = tcase_create ("Stub methods");
  tcase_add_test (tc_steps, rdrand_step_16_stub);
  tcase_add_test (tc_steps, rdrand_step_32_stub);
  tcase_add_test (tc_steps, rdrand_step_64_stub);
  suite_add_tcase (s, tc_steps);

  return s;
}

/** ******************************************************************/
/**                      rdrand native steps                         */
/** ******************************************************************/
START_TEST (rdrand_step_16_native)
{
  unsigned char dst[DEST_SIZE]={0};
  ck_assert_int_eq (rdrand16_step_native((uint16_t *)&dst),RDRAND_SUCCESS);
  // test if it wrote just into the place it should
  ck_assert(test_zeros(dst, DEST_SIZE, 2, DEST_SIZE));
  // test if it wrote something (rarely can fail)
  TEST_BYTES_PRINT=0;
  ck_assert_msg(test_zeros(dst, DEST_SIZE, 0, 2)==FALSE, 
		"This test can rarely fail if rdrand generates just all zeros. \
		Try it once more to be sure.\n");
  TEST_BYTES_PRINT=1;
}
END_TEST

START_TEST (rdrand_step_32_native)
{
  unsigned char dst[DEST_SIZE] = {0};
  ck_assert_int_eq (rdrand32_step_native((uint32_t *)&dst),RDRAND_SUCCESS);
  
  // test if it wrote just into the place it should
  ck_assert(test_zeros(dst, DEST_SIZE, 4, DEST_SIZE));
  
  // test if it wrote something (rarely can fail)
  TEST_BYTES_PRINT=0;
  ck_assert_msg(test_zeros(dst, DEST_SIZE, 0, 4)==FALSE, 
		"This test can rarely fail if rdrand generates just all zeros. \
		Try it once more to be sure.\n");
  TEST_BYTES_PRINT=1;
}
END_TEST


START_TEST (rdrand_step_64_native)
{
  unsigned char dst[DEST_SIZE] = {0};
  ck_assert_int_eq (rdrand64_step_native((uint64_t *)&dst),RDRAND_SUCCESS);
  
  // test if it wrote just into the place it should
  ck_assert(test_zeros(dst, DEST_SIZE, 8, DEST_SIZE));
  
  // test if it wrote something (rarely can fail)
  TEST_BYTES_PRINT=0;
  ck_assert_msg(test_zeros(dst, DEST_SIZE, 0, 8)==FALSE, 
		"This test can rarely fail if rdrand generates a byte all zeros. \
		Try it once more to be sure.\n");
  TEST_BYTES_PRINT=1;
}
END_TEST

Suite *
rdrand_native_steps_methods_suite (void)
{
  Suite *s = suite_create ("Native steps suite");

  TCase *tc_steps = tcase_create ("native steps");
  tcase_add_test (tc_steps, rdrand_step_16_native);
  tcase_add_test (tc_steps, rdrand_step_32_native);
  tcase_add_test (tc_steps, rdrand_step_64_native);
  suite_add_tcase (s, tc_steps);

  return s;
}


/** ******************************************************************/
/**                      retry  steps                                 */
/** ******************************************************************/
void rdrand_retry_16_test(int retry)
{
	 unsigned char dst[DEST_SIZE] = {0};
  ck_assert_int_eq (rdrand_get_uint16_retry((uint16_t *)&dst,retry),RDRAND_SUCCESS);
  
  // test if it wrote just into the place it should
  ck_assert(test_zeros(dst, DEST_SIZE, 2, DEST_SIZE));
  
  // test if it wrote something (rarely can fail)
  ck_assert(test_ones(dst, DEST_SIZE, 0, 2));
}

START_TEST (rdrand_retry_16)
{
  int i;
  for (i=-2; i<=RETRY_LIMIT; i++)
	rdrand_retry_16_test(i);
}
END_TEST



void rdrand_retry_32_test(int retry)
{
  unsigned char dst[DEST_SIZE] = {0};
  ck_assert_int_eq (rdrand_get_uint32_retry((uint32_t *)&dst,retry),RDRAND_SUCCESS);
  
  // test if it wrote just into the place it should
  ck_assert(test_zeros(dst, DEST_SIZE, 4, DEST_SIZE));
  
  // test if it wrote something (rarely can fail)
  ck_assert(test_ones(dst, DEST_SIZE, 0, 4));
}
START_TEST (rdrand_retry_32)
{
  int i;
  for (i=-2; i<=RETRY_LIMIT; i++)
	rdrand_retry_32_test(i);
}
END_TEST

void rdrand_retry_64_test(int retry)
{
  unsigned char dst[DEST_SIZE] = {0};
  ck_assert_int_eq (rdrand_get_uint64_retry((uint64_t *)&dst,retry),RDRAND_SUCCESS);
  
  // test if it wrote just into the place it should
  ck_assert(test_zeros(dst, DEST_SIZE, 8, DEST_SIZE));
  
  // test if it wrote something (rarely can fail)
  ck_assert(test_ones(dst, DEST_SIZE, 0, 8));
}
START_TEST (rdrand_retry_64)
{
  int i;
  for (i=-2; i<=RETRY_LIMIT; i++)
	rdrand_retry_64_test(i);
}
END_TEST

Suite *
rdrand_retry_methods_suite (void)
{
  Suite *s = suite_create ("Retry methods suite");

  TCase *tc_steps = tcase_create ("retry functions");
  tcase_add_test (tc_steps, rdrand_retry_16);
  tcase_add_test (tc_steps, rdrand_retry_32);
  tcase_add_test (tc_steps, rdrand_retry_64);
  suite_add_tcase (s, tc_steps);

  return s;
}

/** *******************************************************************/
/**                       Arrays                                      */
/** *******************************************************************/


START_TEST (array_bytes)
{
  unsigned int size=ARRAY_SIZE-1;
  unsigned int offset=16;
  
  unsigned int multiplier=1; // 1 - 8bit, 2 - 8bit, 4 - 32bit, ...
  
  /* Generate one size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_bytes_retry((uint8_t *)&dst, size/multiplier, RETRY_LIMIT), size/multiplier);
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size));
  }}}	
  
  /* Generate half size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_bytes_retry((uint8_t *)&dst, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size/2));
  }}}	
  
  /* Generate half size with offset */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_bytes_retry((uint8_t *)&dst+offset/multiplier, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2+offset, ARRAY_SIZE));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, 0, offset));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, offset, size/2));
  }}}
}
END_TEST


START_TEST (array_8)
{
  unsigned int size=ARRAY_SIZE-1;
  unsigned int offset=16;
  
  unsigned int multiplier=1; // 1 - 8bit, 2 - 8bit, 4 - 32bit, ...
  
  /* Generate one size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint8_array_retry((uint8_t *)&dst, size/multiplier, RETRY_LIMIT), size/multiplier);
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size));
  }}}	
  
  /* Generate half size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint8_array_retry((uint8_t *)&dst, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size/2));
  }}}	
  
  /* Generate half size with offset */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint8_array_retry((uint8_t *)&dst+offset/multiplier, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2+offset, ARRAY_SIZE));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, 0, offset));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, offset, size/2));
  }}}
}
END_TEST


START_TEST (array_16)
{
  unsigned int size=ARRAY_SIZE-1;
  unsigned int offset=16;
  
  unsigned int multiplier=2; // 1 - 8bit, 2 - 16bit, 4 - 32bit, ...
  
  /* Generate one size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint16_array_retry((uint16_t *)&dst, size/multiplier, RETRY_LIMIT), size/multiplier);
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size));
  }}}	
  
  /* Generate half size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint16_array_retry((uint16_t *)&dst, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size/2));
  }}}	
  
  /* Generate half size with offset */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint16_array_retry((uint16_t *)&dst+offset/multiplier, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2+offset, ARRAY_SIZE));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, 0, offset));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, offset, size/2));
  }}}
}
END_TEST


START_TEST (array_32)
{
  unsigned int size=ARRAY_SIZE-1;
  unsigned int offset=16;
  
  unsigned int multiplier=4; // 1 - 8bit, 2 - 32bit, 4 - 32bit, ...
  
  /* Generate one size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint32_array_retry((uint32_t *)&dst, size/multiplier, RETRY_LIMIT), size/multiplier);
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size));
  }}}	
  
  /* Generate half size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint32_array_retry((uint32_t *)&dst, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size/2));
  }}}	
  
  /* Generate half size with offset */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint32_array_retry((uint32_t *)&dst+offset/multiplier, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2+offset, ARRAY_SIZE));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, 0, offset));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, offset, size/2));
  }}}
}
END_TEST


START_TEST (array_64)
{
  unsigned int size=ARRAY_SIZE-1;
  unsigned int offset=16;
  
  unsigned int multiplier=8; // 1 - 8bit, 2 - 64bit, 4 - 32bit, ...
  
  /* Generate one size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint64_array_retry((uint64_t *)&dst, size/multiplier, RETRY_LIMIT), size/multiplier);
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size));
  }}}	
  
  /* Generate half size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint64_array_retry((uint64_t *)&dst, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size/2));
  }}}	
  
  /* Generate half size with offset */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint64_array_retry((uint64_t *)&dst+offset/multiplier, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2+offset, ARRAY_SIZE));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, 0, offset));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, offset, size/2));
  }}}
}
END_TEST


START_TEST (array_reseed_delay_64)
{
  unsigned int size=ARRAY_SIZE-1;
  unsigned int offset=16;
  
  unsigned int multiplier=8; // 1 - 8bit, 2 - 64bit, 4 - 32bit, ...
  
  /* Generate one size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint64_array_reseed_delay((uint64_t *)&dst, size/multiplier, RETRY_LIMIT), size/multiplier);
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size));
  }}}	
  
  /* Generate half size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint64_array_reseed_delay((uint64_t *)&dst, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size/2));
  }}}	
  
  /* Generate half size with offset */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint64_array_reseed_delay((uint64_t *)&dst+offset/multiplier, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2+offset, ARRAY_SIZE));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, 0, offset));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, offset, size/2));
  }}}
}
END_TEST



START_TEST (array_reseed_skip_64)
{
  unsigned int size=ARRAY_SIZE-1;
  unsigned int offset=16;
  
  unsigned int multiplier=8; // 1 - 8bit, 2 - 64bit, 4 - 32bit, ...
  
  /* Generate one size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint64_array_reseed_skip((uint64_t *)&dst, size/multiplier, RETRY_LIMIT), size/multiplier);
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size));
  }}}	
  
  /* Generate half size */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint64_array_reseed_skip((uint64_t *)&dst, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2, ARRAY_SIZE));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, 0, size/2));
  }}}	
  
  /* Generate half size with offset */
  {{{
	  unsigned char dst[ARRAY_SIZE] = {0};
	  ck_assert_int_eq (rdrand_get_uint64_array_reseed_skip((uint64_t *)&dst+offset/multiplier, size/(2*multiplier), RETRY_LIMIT), size/(2*multiplier));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, size/2+offset, ARRAY_SIZE));
	  // test if it wrote just into the place it should
	  ck_assert(test_zeros(dst, ARRAY_SIZE, 0, offset));
	  // test if it wrote something (rarely can fail)
	  ck_assert(test_ones(dst, ARRAY_SIZE, offset, size/2));
  }}}
}
END_TEST


Suite *
arrays_suite (void)
{
  Suite *s = suite_create ("Arrays suite");

  TCase *tc_steps = tcase_create ("arrays");
  tcase_add_test (tc_steps, array_8);
  tcase_add_test (tc_steps, array_16);
  tcase_add_test (tc_steps, array_32);
  tcase_add_test (tc_steps, array_64);
  tcase_add_test (tc_steps, array_bytes);
  tcase_add_test (tc_steps, array_reseed_delay_64);
  tcase_add_test (tc_steps, array_reseed_skip_64);
  suite_add_tcase (s, tc_steps);

  return s;
}

/** *******************************************************************/
/**             MAIN                                                  */
/** *******************************************************************/
int main (void){
  Suite *s;
  SRunner *sr;
  
  if(rdrand_testSupport() == RDRAND_UNSUPPORTED){
	  fprintf(stderr, 
		"RdRand is not supported on this CPU! Can't run.\n");
	  exit(EXIT_FAILURE);
  }
	
  int number_failed;
  
  /* Standard suites */
  s = rdrand_stub_methods_suite ();
  sr = srunner_create (s);
  
  s = rdrand_native_steps_methods_suite ();
  srunner_add_suite(sr, s);
  
  s = rdrand_retry_methods_suite ();
  srunner_add_suite(sr, s);
  
  s = arrays_suite ();
  srunner_add_suite(sr, s);
  
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  
  
  
  if(number_failed == 0)
  {
	  fprintf(stderr,"\n100%%: Everything OK.\n-----------------\n");
	  return EXIT_SUCCESS;
  }
  
  fprintf(stderr,"\nERROR: %d test(s) failed!\n-----------------\n",number_failed);
  
  return EXIT_FAILURE;
}
