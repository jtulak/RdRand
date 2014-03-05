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
#include "../src/librdrand-aes.private.h"
#include "../src/librdrand-aes.h"

extern aes_cfg_t AES_CFG;
/** ******************************************************************/
/**                      aes setup                                   */
/** ******************************************************************/
// {{{ AES setup
// {{{ allocation tests
START_TEST(aes_malloc_bad) {
  ck_assert(keys_allocate(0, 128) == FALSE);
  ck_assert(keys_allocate(3, 127) == FALSE);
}
END_TEST

START_TEST(aes_malloc_correct) {
  ck_assert(keys_allocate(3, 128) == TRUE);
  keys_free();
}
END_TEST
// }}} allocation tests

// {{{ keys manual setting 
START_TEST(aes_set_keys_start) {
  unsigned char ** guu;

  size_t amount = 3;
  size_t key_length = 32;
  size_t nonce_length = 16;
  char *nonces[] = {
    "aa",
    "bb",
    "cc"
  };
  char *keys[] = {
    "aaaa",
    "bbbb",
    "cccc"
  };


  ck_assert(rdrand_set_aes_keys(amount, key_length, nonces, keys) == TRUE);
  
  ck_assert(AES_CFG.keys_type == KEYS_GIVEN);
  ck_assert(AES_CFG.keys.amount == amount);
  ck_assert(AES_CFG.keys.key_length == key_length);
  ck_assert(AES_CFG.keys.nonce_length == nonce_length);
  ck_assert(AES_CFG.keys.index == 0);

// FIXME Throw away the ** and make it a void*ptr;
// For strings make a convert function
  guu = AES_CFG.keys.keys;
  printf("size: %zu\n",sizeof(guu));
  printf("size: %zu\n",sizeof(keys));
  printf("%c # %c\n",guu[2][3],keys[2][3]);
  ck_assert(memcmp(AES_CFG.keys.keys[0], keys[0],amount) == 0);
  ck_assert(memcmp(AES_CFG.keys.keys[1], keys[1],amount) == 0);
  ck_assert(memcmp(AES_CFG.keys.keys[2], keys[2],amount) == 0);

  ck_assert(memcmp(AES_CFG.keys.nonces[0], nonces[0],amount) == 0);
  ck_assert(memcmp(AES_CFG.keys.nonces[1], nonces[1],amount) == 0);
  ck_assert(memcmp(AES_CFG.keys.nonces[2], nonces[2],amount) == 0);
}
END_TEST

START_TEST(aes_set_keys_end) {
  rdrand_clean_aes();
  ck_assert(AES_CFG.keys_type == 0);
  ck_assert(AES_CFG.keys.amount == 0);
  ck_assert(AES_CFG.keys.key_length == 0);
  ck_assert(AES_CFG.keys.nonce_length == 0);
  ck_assert(AES_CFG.keys.index == 0);
  ck_assert(AES_CFG.keys.keys == NULL);
  ck_assert(AES_CFG.keys.nonces == NULL);
}
END_TEST
// }}} keys manual setting



Suite *
aes_creation_suite(void) {
  Suite *s = suite_create("AES creation suite");
  TCase *tc;

  tc = tcase_create("Mallocs");
  tcase_add_test(tc, aes_malloc_bad);
  tcase_add_test(tc, aes_malloc_correct);
  suite_add_tcase(s, tc);

  tc = tcase_create("Settings");
  tcase_add_test(tc, aes_set_keys_start);

  tcase_add_test(tc, aes_set_keys_end);
  suite_add_tcase(s, tc);

  return s;
}
// }}} AES setup
/** *******************************************************************/
/**             MAIN                                                  */
/** *******************************************************************/

int main(void) {
  Suite *s;
  SRunner *sr;
/*
  if (rdrand_testSupport() == RDRAND_UNSUPPORTED) {
    fprintf(stderr,
    "RdRand is not supported on this CPU! Can't run.\n");
    exit(EXIT_FAILURE);
  }
*/
  int number_failed;

  /* Standard suites */
  s = aes_creation_suite();
  sr = srunner_create(s);


  // s = aes_creation_suite ();
  // srunner_add_suite(sr, s);


  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);


  if (number_failed == 0) {
    fprintf(stderr, "\n-----------------\n");
    return EXIT_SUCCESS;
  }

  fprintf(stderr, "\n-----------------\n");
  return EXIT_FAILURE;
}
