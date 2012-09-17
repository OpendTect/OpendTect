#ifndef plftypes_h
#define plftypes_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Contents:	Platform dependent types
 RCS:		$Id: plftypes.h,v 1.7 2009/07/22 16:01:14 cvsbert Exp $
________________________________________________________________________

*/

#include "plfdefs.h"

#ifdef __sun__
# include <sys/types.h>
#else
# ifndef __msvc__
#  include <stdint.h>
# endif

#endif

/* 16 bits short is standard. Only use to emphasise the 16-bitness */
#define od_int16	short
#define od_uint16	unsigned short

/* 32 bits int is standard. Only use to emphasise the 32-bitness */
#define od_int32	int
#define od_uint32	unsigned int

/* 64 bits is int64_t. The definition is in various header files. */
#ifndef __msvc__
# define od_int64	int64_t
# define od_uint64	uint64_t
#else
# define od_int64 	__int64
# define od_uint64	unsigned __int64
#endif

#endif
