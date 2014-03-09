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
void mem_dump(unsigned char *mem, unsigned int length) {
    unsigned i;
    for (i=0; length > i; i++){
        printf("%x",mem[i]);
    }
    printf("\n");
}   
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
    unsigned char zero[DEFAULT_KEY_LEN]={};
  ck_assert(keys_allocate(3, DEFAULT_KEY_LEN) == TRUE);
  ck_assert_msg(memcmp(AES_CFG.keys.keys[0], zero, DEFAULT_KEY_LEN) == 0,
          "Allocated memory for keys wasn't deleted.\n");
  ck_assert_msg(memcmp(AES_CFG.keys.nonces[0], zero, DEFAULT_KEY_LEN/2) == 0,
          "Allocated memory for nonces wasn't deleted.\n");

  keys_free();
}
END_TEST
// }}} allocation tests

// {{{ keys manual setting 
// {{{ SETUP MACRO
// return amount of keys
#define SETUP_KEYS() \
  unsigned int amount = 3;\
  size_t key_length = 32;\
  /*size_t nonce_length = 16;*/\
  char *nonces[] = {\
    "aa",\
    "bb",\
    "cc"\
  };\
  char *keys[] = {\
    "aaaa",\
    "bbbb",\
    "cccc"\
  };\
  ck_assert(rdrand_set_aes_keys(amount, key_length, nonces, keys) == TRUE);
// }}}


// {{{ aes_set_keys_start
START_TEST(aes_set_keys_start) {

    SETUP_KEYS();
    ck_assert(AES_CFG.keys_type == KEYS_GIVEN);
    ck_assert(AES_CFG.keys.amount == amount);
    ck_assert(AES_CFG.keys.key_length == key_length);
    ck_assert(AES_CFG.keys.nonce_length == key_length/2);
    ck_assert(AES_CFG.keys.index == 0);

    ck_assert(memcmp(AES_CFG.keys.keys[0], keys[0],amount) == 0);
    ck_assert(memcmp(AES_CFG.keys.keys[1], keys[1],amount) == 0);
    ck_assert(memcmp(AES_CFG.keys.keys[2], keys[2],amount) == 0);

    ck_assert(memcmp(AES_CFG.keys.nonces[0], nonces[0],amount) == 0);
    ck_assert(memcmp(AES_CFG.keys.nonces[1], nonces[1],amount) == 0);
    ck_assert(memcmp(AES_CFG.keys.nonces[2], nonces[2],amount) == 0);


}
END_TEST
// }}} aes_set_keys_start

// {{{ aes_set_keys_boundaries
START_TEST(aes_set_keys_boundaries) {
   // TODO What could be tested there? 
}
END_TEST
// }}} aes_set_keys_boundaries


// {{{ aes_set_keys_changing
START_TEST(aes_set_keys_changing) {
    unsigned int i, old_index, index, changes;
    SETUP_KEYS();

    old_index = AES_CFG.keys.index;
    for (i=0; i < 5; i++) {
        keys_change();
        index = AES_CFG.keys.index;
        // test boundaries
        ck_assert_msg( index < AES_CFG.keys.amount, 
                "Generated index is out of <0,%u) boundaries. Value: %u.\n",
                AES_CFG.keys.amount, index);
        if (index != old_index) {
            old_index = AES_CFG.keys.index;
            changes++;
        }
    }
    ck_assert_msg(changes > 0, "No changes in index happened during multiple runs!\n");
}
END_TEST
// }}} aes_set_keys_changing

// {{{ aes_set_counter_changing
START_TEST(aes_set_counter_changing) {
    unsigned int i, old_next_counter, next_counter, changes=0;
    SETUP_KEYS();

    old_next_counter = AES_CFG.keys.next_counter;
    
    for (i=0; i < 5; i++) {
        keys_randomize();
        next_counter = AES_CFG.keys.next_counter;
        // test boundaries
        ck_assert_msg( next_counter <=MAX_COUNTER, 
                "Generated next_counter is out of <0,%u> boundaries. Value: %u.\n",
                MAX_COUNTER, next_counter);
        // test change
        if (next_counter != old_next_counter) {
            changes++;
            old_next_counter = AES_CFG.keys.next_counter;
        }
    }
    ck_assert_msg(changes > 0, "No changes in next_counter happened during multiple runs!\n");
}
END_TEST
// }}} aes_set_counter_changing

// {{{ aes_set_keys_end
START_TEST(aes_set_keys_end) {

    SETUP_KEYS();

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
// }}} aes_set_keys_end
// }}} keys manual setting


// {{{ keys generated
// {{{ aes_random_key_startup
START_TEST(aes_random_key_startup) {
    rdrand_set_aes_random_key();
    ck_assert(AES_CFG.keys.key_length == DEFAULT_KEY_LEN);
    ck_assert(AES_CFG.keys_type == KEYS_GENERATED);
    ck_assert(AES_CFG.keys.amount == 1);
    ck_assert(AES_CFG.keys.nonce_length == AES_CFG.keys.key_length/2);
    ck_assert(AES_CFG.keys.index == 0);
    ck_assert(AES_CFG.keys.keys != NULL);
    ck_assert(AES_CFG.keys.nonces != NULL);
}
END_TEST
// }}} aes_random_key_startup

// {{{ aes_random_key_end
START_TEST(aes_random_key_end) {

    rdrand_set_aes_random_key();

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
// }}} aes_random_key_end

// {{{ aes_random_key_gen
START_TEST(aes_random_key_gen) {
    unsigned char old_key[DEFAULT_KEY_LEN] = {};
    unsigned int changes=0, i;

    // generate
    ck_assert(rdrand_set_aes_random_key());
    //mem_dump(old_key, DEFAULT_KEY_LEN);
    //mem_dump(AES_CFG.keys.keys[0], AES_CFG.keys.key_length);
    //mem_dump(AES_CFG.keys.nonces[0], AES_CFG.keys.nonce_length);
    
    // test if the key was generated at the beginning 
    ck_assert_msg(memcmp(AES_CFG.keys.keys[0], old_key, AES_CFG.keys.key_length) !=  0,
            "Key wasn't generated at initialization, but is still all zero.\n");
    
    // nonce is too all zero by default, so it can be checked this way
    ck_assert(memcmp(AES_CFG.keys.nonces[0], old_key, AES_CFG.keys.nonce_length) !=  0);

    // save the generated key
    memcpy(old_key, AES_CFG.keys.keys[0], AES_CFG.keys.key_length);

    for (i=0; i<5; i++) {
        ck_assert(key_generate() == 1);
        // if there is a difference, increment counter
        if(memcmp(AES_CFG.keys.keys[0], old_key, AES_CFG.keys.key_length) != 0) {
            changes++;
            memcpy(old_key, AES_CFG.keys.keys[0], AES_CFG.keys.key_length);
         }
        // print the keys
        //mem_dump(AES_CFG.keys.keys[0],AES_CFG.keys.key_length);
    }
    ck_assert_msg(changes > 0, "No changes in generated key happened during multiple runs!\n");
}
END_TEST
// }}} aes_random_key_gen)
// }}} keys generated

// {{{ aes_keys_counter_zero
// Test whether counter() expects also zero value
START_TEST(aes_keys_counter_zero) {
    unsigned char key[DEFAULT_KEY_LEN];

    rdrand_set_aes_random_key();
    // set the zero
    AES_CFG.keys.next_counter = 0;

    memcpy(key, AES_CFG.keys.keys[0], DEFAULT_KEY_LEN);
    counter();
    // now has to change
    ck_assert(memcmp(key, AES_CFG.keys.keys[0], DEFAULT_KEY_LEN) != 0);
    
}
END_TEST
// }}}
// {{{ aes_keys_counter
// Test whether counter() really counts and change key.
START_TEST(aes_keys_counter) {
    unsigned int i, changes=0, rounds=2;
    unsigned char key[DEFAULT_KEY_LEN];

    rdrand_set_aes_random_key();
    memcpy(key, AES_CFG.keys.keys[0], DEFAULT_KEY_LEN);
    
    for (i=0; i<MAX_COUNTER*rounds+1; i++) {
        counter();
        if (memcmp(key, AES_CFG.keys.keys[0], DEFAULT_KEY_LEN) != 0) {
            changes++;
            memcpy(key, AES_CFG.keys.keys[0], DEFAULT_KEY_LEN);
        }
    }

    rdrand_clean_aes();
    // at least "rounds" times should the key change,
    // but it can change more times
    ck_assert(changes >= rounds);
    
}
END_TEST
// }}}


// {{{ aes_creation_suite
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
  tcase_add_test(tc, aes_set_keys_boundaries);
  tcase_add_test(tc, aes_set_keys_changing);
  tcase_add_test(tc, aes_set_counter_changing);

  tcase_add_test(tc, aes_random_key_startup);
  tcase_add_test(tc, aes_random_key_end);
  tcase_add_test(tc, aes_random_key_gen);

  tcase_add_test(tc, aes_keys_counter_zero);
  tcase_add_test(tc, aes_keys_counter);

  suite_add_tcase(s, tc);
    
  return s;
}
// }}} aes_creation_suite
// }}} AES setup

// {{{ AES generation
// {{{ aes_get_bytes
START_TEST (aes_get_bytes) {

}
END_TEST
// }}} aes_get_bytes
// {{{ aes_generation_suite
Suite *
aes_generation_suite(void) {
    Suite *s = suite_create("AES generation suite");
    TCase *tc;

    tc = tcase_create("get_bytes");
    tcase_add_test(tc, aes_get_bytes);    
    suite_add_tcase(s, tc);


  return s;
}
// }}} aes_generation_suite
// }}} AES generation
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


   s = aes_generation_suite ();
   srunner_add_suite(sr, s);


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
