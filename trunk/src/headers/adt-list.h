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

void *list_alloc(const size_t );
void list_free(PList *, PDelegate *);

PList *list_get_next(PList * );
void list_set_next(PList *, PList *);
char list_has_next(PList *);

PGenericList *gen_list_alloc(void);
void gen_list_free(PGenericList *, PDelegate *);
PGenericList *gen_list_alloc_chain(const unsigned int chain_length);
void gen_list_free_chain(PGenericList *L, PDelegate *free_elm_fnc);
void gen_list_free_elm(PGenericList *, PDelegate *);
void *gen_list_get_elm(PGenericList * );
void gen_list_set_elm(PGenericList *, void * ) ;

unsigned long int list_num_allocated_pointers(void);

#endif /* LIST_H_ */
