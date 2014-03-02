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
#ifndef CHECK_RDRAND_GEN_INCLUDE
#define CHECK_RDRAND_GEN_INCLUDE

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <check.h>
//#include "../src/librdrand.h"
#include "../src/rdrand-gen.h"


#define TRUE 1
#define FALSE 0


#if 0
/** ******************************************************************/
/**                      aes setup                                   */
/** ******************************************************************/

START_TEST (aes_setup_manual_keys)
{
	//ck_assert(rdrand_set_aes_keys(size_t amount, size_t key_length, unsigned char **nonce, unsigned char **keys));
  //ck_assert_int_eq (rdrand16_step((uint16_t *)&dst),RDRAND_SUCCESS);
  // test if it set all to 1
  //ck_assert(test_ones(dst,DEST_SIZE, 0, 2));
}
END_TEST


Suite *
rdrand_stub_methods_suite (void)
{
  Suite *s = suite_create ("Stub methods suite");

  TCase *tc_steps = tcase_create ("Stub methods");
  //tcase_add_test (tc_steps, rdrand_step_16_stub);
  suite_add_tcase (s, tc_steps);

  return s;
}
#endif

/** *******************************************************************/
/**             MAIN                                                  */
/** *******************************************************************/

int main (void){
	#if 0
	Suite *s;
  	SRunner *sr;
	if(rdrand_testSupport() == RDRAND_UNSUPPORTED){
	  fprintf(stderr, 
		"RdRand is not supported on this CPU! Can't run.\n");
	  exit(EXIT_FAILURE);
	}

	int number_failed =0;
	#if 0
	/* Standard suites */
	s = rdrand_stub_methods_suite ();
	sr = srunner_create (s);


	//srunner_add_suite(sr, s);


	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	#endif

	if(number_failed == 0)
	{
	  fprintf(stderr,"\n100%%: Everything OK.\n-----------------\n");
	  return EXIT_SUCCESS;
	}

	fprintf(stderr,"\nERROR: %d test(s) failed!\n-----------------\n",number_failed);

	return EXIT_FAILURE;

	#endif
	return EXIT_SUCCESS;
}


#endif // CHECK_RDRAND_GEN_INCLUDE