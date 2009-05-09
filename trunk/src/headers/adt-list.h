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

typedef struct List {
    void *_next; /* sketchy! :P */
} List;

typedef struct GenericList {
    List _;
    void *_elm;
} GenericList;

void *list_alloc(size_t $$);
void list_free(void *, D1_t $$);

List *list_get_next(void * $$);
void list_set_next(void *, void * $$);

GenericList *gen_list_alloc($);
void gen_list_free(GenericList *, D1_t $$);
void gen_list_free_elm(GenericList *, D1_t $$);
void *gen_list_get_elm(GenericList * $$);
void gen_list_set_elm(GenericList *, void * $$) ;

#endif /* LIST_H_ */
