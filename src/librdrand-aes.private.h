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
    unsigned int amount;
    unsigned int index;
    unsigned int next_counter;
    unsigned char **nonces;
    unsigned char *nonce_current; //  pointer to the current nonce
    size_t nonce_length; // in bits
    unsigned char **keys;
    unsigned char *key_current; // pointer to the current index of keys
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
 * Decrement counter and if needed, change used key.
 */
void counter();

/**
 * Set key index for AES to another random one.
 * Used when rdrand_set_aes_keys() was set.
 * @return            1 if it was successful
 */
int keys_change();

/**
 * Set a random timeout for new key generation/step.
 * Called on every key change.
 * @return            1 if it was successful
 */
int keys_randomize();

/**
 * Generate a random key.
 * Used when rdrand_set_aes_random_key() was set.
 * @return            1 if it was successful
 */
int key_generate();


/**
 * Allocate memory for keys.
 * @param  amount     is number of keys
 * @param  key_length length of each key, must be pow(2)
 * @return            1 if it was successful
 */
int keys_allocate(unsigned int amount, size_t key_length);

/**
 * Destroy saved keys and free the memory.
 */
void keys_free();

/**
 * Lock memory to prevent saving it on swap
 * @return TRUE if it was successful
 */
int keys_mem_lock(void * ptr, size_t len);
/**
 * Unock memory from preventing saving it on swap
 * @return TRUE if it was successful
 */
int keys_mem_unlock(void * ptr, size_t len);

#endif  // LIBRDRAND_AES_PRIVATE_H_INCLUDED
