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
#include "librdrand-aes.h"

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


