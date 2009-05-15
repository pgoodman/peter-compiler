/*
 * adt-typesafe-prod-dict.h
 *
 *  Created on: May 15, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef ADTTYPESAFEPRODDICT_H_
#define ADTTYPESAFEPRODDICT_H_

#include "std-include.h"
#include "p-parser-types.h"

ProdDictionary *prod_dict_alloc(uint32_t num_slots,
                     ProdDictionaryHashFunction key_hash_fnc,
                     ProdDictionaryCollisionFunction val_collision_fnc);

void prod_dict_free(ProdDictionary *H, PDelegate free_elm_fnc );

char prod_dict_set(ProdDictionary *H, PParserFunc key, void * val,
              PDelegate free_on_overwrite_fnc);

void prod_dict_unset(ProdDictionary *H, PParserFunc key, PDelegate free_fnc);

void *  prod_dict_get(ProdDictionary *H, PParserFunc key);

char prod_dict_is_set(ProdDictionary *H, PParserFunc key);

#endif /* ADTTYPESAFEPRODDICT_H_ */
