/*
 * type.h
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef TYPE_H_
#define TYPE_H_

#include <stdheader.h>
#include <abstract-data-types/hash-table.h>

/* types for types */
typedef P_Kind;
typedef P_Type;
typedef P_SymbolType;
typedef P_FunctionType;
typedef P_MethodType;
typedef P_ObjectType;
typedef P_UniversalType;

/* types for things that use/hold types */
typedef P_TypeComparison;
typedef P_TypeComparator;
typedef P_TypeRelation;

/* other than universally quantified type, the type kinds are orthogonal and
 * thus define distinct type lattices */
typedef enum {
    P_KIND_SYMBOL,
    P_KIND_FUNCTION,
    P_KIND_METHOD,
    P_KIND_OBJECT,
    P_KIND_UNIVERSAL /* universally quantified type, i.e. a generic type */
} P_Kind;

/* type */
typedef struct P_Type {
    P_Kind _;

    /* type name, case sensitive */
    char *name;

    /* defines both a type comparison operator to use and its
     * expected return value for a valid type */
    P_TypeRelation relation;

    /* types that build up this type */
    P_Type** inner_types;
    char num_inner_types;
} P_Type;

typedef P_Type P_SymbolType;
typedef P_Type P_UniversalType;

typedef struct P_ObjectType {

    /* the number of immediate sub and super types */
    uint16_t num_imm_supertypes,
             num_imm_subtypes;

    /* creates the type lattice structure for object types */
    struct P_Type **imm_super_types,
                  **imm_sub_types;
} P_ObjectType;

typedef struct P_FunctionType {
    P_Type _,
           *arg_type,
           *return_type;

    enum {0, 1} num_args;
} P_FunctionType;

typedef struct P_MethodType {
    P_FunctionType;
    P_Type *object_type;

} P_MethodType;

typedef enum {
    P_STRICT_SUBTYPE = 1,
    P_EQUIVALENT = 2,
    P_STRICT_SUPERTYPE = 4,
    P_NON_COMPARABLE = 8
} P_TypeComparison;

/* type of function that can compare two arbitrary types */
typedef P_TypeComparison (*P_TypeComparator)(P_Type *, P_Type *);

/* a relation function and the value of what makes a valid relation */
typedef struct P_TypeRelation {
    P_TypeComparator comparator;
    unsigned char valid_relation; /* subtype or supertype */
} P_TypeRelation;

#endif /* TYPE_H_ */
