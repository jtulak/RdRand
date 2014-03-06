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
    Now the legal stuff is done. This file contain the library itself.
*/
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include "./librdrand.h"
#include "./librdrand-aes.private.h"
#include "./librdrand-aes.h"


/*****************************************************************************/
// {{{ misc
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
// }}} misc

// {{{ keys_allocate/free
int keys_allocate(unsigned int amount, size_t key_length) {
    // test for valid numbers
    if (!isPowerOfTwo(key_length) || amount == 0)
        return 0;
    AES_CFG.keys.amount = amount;
    AES_CFG.keys.key_length = key_length;
    AES_CFG.keys.nonce_length = key_length/2; 


//    printf("allocating: %zu B x %zu\n",AES_CFG.keys.key_length,amount);
    AES_CFG.keys.keys = malloc(AES_CFG.keys.key_length * amount);
    AES_CFG.keys.nonces = malloc(AES_CFG.keys.nonce_length * amount);

    if (AES_CFG.keys.keys == NULL || AES_CFG.keys.nonces == NULL)
        return 0;

    keys_mem_lock();
    return 1;
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
        AES_CFG.keys.amount * AES_CFG.keys.key_length);
    memset(AES_CFG.keys.nonces,
        0,
        AES_CFG.keys.amount * AES_CFG.keys.nonce_length);

    keys_mem_unlock();
    // free the memory
    free(AES_CFG.keys.keys);
    AES_CFG.keys.keys = NULL;

    free(AES_CFG.keys.nonces);
    AES_CFG.keys.nonces = NULL;

}
// }}} keys_allocate/free

// {{{ keys_mem_lock/unlock
int keys_mem_lock() {
    // TODO
    return TRUE;
}

int keys_mem_unlock() {
    // TODO
    return TRUE;
}
// }}} keys_mem_lock/unlock


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
// {{{ rdrand_set_aes_keys
int rdrand_set_aes_keys(unsigned int amount,
                        size_t key_length,
                        char **nonces,
                        char **keys) {
    AES_CFG.keys.index=0;
    AES_CFG.keys.next_counter=0;
    AES_CFG.keys_type = KEYS_GIVEN;
    if (keys_allocate(amount, key_length) == 0) {
        return 0;
    }
    memcpy(AES_CFG.keys.keys, keys, amount*key_length);
    memcpy(AES_CFG.keys.nonces, nonces, amount*(key_length/2));

    return TRUE;
}
// }}} rdrand_set_aes_keys

/**
 * Set automatic key generation.
 * /dev/random will be used as a key generator.
 */
// {{{ rdrand_set_aes_random_key
int rdrand_set_aes_random_key() {
    AES_CFG.keys_type = KEYS_GENERATED;
    AES_CFG.keys.index=0;
    AES_CFG.keys.next_counter=0;
    
    if (keys_allocate(1, DEFAULT_KEY_LEN) == 0){
        return 0;
    }
    if(key_generate() == 0){
        return 0;
    }
    return 1;
}
//}}} rdrand_set_aes_random_key

// {{{ rdrand_clean_aes
void rdrand_clean_aes() {
    keys_free();
    AES_CFG.keys_type=0;
    AES_CFG.keys.amount=0;
    AES_CFG.keys.key_length=0;
    AES_CFG.keys.nonce_length=0;
    AES_CFG.keys.next_counter=0;

}
// }}} rdrand_clean_aes


/**
 * Get an array of 64 bit random values.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired.
 *
 * All output from rdrand is passed through AES-CTR encryption.
 *
 * Either rdrand_set_aes_keys or rdrand_set_aes_random_key
 * has to be set in advance.
 * 
 * @param  dest        [description]
 * @param  count       [description]
 * @param  retry_limit [description]
 * @return             [description]
 */
// {{{ rdrand_get_bytes_aes_ctr
unsigned int rdrand_get_bytes_aes_ctr(
    void *dest,
    const unsigned int count,
    int retry_limit) {

    if (--AES_CFG.keys.next_counter <= 0) {
        if (AES_CFG.keys_type == KEYS_GIVEN) { 
            keys_change(); // set a new random index 
            keys_randomize(); // set a new random timer
        } else { // KEYS_GENERATED
            //key_generate();
        }
    }
    // TODO

    return 0;
}

// }}} rdrand_get_bytes_aes_ctr


// {{{ keys and randomizing
/**
 * Set key index for AES to another random one.
 * Will call keys_shuffle at the end.
 * Used when rdrand_set_aes_keys() was set.
 */
int keys_change() {
    unsigned int buf;
    if (RAND_bytes((unsigned char*)&buf, sizeof(unsigned int)) != 1) {
        // TODO report error
        fprintf(stderr, "ERROR: can't change keys index, not enough entropy!\n");
        return 0;
    }
    AES_CFG.keys.index = ((double)buf / UINT_MAX)*AES_CFG.keys.amount;
    return 1;
}

/**
 * Set a random timeout for new key generation/step.
 * Called on every key change.
 */
int keys_randomize() {
    unsigned int buf;
    if (RAND_bytes((unsigned char*)&buf, sizeof(unsigned int)) != 1) { 
        // TODO report error
        fprintf(stderr, "ERROR: can't change keys index, not enough entropy!\n");
        return 0;
    } 
    AES_CFG.keys.next_counter = ((double)buf/UINT_MAX)*MAX_COUNTER;
    return 1;
}

/**
 * Generate a random key.
 * Used when rdrand_set_aes_random_key() was set.
 * TODO
 */
int key_generate() {
    unsigned char buf[MAX_KEY_LENGTH] = {};
    if (RAND_bytes(buf, AES_CFG.keys.key_length) != 1) { 
        // TODO report error
        fprintf(stderr, "ERROR: can't generate key, not enough entropy!\n");
        return 0;
    }
    memcpy(AES_CFG.keys.keys[0],buf, AES_CFG.keys.key_length);
    return 1;
}
// }}} keys and randomizing

