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
// bind current key to openssl, return 0 on failure
int key_to_openssl() {
    if ( EVP_CipherInit_ex( 
                &(AES_CFG.en),
                EVP_aes_128_ctr(),
                NULL,
                AES_CFG.keys.key_current,
                AES_CFG.keys.nonce_current,
                1 ) != 1 ) { 
        // enc=1 => encryption, enc=0 => decryption
        perror("EVP_CipherInit_ex");
        return 0;
    };
    return 1;
}
// }}} misc

// {{{ keys_allocate/free
int keys_allocate(unsigned int amount, size_t key_length) {
    // test for valid numbers
    if (!isPowerOfTwo(key_length) || amount == 0)
        return 0;
    AES_CFG.keys.amount = amount;
    AES_CFG.keys.key_length = key_length;
    //AES_CFG.keys.nonce_length = key_length/2; 

    // init OpenSSL
    EVP_CIPHER_CTX_init( &(AES_CFG.en) );

    // allocate first level of array
    AES_CFG.keys.keys = malloc(sizeof(char*) *  amount);
    AES_CFG.keys.nonces = malloc(sizeof(char*) * amount);
    if (AES_CFG.keys.keys == NULL || AES_CFG.keys.nonces == NULL)
        return 0;
    // and lock it
    keys_mem_lock(AES_CFG.keys.keys, amount*sizeof(char*));
    keys_mem_lock(AES_CFG.keys.nonces, amount*sizeof(char*));

    // block
    {
        unsigned int i;
        for (i=0; i < amount; i++){
            // for keys and nonces
            // allocate strings
            AES_CFG.keys.keys[i]=malloc(key_length * sizeof(char));
            // set it zero
            memset(AES_CFG.keys.keys[i], 0, AES_CFG.keys.key_length);
            // lock it
            keys_mem_lock (AES_CFG.keys.keys[i], AES_CFG.keys.key_length);

            AES_CFG.keys.nonces[i]=malloc(key_length * sizeof(char));
            memset(AES_CFG.keys.nonces[i], 0, key_length);
            keys_mem_lock (AES_CFG.keys.nonces[i], key_length);
        }
    }
    // set current to [0]
    AES_CFG.keys.key_current = AES_CFG.keys.keys[0];
    AES_CFG.keys.nonce_current = AES_CFG.keys.nonces[0];

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

    AES_CFG.keys.key_current = NULL;
    AES_CFG.keys.nonce_current = NULL;
    AES_CFG.keys.index = 0;

    // Destroy keays in memory.
    // Overwrite all keys and nonces, then free them.
    // At the end, do the same for the arrays.
    {
        unsigned int i;
        for (i=0; i < AES_CFG.keys.amount; i++) {
            memset(AES_CFG.keys.keys[i], 0, AES_CFG.keys.key_length);
            keys_mem_unlock (AES_CFG.keys.keys[i], AES_CFG.keys.key_length);
            free(AES_CFG.keys.keys[i]);

            memset(AES_CFG.keys.nonces[i], 0, AES_CFG.keys.key_length);
            keys_mem_unlock(AES_CFG.keys.nonces[i], AES_CFG.keys.key_length);
            free(AES_CFG.keys.nonces[i]);
        }
    }

    memset(AES_CFG.keys.keys, 0, AES_CFG.keys.amount*sizeof(char*));
    memset(AES_CFG.keys.nonces, 0, AES_CFG.keys.amount*sizeof(char*));

    keys_mem_unlock(AES_CFG.keys.keys, AES_CFG.keys.amount*sizeof(char*));
    keys_mem_unlock(AES_CFG.keys.nonces, AES_CFG.keys.amount*sizeof(char*));
    
    free(AES_CFG.keys.keys);
    free(AES_CFG.keys.nonces);

    AES_CFG.keys.keys = NULL;
    AES_CFG.keys.nonces = NULL;

    // clean openssl
    if ( EVP_CIPHER_CTX_cleanup(&(AES_CFG.en)) != 1 ) {
        perror("EVP_CIPHER_CTX_cleanup");
    }
}
// }}} keys_allocate/free

// {{{ keys_mem_lock/unlock
int keys_mem_lock(void * ptr, size_t len) {
    // TODO
    (void) ptr;
    (void) len;
    return TRUE;
}

int keys_mem_unlock(void * ptr, size_t len) {
    // TODO
    (void) ptr;
    (void) len;
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
                        unsigned char **nonces,
                        unsigned char **keys) {
    AES_CFG.keys.index=0;
    AES_CFG.keys.next_counter=0;
    AES_CFG.keys_type = KEYS_GIVEN;
    if (keys_allocate(amount, key_length) == 0) {
        return 0;
    }
    
    { // subblock for var. i
        unsigned int i;
        for (i=0; i<amount; i++) {
            memcpy(AES_CFG.keys.keys[i], keys[i], key_length);
            memcpy(AES_CFG.keys.nonces[i], nonces[i], (key_length/2));
        }
    }
    // random index
    keys_change();
    return 1;
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

/**
 * Perform cleaning of all AES related settings:
 * Discard keys, ...
 */
// {{{ rdrand_clean_aes
void rdrand_clean_aes() {
    keys_free();
    AES_CFG.keys_type=0;
    AES_CFG.keys.amount=0;
    AES_CFG.keys.key_length=0;
//    AES_CFG.keys.nonce_length=0;
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
 * TODO parallel encryption/generation
 * 
 * @param  dest        destination location
 * @param  count       bytes to generate
 * @param  retry_limit how many times to retry the RdRand instruction
 * @return             amount of sucessfully generated and ecrypted bytes
 */
// {{{ rdrand_get_bytes_aes_ctr
unsigned int rdrand_get_bytes_aes_ctr(
    void *dest,
    const unsigned int count,
    int retry_limit) {

    // allow enough space in output buffer for additional block (padding)
    unsigned char output[MAX_BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
    unsigned char buf[MAX_BUFFER_SIZE];
    unsigned int buffers, tail, i, generated=0;
    int out_len;

    // keys change and such
    counter();

    memset(output, 0, MAX_BUFFER_SIZE);
    // compute cycles 
    buffers = count/MAX_BUFFER_SIZE;
    tail = count % MAX_BUFFER_SIZE;
        
    for (i=0; i < buffers; i++) {
        // generate full buffer
        if(rdrand_get_bytes_retry(buf, MAX_BUFFER_SIZE, retry_limit) != MAX_BUFFER_SIZE) {
            return generated;
        }
        // encrypt full buffer
         if( EVP_EncryptUpdate(&(AES_CFG.en), output, &out_len, buf, MAX_BUFFER_SIZE) != 1 ) {
            perror("EVP_EncryptUpdate");
            return generated;
        };

        memcpy(dest + i*MAX_BUFFER_SIZE, output, MAX_BUFFER_SIZE);
        generated += MAX_BUFFER_SIZE;

    }
    
    if(tail) {
        // generate tail
        if(rdrand_get_bytes_retry(buf, tail, retry_limit) != tail) {
            return generated;
        }
        // encrypt tail
         if( EVP_EncryptUpdate(&(AES_CFG.en), output, &out_len, buf, tail) != 1 ) {
            perror("EVP_EncryptUpdate");
            return generated;
        };
        memcpy(dest + i*MAX_BUFFER_SIZE, output, tail);
        generated += tail;
    }

    // TODO is this important?
    //if ( EVP_EncryptFinal_ex(&(AES_CFG.en), output, &out_len) != 1 ) {
    //    perror("EVP_CIPHER_CTX_cleanup");
    //    return 1;
    //}


    return generated;
}

// }}} rdrand_get_bytes_aes_ctr

/**
 * Decrement counter and if needed, change used key.
 * TODO to count not the start of get_bytes, but amount of generated bytes
 */
// {{{ counter
void counter() {
    if (AES_CFG.keys.next_counter-- == 0) {
        //perror("!!! DEBUG: KEY CHANGED !!!\n");
        if (AES_CFG.keys_type == KEYS_GIVEN) { 
            keys_change(); // set a new random index 
            keys_randomize(); // set a new random timer
        } else { // KEYS_GENERATED
            key_generate(); // generate a new key and nonce
            keys_randomize(); // set a new random timer
        }
    }
}
// }}}

// {{{ keys and randomizing
/**
 * Set key index for AES to another random one.
 * Will call keys_shuffle at the end.
 * Used when rdrand_set_aes_keys() was set.
 */
int keys_change() {
    unsigned int buf;
    if (RAND_bytes((unsigned char*)&buf, sizeof(unsigned int)) != 1) {
        fprintf(stderr, "ERROR: can't change keys index, not enough entropy!\n");
        return 0;
    }
    AES_CFG.keys.index = ((double)buf / UINT_MAX)*AES_CFG.keys.amount;
    AES_CFG.keys.key_current = AES_CFG.keys.keys[AES_CFG.keys.index];
    AES_CFG.keys.nonce_current = AES_CFG.keys.nonces[AES_CFG.keys.index];

    key_to_openssl();
    return 1;
}

/**
 * Set a random timeout for new key generation/step.
 * Called on every key change.
 */
int keys_randomize() {
    unsigned int buf;
    if (RAND_bytes((unsigned char*)&buf, sizeof(unsigned int)) != 1) { 
        fprintf(stderr, "ERROR: can't change keys index, not enough entropy!\n");
        return 0;
    } 
    AES_CFG.keys.next_counter = ((double)buf/UINT_MAX)*MAX_COUNTER;
    return 1;
}

/**
 * Generate a random key.
 * Used when rdrand_set_aes_random_key() was set.
 */
int key_generate() {
    unsigned char buf[MAX_KEY_LENGTH] = {};
    if (RAND_bytes(buf, AES_CFG.keys.key_length) != 1) { 
        fprintf(stderr, "ERROR: can't generate key, not enough entropy!\n");
        return 0;
    }
    memcpy(AES_CFG.keys.keys[0],buf, AES_CFG.keys.key_length);

    if (RAND_bytes(buf, AES_CFG.keys.key_length) != 1) { 
        fprintf(stderr, "ERROR: can't generate nonce, not enough entropy!\n");
        return 0;
    }
    memcpy(AES_CFG.keys.nonces[0],buf, AES_CFG.keys.key_length);
    key_to_openssl();
    return 1;
}
// }}} keys and randomizing

