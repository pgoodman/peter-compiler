/*
 * list.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef LIST_H_
#define LIST_H_

#include "std-include.h"
#include "func-delegate.h"

typedef struct PList {
    void *_next; /* sketchy! :P */
} PList;

typedef struct PGenericList {
    PList _;
    void *_elm;
} PGenericList;

void *list_alloc($$ const size_t );
void list_free($$ void *, PDelegate );

PList *list_get_next($$ void * );
void list_set_next($$ void *, void * );
char list_has_next($$ void *);

PGenericList *gen_list_alloc($);
void gen_list_free($$ PGenericList *, PDelegate );
void gen_list_free_elm($$ PGenericList *, PDelegate );
void *gen_list_get_elm($$ PGenericList * );
void gen_list_set_elm($$ PGenericList *, void * ) ;

#endif /* LIST_H_ */
