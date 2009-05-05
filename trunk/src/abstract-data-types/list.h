/*
 * list.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef LIST_H_
#define LIST_H_

typedef struct List {
    struct List *next;
} List;

typedef struct GenericList {
    List;
    void *elm;
} GenericList;

void *list_alloc(int);
void list_free(List *, D1);

inline GenericList *gen_list_alloc(void);
void gen_list_free(GenericList *, D1);

#endif /* LIST_H_ */
