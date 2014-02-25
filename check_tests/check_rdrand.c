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
*/
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <check.h>
#include "../src/librdrand.h"

#define DEST_SIZE 9

#define TRUE (1==1)
#define FALSE (1==0)


// Test if given bytes in array are zeros
int test_zeros(unsigned char *arr, unsigned int from, unsigned int to)
{
	unsigned int i;
	if( from > to || to > DEST_SIZE) return FALSE;
	for (i=from; i<to; i++)
	{
		if (arr[i] != 0 ) 
		{
			return FALSE;
		}
		
	}
	return TRUE;
}


START_TEST (rdrand_step_16)
{
  unsigned char dst[DEST_SIZE] = {0};
  ck_assert_int_eq (rdrand16_step((uint16_t *)&dst),RDRAND_SUCCESS);
  ck_assert(test_zeros(dst, 2, DEST_SIZE));
  ck_assert(test_zeros(dst, 0, 2)==FALSE);
  
}
END_TEST

Suite *
rdrand_steps_methods_suite (void)
{
  Suite *s = suite_create ("Steps methods suite");

  /* Core test case */
  TCase *tc_core = tcase_create ("16");
  tcase_add_test (tc_core, rdrand_step_16);
  suite_add_tcase (s, tc_core);

  return s;
}

int main (void){
	
  if(rdrand_testSupport() == RDRAND_UNSUPPORTED){
	  fprintf(stderr, 
		"RdRand is not supported on this CPU! Can't run.\n");
	  exit(EXIT_FAILURE);
  }
	
  int number_failed;
  Suite *s = rdrand_steps_methods_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
