#ifndef plftypes_h
#define plftypes_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Contents:	Platform dependent types
 RCS:		$Id: plftypes.h,v 1.1 2004-12-17 11:58:54 bert Exp $
________________________________________________________________________

*/

#include "plfdefs.h"

#if defined(__sun__) || defined(__sgi__)
# include <sys/types.h>
#else
# ifndef __win__
#  include <stdint.h>
# endif
#endif

/* 32 bits int is standard. Only use if you want to emphasise the 32-bitness */
#define int32 int
#define uint32 unsigned int

/* 64 bits is int64_t on UNIX. The definition is in various header files. */
#ifndef __win__
# define int64 int64_t
# define uint64 uint64_t
#else
# define int64 SET_THIS_AREND
# define uint64 SET_THIS_TOO_AREND
#endif


#endif
