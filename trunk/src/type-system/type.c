/*
 * type.c
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "type.h"

/**
 * Allocate a new type on the heap. This assumes that if there are inner types
 * then they have been heap-allocated. It might turn out that the type in fact
 * exists and is in use and so we will return it if it does.
 */
void *type_alloc(size_t size,
                 HashTable *all_types,
                 const char * const name, const char name_len,
                 void * inner_types[], const char inner_types_len) {
    P_Type *T,
           *inner_type;

    int i;

    /* this keeps track of type-erasure and the generic-type-erased name */
    char inner_generic_type_counter = 0,
         *type_name,
         temp_type_name[100]; /* this should be enough to hold any type-erased name */

    /* build up a new type name for this type. */
    if(inner_types_len > 0) {

        /* copy the name into the temporary type name */
        for(type_name = type_name;
            type_name < temp_type_name+name_len;
            type_name++ = name++)
            ;
    }
}
