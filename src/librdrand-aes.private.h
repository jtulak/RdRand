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
    Now the legal stuff is done. This file contain the library itself.
*/
#ifndef LIBRDRAND_AES_PRIVATE_H_INCLUDED
#define LIBRDRAND_AES_PRIVATE_H_INCLUDED

#ifndef TRUE
    #define TRUE 1
    #define FALSE 0
#endif

enum {
    KEYS_GENERATED,
    KEYS_GIVEN
};


typedef struct s_keys {
    size_t amount;
    unsigned int index;
    unsigned char **nonces;
    size_t nonce_length; // in bits
    unsigned char **keys;
    size_t key_length; // in bits
} t_keys;

typedef struct aes_cfg_s {
    t_keys keys;
    int keys_type;
} aes_cfg_t;

#ifdef STUB_RDRAND  // for testing
     extern aes_cfg_t AES_CFG;
#endif

/**
 * Set key for AES to a next in the list.
 * Will call keys_shuffle at the end.
 * Used when rdrand_set_aes_keys() was set.
 * TODO
 */

int keys_next();
/**
 * Shuffle list of keys.
 * Used when rdrand_set_aes_keys() was set.
 * TODO
 */
int keys_shuffle();


/**
 * Generate a random key.
 * Used when rdrand_set_aes_random_key() was set.
 * TODO
 */
int keys_generate();

/**
 * Set a random timeout for new key generation/step.
 * Called on every key change.
 * TODO
 */
unsigned keys_new_timeout();

/**
 * Allocate memory for keys.
 * @param  amount     is number of keys
 * @param  key_length length of each key, must be pow(2)
 * @return            true if it was successful
 */
int keys_allocate(size_t amount, size_t key_length);

/**
 * Destroy saved keys and free the memory.
 */
void keys_free();

/**
 * Lock memory to prevent saving it on swap
 * @return TRUE if it was successful
 */
int keys_mem_lock();
/**
 * Unock memory from preventing saving it on swap
 * @return TRUE if it was successful
 */
int keys_mem_unlock();

#endif  // LIBRDRAND_AES_PRIVATE_H_INCLUDED
