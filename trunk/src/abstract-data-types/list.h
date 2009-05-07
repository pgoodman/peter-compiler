/*
 * list.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef LIST_H_
#define LIST_H_

#include <stdlib.h>
#include <memory-management/static-mem.h>
#include "delegate.h"

typedef struct List {
    void *_next; // sketchy! :P
} List;

typedef struct GenericList {
    List _;
    void *_elm;
} GenericList;

void *list_alloc(int);
void list_free(void *, D1);

List *list_get_next(void *);
void list_set_next(void *, void *);

inline GenericList *gen_list_alloc(void);
void gen_list_free(GenericList *, D1);

void *gen_list_get_elm(GenericList *);
void gen_list_set_elm(GenericList *, void *) ;

#endif /* LIST_H_ */
