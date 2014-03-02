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
#include "./librdrand.h"
#include "./librdrand-aes.h"

#define DEFAULT_KEY_LEN 128

enum {
    KEYS_GENERATED,
    KEYS_GIVEN
};

enum {
    SUCCESS,
    ERROR
};


typedef struct s_keys {
    size_t amount;
    unsigned int index;
    unsigned char **nonces;
    size_t nonce_length;
    unsigned char **keys;
    size_t key_length;
} t_keys;

typedef struct s_aes_cfg {
    t_keys keys;
    int keys_type;
} t_aes_cfg;

/**
 * Set key for AES to a next in the list.
 * Will call keys_shuffle at the end.
 * Used when rdrand_set_aes_keys() was set.
 */
int keys_next();
/**
 * Shuffle list of keys.
 * Used when rdrand_set_aes_keys() was set.
 */
int keys_shuffle();


/**
 * Generate a random key.
 * Used when rdrand_set_aes_random_key() was set.
 */
int keys_generate();

/**
 * Set a random timeout for new key generation/step.
 * Called on every key change.
 */
unsigned keys_new_timeout();


/*****************************************************************************/
// t_buffer* BUFFER;
t_aes_cfg AES_CFG;

int keys_allocate(size_t amount, size_t key_length) {
    AES_CFG.keys.key_length = key_length;
    AES_CFG.keys.nonce_length = key_length/2;

    AES_CFG.keys.keys = calloc(AES_CFG.keys.key_length, amount);
    AES_CFG.keys.nonces = calloc(AES_CFG.keys.nonce_length, amount);

    if (AES_CFG.keys.keys == NULL || AES_CFG.keys.nonces == NULL)
        return ERROR;

    return SUCCESS;
}

/**
 * Set manually keys for AES.
 * These keys will be rotated randomly.
 */
/*
TODO: rotate just in random times, or also random order?
Maybe a shuffle at the end of the list?
*/
int rdrand_set_aes_keys(size_t amount,
                        size_t key_length,
                        unsigned char **nonce,
                        unsigned char **keys) {
    AES_CFG.keys_type = KEYS_GIVEN;
    keys_allocate(amount, key_length);

    memcpy(AES_CFG.keys.keys, keys, amount*key_length);

    // TODO some better return
    return 0;
}

/**
 * Set automatic key generation.
 * /dev/random will be used as a key generator.
 */
int rdrand_set_aes_random_key() {
    AES_CFG.keys_type = KEYS_GIVEN;
    keys_allocate(1, DEFAULT_KEY_LEN);

    // TODO some better return
    return 0;
}



