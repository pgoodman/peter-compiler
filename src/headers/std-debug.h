/*
 * debug.h
 *
 *  Created on: May 9, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef DEBUG_H_
#define DEBUG_H_

typedef struct StackTrace {
    struct StackTrace *next;
    char *file;
    unsigned int line;
} StackTrace;

#if defined(P_DEBUG) && P_DEBUG == 1

#define $MH StackTrace __T,*_ST;{__T.next=NULL;__T.file=__FILE__;__T.line=__LINE__;_ST=&__T;}
#define $H StackTrace __T;{__T.next=_ST;__T.file=__FILE__;__T.line=__LINE__;_ST=&__T;}
#define $ StackTrace *_ST
#define $$ , $
#define _$ _ST
#define _$$ , _ST
#define return_with {if(_ST!=NULL)_ST=_ST->next;}return

#define std_error(e) { \
    printf(e " in %s on line %d.", __FILE__, (unsigned int)__LINE__); \
    fflush(stdout); \
    exit(1);}

#else

#define $MH
#define $H
#define $ void
#define $$
#define _$
#define _$$
#define return_with return
#define std_error(e) exit(1);

#endif /* DEBUG */

#endif /* DEBUG_H_ */
