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
#include <assert.h>
#include "./tools.h"
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
  size_t key_length = 16;\
  /*size_t nonce_length = 16;*/\
  unsigned char *nonces[] = {\
    (unsigned char*) "aaaabbbb",\
    (unsigned char*) "bbbbcccc",\
    (unsigned char*) "ccccdddd"\
  };\
  unsigned char *keys[] = {\
    (unsigned char*) "aaaabbbbccccdddd",\
    (unsigned char*) "bbbbccccddddeeee",\
    (unsigned char*) "ccccddddeeeeffff"\
  };\
  ck_assert(rdrand_set_aes_keys(amount, key_length, keys, nonces) == 1);
// }}}


// {{{ aes_set_keys_start
START_TEST(aes_set_keys_start) {

    SETUP_KEYS();
    ck_assert(AES_CFG.keys_type == KEYS_GIVEN);
    ck_assert(AES_CFG.keys.amount == amount);
    ck_assert(AES_CFG.keys.key_length == key_length);
//    ck_assert(AES_CFG.keys.nonce_length == key_length/2);
    ck_assert(AES_CFG.keys.index < amount);
    
    ck_assert(AES_CFG.keys.key_current 
            == AES_CFG.keys.keys[AES_CFG.keys.index]);
    ck_assert(AES_CFG.keys.nonce_current 
            == AES_CFG.keys.nonces[AES_CFG.keys.index]);

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
    unsigned char* keys[]={(unsigned char*)"aa"};
    unsigned char* nonces[]={(unsigned char*)"b"};
    unsigned int amount = 1;
    size_t len=2;
    
    // too short key, shouldn't be allowed
    ck_assert(rdrand_set_aes_keys(amount, len, keys, nonces) == 0);

}
END_TEST
// }}} aes_set_keys_boundaries


// {{{ aes_set_keys_changing
START_TEST(aes_set_keys_changing) {
    unsigned int i, old_index, index, changes;
    SETUP_KEYS();

    old_index = AES_CFG.keys.index;
    for (i=0; i < 5; i++) {
        keys_change(); // do the change
        index = AES_CFG.keys.index; // save variable for shorter typing
        
        // test boundaries
        ck_assert_msg( index < AES_CFG.keys.amount, 
                "Generated index is out of <0,%u) boundaries. Value: %u.\n",
                AES_CFG.keys.amount, index);

        // test _current pointers
        ck_assert_msg(AES_CFG.keys.nonce_current == AES_CFG.keys.nonces[index],
                "nonce_current wasn't changed accordingly to the index!\n");
        ck_assert_msg(AES_CFG.keys.key_current == AES_CFG.keys.keys[index],
                "key_current wasn't changed accordingly to the index!\n");

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
//    ck_assert(AES_CFG.keys.nonce_length == 0);
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
//    ck_assert(AES_CFG.keys.nonce_length == AES_CFG.keys.key_length/2);
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
//    ck_assert(AES_CFG.keys.nonce_length == 0);
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
//    ck_assert(memcmp(AES_CFG.keys.nonces[0], old_key, AES_CFG.keys.nonce_length) !=  0);

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
    counter(1);
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
        counter(1);
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



// {{{ aes_keys_counter_in_generation
// Test whether counter() really counts and change key.
// Tested in generation
START_TEST(aes_keys_counter_in_generation) {
    unsigned int i, changes=0, rounds=5;
    unsigned char key[DEFAULT_KEY_LEN];
    // if the buffer is fully generated, the key has to change
    unsigned char buf[MAX_COUNTER];

    rdrand_set_aes_random_key();
    memcpy(key, AES_CFG.keys.keys[0], DEFAULT_KEY_LEN);
    
    for (i=0; i<rounds; i++) {
        ck_assert(rdrand_get_bytes_aes_ctr(buf, MAX_COUNTER, 3));
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
  tcase_add_test(tc, aes_keys_counter_in_generation);

  suite_add_tcase(s, tc);
    
  return s;
}
// }}} aes_creation_suite
// }}} AES setup

/** ******************************************************************/
/**                      aes generation                              */
/** ******************************************************************/
// {{{ AES generation
// {{{ aes_get_bytes
START_TEST (aes_get_bytes) {
    unsigned char buf[MAX_BUFFER_SIZE * 50 + 31]; 
    unsigned int i,SIZE=MAX_BUFFER_SIZE * 50 + 31;
    rdrand_set_aes_random_key();
    
    ck_assert(rdrand_get_bytes_aes_ctr(buf, SIZE, 3) == SIZE);
   
    // test whether two following buffer spaces are different.
    for (i=0; i< 49; i++) {
        ck_assert_msg(
                memcmp(
                    &buf[i*MAX_BUFFER_SIZE], 
                    &buf[(i+1)*MAX_BUFFER_SIZE], 
                    MAX_BUFFER_SIZE
                ) != 0
        , "Two following buffer spaces are identical (at i=%u)!\n",i);

    }
    // test tail if it is not the same as the previous data
    ck_assert_msg(
            memcmp(
                &buf[49*MAX_BUFFER_SIZE],
                &buf[50*MAX_BUFFER_SIZE],
                31
                ) != 0
            , "Tail is identical to previous data!\n");


    rdrand_clean_aes();
}
END_TEST
// }}} aes_get_bytes

// {{{ aes_compare_ecrypt_data
START_TEST ( aes_compare_ecrypt_data) {
  unsigned char key[16];
  unsigned char *keys[1];
  unsigned char nonce_counter[16]={0};
  unsigned char *nonces[1];
  char key_hex[32]="c96b8a45affc5c9050378dd32168c381";
  //char key_hex[32]="00000000000000000000000000000000";
  char nonce_hex[16]="41e31e41e3f8c26f"; //only upper 64-bits
  //char nonce_hex[16]="0000000000000000"; //only upper 64-bits
  unsigned char output [8192]={0};

  // expected value - gained with another program (openssl-demo/aesctr.c)
  // for input 0xFFFFFFFF...
  char expected_result_hex[64] = "2c6e98c0f3e667673bb3fe2fb1b2ca4dfb2211f3bdf0231ab266fa8a045f8562";
  unsigned char expected_result[32];

  keys[0]=key;
  nonces[0]=nonce_counter;

  hex2byte(key_hex, SIZEOF(key_hex), key, SIZEOF(key));
  hex2byte(nonce_hex, SIZEOF(nonce_hex), nonce_counter, SIZEOF(nonce_counter)/2); //Only upper 64-bits, lower bits are 0

    // printf("key: ");
    // mem_dump(*keys,16);
    // printf("nonce: ");
    // mem_dump(*nonces,16);
    // set manual keys
    rdrand_set_aes_keys(1, 16, keys, nonces);

    ck_assert_msg(rdrand_get_bytes_aes_ctr(output, 32, 3)== 32,
            "Not enough bytes generated!\n");
    //mem_dump(output, 32);

    // compare expected value
    hex2byte(expected_result_hex, 64, expected_result, 32);
    ck_assert_msg(memcmp(output, expected_result, 32)==0,
            "Encrypted value is different from expected one.\n");

    // try another runs
    ck_assert_msg(rdrand_get_bytes_aes_ctr(output, 32, 3)== 32,
            "Not enough bytes generated!\n");
    ck_assert_msg(rdrand_get_bytes_aes_ctr(output, 8000, 3)== 8000,
            "Not enough bytes generated!\n");
    // TODO compare the value

    rdrand_clean_aes();
}
END_TEST
// }}}

// {{{ aes_generation_suite
Suite *
aes_generation_suite(void) {
    Suite *s = suite_create("AES generation suite");
    TCase *tc;

    tc = tcase_create("get_bytes");
    tcase_add_test(tc, aes_get_bytes);    
    tcase_add_test(tc, aes_compare_ecrypt_data);    
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
