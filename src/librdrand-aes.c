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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

/*
    Now the legal stuff is done. This file contain the library itself.
*/
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include "./librdrand.h"
#include "./librdrand-aes.private.h"
#include "./librdrand-aes.h"


/*****************************************************************************/
// t_buffer* BUFFER;
aes_cfg_t AES_CFG = {.keys={.amount=0}};

/**
 * Test if number is power of two.
 * http://stackoverflow.com/questions/600293/how-to-check-if-a-number-is-a-power-of-2
 * @param  x [description]
 * @return   [description]
 */
int isPowerOfTwo(ulong x) {
    return x && (x & (x - 1)) == 0;
}


int keys_allocate(size_t amount, size_t key_length) {
    // test for valid numbers
    if (!isPowerOfTwo(key_length) || amount == 0)
        return FALSE;
    AES_CFG.keys.amount = amount;
    AES_CFG.keys.key_length = key_length;
    AES_CFG.keys.nonce_length = key_length/2; 

    printf("allocating: %zu B x %zu\n",AES_CFG.keys.key_length,amount);
    AES_CFG.keys.keys = malloc(AES_CFG.keys.key_length * amount);
    AES_CFG.keys.nonces = malloc(AES_CFG.keys.nonce_length * amount);

    if (AES_CFG.keys.keys == NULL || AES_CFG.keys.nonces == NULL)
        return FALSE;

    keys_mem_lock();
    return TRUE;
}

/**
 * Destroy saved keys and free the memory.
 */
void keys_free() {
    if (AES_CFG.keys.keys == NULL) {
        // If there is nothing to free
        return;
    }

    // destroy keays in memory
    memset(AES_CFG.keys.keys,
        0,
        AES_CFG.keys.amount * BYTES(AES_CFG.keys.key_length));
    memset(AES_CFG.keys.nonces,
        0,
        AES_CFG.keys.amount * BYTES(AES_CFG.keys.nonce_length));

    keys_mem_unlock();
    // free the memory
    free(AES_CFG.keys.keys);
    AES_CFG.keys.keys = NULL;

    free(AES_CFG.keys.nonces);
    AES_CFG.keys.nonces = NULL;
}

int keys_mem_lock() {
    // TODO
    return TRUE;
}

int keys_mem_unlock() {
    // TODO
    return TRUE;
}

/**
 * Set manually keys for AES.
 * These keys will be rotated randomly.
 * 
 * @param  amount     Count of keys
 * @param  key_length Length of all keys in bits 
 *                    (must be pow(2))
 * @param  nonces     Array of nonces. Nonces have to be half of 
 *                    length of keys.
 * @param  keys       Array of keys. All have to be the same length.
 * @return            True if the keys were successfuly set
*/
int rdrand_set_aes_keys(size_t amount,
                        size_t key_length,
                        char **nonces,
                        char **keys) {
    AES_CFG.keys_type = KEYS_GIVEN;
    AES_CFG.keys.index = 0;
    if (keys_allocate(amount, key_length) == FALSE) {
        return FALSE;
    }
    memcpy(AES_CFG.keys.keys, keys, amount*key_length);
    memcpy(AES_CFG.keys.nonces, nonces, amount*(key_length/2));

    return TRUE;
}

/**
 * Set automatic key generation.
 * /dev/random will be used as a key generator.
 */
int rdrand_set_aes_random_key() {
    AES_CFG.keys_type = KEYS_GIVEN;
    keys_allocate(1, DEFAULT_KEY_LEN);

    //RAND_bytes(AES_CFG.keys.keys[0],)
    // TODO some better return
    return 0;
}

void rdrand_clean_aes() {
    keys_free();
    AES_CFG.keys_type=0;
    AES_CFG.keys.amount=0;
    AES_CFG.keys.key_length=0;
    AES_CFG.keys.nonce_length=0;

}



