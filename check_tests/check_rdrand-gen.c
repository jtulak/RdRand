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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
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
#include <stdio.h>
#include <check.h>
#include "./tools.h"
#include "../src/librdrand.h"
#include "../src/librdrand-aes.private.h"
#include "../src/librdrand-aes.h"
#include "../src/rdrand-gen.h"

#define KEYS_FILE "keys.txt"
#define FIXED_KEY "c96b8a45affc5c9050378dd32168c381"
#define FIXED_NONCE "41e31e41e3f8c26f"

#define TRUE 1
#define FALSE 0

extern aes_cfg_t AES_CFG;

// {{{ str_compare
int str_compare(char * a, char * b)
{
    if (a == NULL){
        if( a == b) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    if(strcmp(a,b) != 0)
        return FALSE;
    return TRUE;

}
// }}} str_compare

// {{{ compareConfigs
int compareConfigs(cnf_t a, cnf_t b) {
    if(!str_compare(a.output_filename, b.output_filename)){
        fprintf(stderr, "ERROR: Different output_filename! \n");
        return FALSE;
    }
    if(!str_compare(a.aeskeys_filename, b.aeskeys_filename)){
        fprintf(stderr, "ERROR: Different aeskeys_filename!\n");
        return FALSE;
    }
    if (a.output != b.output) {
        fprintf(stderr, "ERROR: Different output!\n");
        return FALSE;
    }
    if (a.method != b.method) {
        fprintf(stderr, "ERROR: Different method!\n");
        return FALSE;
    }
    if (a.help_flag != b.help_flag) {
        fprintf(stderr, "ERROR: Different help_flag!\n");
        return FALSE;
    }
    if (a.version_flag != b.version_flag) {
        fprintf(stderr, "ERROR: Different version_flag!\n");
        return FALSE;
    }
    if (a.verbose_flag != b.verbose_flag) {
        fprintf(stderr, "ERROR: Different verbose_flag!\n");
        return FALSE;
    }
    if (a.printedWarningFlag != b.printedWarningFlag) {
        fprintf(stderr, "ERROR: Different printedWarningFlag!\n");
        return FALSE;
    }
    if (a.threads != b.threads) {
        fprintf(stderr, "ERROR: Different threads! %u/%u\n",a.threads,b.threads);
        return FALSE;
    }
    if (a.bytes != b.bytes) {
        fprintf(stderr, "ERROR: Different bytes! %zd/%zd\n",a.bytes,b.bytes);
        return FALSE;
    }
    if (a.blocks != b.blocks) {
        fprintf(stderr, "ERROR: Different blocks! %zd/%zd\n",a.blocks,b.blocks);
        return FALSE;
    }
    if (a.chunk_size != b.chunk_size) {
        fprintf(stderr, "ERROR: Different chunk_size! %zd/%zd\n",
            a.chunk_size, b.chunk_size);
        return FALSE;
    }
    if (a.chunk_count != b.chunk_count) {
        fprintf(stderr, "ERROR: Different chunk_count! %zd/%zd\n",
            a.chunk_count, b.chunk_count);
        return FALSE;
    }
    if (a.ending_bytes != b.ending_bytes) {
        fprintf(stderr, "ERROR: Different ending_bytes! %zd/%zd\n",
            a.ending_bytes,b.ending_bytes);
        return FALSE;
    }
    return TRUE;
}
// }}} compareConfigs

/** ******************************************************************/
/**                      arguments parsing                           */
/** ******************************************************************/
// {{{
START_TEST (parseArgs_no_args)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    cnf_t cc = DEFAULT_CONFIG_SETTING;
    cc.chunk_size=MAX_CHUNK_SIZE;
    // arguments
    int argc = 1;
    char *argv[] = {"rdrand-gen"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_SUCCESS);
    ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_help)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    cnf_t cc = DEFAULT_CONFIG_SETTING;
    cc.chunk_size=MAX_CHUNK_SIZE;
    cc.help_flag=1;
    // arguments
    int argc = 2;
    char *argv[] = {"rdrand-gen","-h"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_SUCCESS);
    ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_aes)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    cnf_t cc = DEFAULT_CONFIG_SETTING;
    cc.chunk_size=MAX_CHUNK_SIZE;
    cc.aes_flag=1;
    // arguments
    int argc = 2;
    char *argv[] = {"rdrand-gen","-a"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_SUCCESS);
    ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_amount_missingNumber)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    //cnf_t cc = DEFAULT_CONFIG_SETTING;
    // arguments
    int argc = 2;
    char *argv[] = {"rdrand-gen","--amount"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_FAILURE);
    //ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_amount_decimalNumber)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    cnf_t cc = DEFAULT_CONFIG_SETTING;
    cc.bytes=3;
    cc.ending_bytes=3; // difference in bytes between bytes and blocks
    // arguments
    int argc = 3;
    char *argv[] = {"rdrand-gen","--amount", "3.14"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_SUCCESS);
    ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_amount_simpleNumber)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    cnf_t cc = DEFAULT_CONFIG_SETTING;
    cc.bytes=214;
    cc.blocks=26; // 214/8 
    cc.chunk_size=13;
    cc.chunk_count=1;
    cc.ending_bytes=6; // 214 - 26*8 (blocks)
    // arguments
    int argc = 3;
    char *argv[] = {"rdrand-gen","--amount","214"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_SUCCESS);
    ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_amount_suffixK)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    cnf_t cc = DEFAULT_CONFIG_SETTING;
    cc.bytes=1024;
    cc.blocks=128; // bytes/8
    cc.chunk_size=64; // blocks/2
    cc.chunk_count=1;
    cc.ending_bytes=0; // difference in bytes between bytes and blocks
    // arguments
    int argc = 3;
    char *argv[] = {"rdrand-gen","--amount","1k"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_SUCCESS);
    ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_amount_suffixK2)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    cnf_t cc = DEFAULT_CONFIG_SETTING;
    cc.bytes=51200;
    cc.blocks=6400; // bytes/8
    cc.chunk_size=2048; // blocks/2
    cc.chunk_count=1;
    cc.ending_bytes=18432; // difference in bytes between bytes and blocks
    // arguments
    int argc = 3;
    char *argv[] = {"rdrand-gen","--amount","50k"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_SUCCESS);
    ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_amount_suffixK3)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    cnf_t cc = DEFAULT_CONFIG_SETTING;
    cc.bytes=102400;
    cc.blocks=12800; // bytes/8
    cc.chunk_size=2048; // blocks/2
    cc.chunk_count=3;
    cc.ending_bytes=4096; // difference in bytes between bytes and blocks
    // arguments
    int argc = 3;
    char *argv[] = {"rdrand-gen","--amount","100k"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_SUCCESS);
    ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_amount_suffix_bad)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    //cnf_t cc = DEFAULT_CONFIG_SETTING;
    // arguments
    int argc = 3;
    char *argv[] = {"rdrand-gen","--amount","1D"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_FAILURE);
    //ck_assert(compareConfigs(config, cc));
}
END_TEST


START_TEST (parseArgs_amount_negative)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    //cnf_t cc = DEFAULT_CONFIG_SETTING;
    // arguments
    int argc = 3;
    char *argv[] = {"rdrand-gen","--amount","-1024"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_FAILURE);
    //ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_threads_negative)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    //cnf_t cc = DEFAULT_CONFIG_SETTING;
    // arguments
    int argc = 3;
    char *argv[] = {"rdrand-gen","--threads","-1"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_FAILURE);
   // ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_threads_withoutNumber)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    //cnf_t cc = DEFAULT_CONFIG_SETTING;
    // arguments
    int argc = 2;
    char *argv[] = {"rdrand-gen","--threads"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_FAILURE);
   // ck_assert(compareConfigs(config, cc));
}
END_TEST

START_TEST (parseArgs_threads_positive)
{
    // default config
    cnf_t config = DEFAULT_CONFIG_SETTING;
    // correct result
    cnf_t cc = DEFAULT_CONFIG_SETTING;
    cc.chunk_size=MAX_CHUNK_SIZE;
    cc.threads=2;
    // arguments
    int argc = 3;
    char *argv[] = {"rdrand-gen","--threads","2"};
    // call
    ck_assert(parse_args(argc, argv,&config) == EXIT_SUCCESS);
    ck_assert(compareConfigs(config, cc));
}
END_TEST



Suite *
parseArgs_suite (void)
{
  Suite *s = suite_create ("Parse arguments suite");
  TCase *tc;

  tc = tcase_create ("Basic arguments");
  tcase_add_test (tc, parseArgs_no_args);
  tcase_add_test (tc, parseArgs_help);
  tcase_add_test (tc, parseArgs_aes);
  suite_add_tcase (s, tc);

  tc = tcase_create ("Amount");
  tcase_add_test (tc, parseArgs_amount_missingNumber);
  tcase_add_test (tc, parseArgs_amount_decimalNumber);
  tcase_add_test (tc, parseArgs_amount_simpleNumber);
  tcase_add_test (tc, parseArgs_amount_suffixK);
  tcase_add_test (tc, parseArgs_amount_suffixK2);
  tcase_add_test (tc, parseArgs_amount_suffixK3);
  tcase_add_test (tc, parseArgs_amount_suffix_bad);
  tcase_add_test (tc, parseArgs_amount_negative);
  suite_add_tcase (s, tc);


  tc = tcase_create ("Threads");
  tcase_add_test (tc, parseArgs_threads_negative);
  tcase_add_test (tc, parseArgs_threads_withoutNumber);
  tcase_add_test (tc, parseArgs_threads_positive);
  suite_add_tcase (s, tc);

  return s;
}
// }}}

/** ******************************************************************/
/**                       keys loading                               */
/** ******************************************************************/
// {{{
// {{{ keys_read_line
START_TEST (keys_read_line){
    FILE *f;
    unsigned char **keys, **nonces;
    unsigned int key_len, nonce_len, res;
    unsigned char fixed_key[RDRAND_MAX_KEY_LENGTH], fixed_nonce[RDRAND_MAX_KEY_LENGTH];

    hex2byte(FIXED_KEY, DEFAULT_KEY_LEN*2, fixed_key, DEFAULT_KEY_LEN);
    hex2byte(FIXED_NONCE, DEFAULT_KEY_LEN, fixed_nonce, DEFAULT_KEY_LEN/2);
    
    
    keys = malloc (sizeof(unsigned char *));
    nonces=malloc (sizeof(unsigned char *));
    keys[0]= malloc(RDRAND_MAX_KEY_LENGTH);
    nonces[0]= malloc(RDRAND_MAX_KEY_LENGTH);

    f=fopen(KEYS_FILE, "r");

    // BEGIN    
    // load key
    res = load_key_line(f, keys, &key_len, nonces, &nonce_len);
    ck_assert_msg(res == E_OK, "Bad return code from load_key_lin: %u\n",res);

    ck_assert(memcmp(keys[0], fixed_key, key_len) == 0);
    ck_assert(memcmp(nonces[0], fixed_nonce, nonce_len) == 0);
    // load empty line at the end
    res = load_key_line(f, keys, &key_len, nonces, &nonce_len);
    ck_assert_msg(res == E_EOF, "Bad return code from load_key_lin: %u\n",res);
    // END

    fclose(f);
}
END_TEST
// }}} keys_read_line


// {{{ keys_open_file
START_TEST (keys_open_file){
    cnf_t cfg = {.aeskeys_filename=KEYS_FILE};
    unsigned char fixed_key[RDRAND_MAX_KEY_LENGTH], fixed_nonce[RDRAND_MAX_KEY_LENGTH];
    AES_CFG.keys.key_current = NULL;

    hex2byte(FIXED_KEY, DEFAULT_KEY_LEN*2, fixed_key, DEFAULT_KEY_LEN);
    hex2byte(FIXED_NONCE, DEFAULT_KEY_LEN, fixed_nonce, DEFAULT_KEY_LEN/2);
    
    ck_assert(load_keys(&cfg)==E_OK);

    ck_assert(AES_CFG.keys.amount == 1);
    ck_assert(memcmp(AES_CFG.keys.keys[0], fixed_key, AES_CFG.keys.key_length) == 0);
    ck_assert(memcmp(AES_CFG.keys.nonces[0], fixed_nonce, AES_CFG.keys.key_length/2) == 0);
    
}
END_TEST
// }}} keys_open_file

Suite *
keys_load_suite (void) {
  Suite *s = suite_create ("Keys loading suite");
  TCase *tc;

  tc = tcase_create ("Opening the key file");
  tcase_add_test (tc, keys_read_line);
  tcase_add_test (tc, keys_open_file);
  suite_add_tcase (s, tc);
 
  return s;
}
// }}}

/** *******************************************************************/
/**             MAIN                                                  */
/** *******************************************************************/
// {{{
int main (void){
	Suite *s;
  	SRunner *sr;
	if(rdrand_testSupport() == RDRAND_UNSUPPORTED){
	  fprintf(stderr, 
		"RdRand is not supported on this CPU! Can't run.\n");
	  exit(EXIT_FAILURE);
	}

	int number_failed =0;
	/* Standard suites */
	s = parseArgs_suite ();
	sr = srunner_create (s);

    s = keys_load_suite ();
	srunner_add_suite(sr, s);


	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);


	if(number_failed == 0)
	{
	  fprintf(stderr,"\n-----------------\n");
	  return EXIT_SUCCESS;
	}

	fprintf(stderr,"\n-----------------\n");
	return EXIT_FAILURE;

}
#endif // CHECK_RDRAND_GEN_INCLUDE
