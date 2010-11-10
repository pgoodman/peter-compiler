/*
 * function.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <func-function.h>

void *function_identity(void *X) {
    return X;
}

enum {
    PTR_TO_UINT32 = sizeof(void *) / sizeof(uint32_t)
};

uint32_t function_hash_pointer(void *ptr) {
    unsigned i;
    uint32_t m = 0;
    union {
        void *ptr;
        uint32_t parts[PTR_TO_UINT32];
    } trans;

    trans.ptr = ptr;

    for(i = 0; i < PTR_TO_UINT32; ++i) {
        m += trans.parts[i];
    }

    return m;
}
