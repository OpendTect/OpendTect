#ifndef general_H
#define general_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of genc.h with C++ stuff.
 RCS:		$Id: general.h,v 1.3 2000-07-27 16:03:25 bert Exp $
________________________________________________________________________

-*/

#ifndef genc_H
#include <genc.h>
#endif

#ifdef __cpp__

#include <fixstring.h>
#include <bufstring.h>

typedef BufferString			UserIDString;
typedef FixedString<mMaxUnitIDLength>	UnitIDString;
typedef FixedString<PATH_LENGTH>	FileNameString;

template <class T>
inline void Swap( T& a, T& b ) { T tmp = a; a = b; b = tmp; }

#include <typeinfo>

template <class T>
inline const char* className( const T& t )
{	//!< Also works for gcc that returns the size first e.g. 4Clss
    const char* nm = typeid(t).name();
    while ( isdigit(*nm) ) nm++;
    return nm;
}


/* ifdef cpp */
#endif

#endif
