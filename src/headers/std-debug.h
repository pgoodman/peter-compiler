/*
 * debug.h
 *
 *  Created on: May 9, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef DEBUG_H_
#define DEBUG_H_

typedef struct PStackTrace {
    struct PStackTrace *next;
    char *file;
    unsigned int line;
} PStackTrace;

#if defined(P_DEBUG) && P_DEBUG == 1

#if defined(P_DEBUG_PRINT_TRACE) && P_DEBUG_PRINT_TRACE == 1
#define $push_trace printf("%*s%s:%d\n", (++__st_depth)<<1, "", __FILE__, __LINE__);
#define $pop_trace printf("%*s%s:%d\n", (--__st_depth)<<1, "", __FILE__,__LINE__);
#else
#define $push_trace
#define $pop_trace
#endif

#define $MH PStackTrace __T,*_ST;\
    unsigned int __st_line=__LINE__;{\
        __T.next=NULL;\
        __T.file=__FILE__;\
        __T.line=__st_line;\
        _ST=&__T;}

#define $H {\
    PStackTrace __T; \
    __T.next=_ST;\
    __T.file=__FILE__; \
    __T.line=__LINE__;\
    _ST=&__T;\
    $push_trace}

/* argument for function definition */
#define $ PStackTrace *_ST, unsigned int __st_line
#define $$ $,

/* arguments for macro definitions */
#define $M _ST,__st_line
#define $$M , $M

/* parameter passing */
#define $A _ST, __st_line
#define $$A $A,

/**
 * !!! Be careful with this macro!!
 */
#define return_with {\
    $pop_trace \
    if(_ST!=NULL){\
        _ST=_ST->next;\
    }}return

#define std_error(e) { \
    printf(e " in %s on line %d.", __FILE__, (unsigned int)__LINE__); \
    fflush(stdout); \
    exit(1);}

#else

#define $MH
#define $H
#define $ void
#define $$
#define $A
#define $$A
#define return_with return
#define std_error(e) exit(1);

#endif /* DEBUG */

#endif /* DEBUG_H_ */
