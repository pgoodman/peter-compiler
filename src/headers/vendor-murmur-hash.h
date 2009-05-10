/*
 * murmur-hash.h
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef MURMURHASH_H_
#define MURMURHASH_H_

#include "vendor-pstdint.h"

uint32_t murmur_hash ( const char *, int32_t, uint32_t);

#endif /* MURMURHASH_H_ */
