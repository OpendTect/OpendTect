#ifndef general_H
#define general_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of genc.h with C++ stuff.
 RCS:		$Id: general.h,v 1.2 2000-04-12 16:08:58 bert Exp $
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


/* ifdef cpp */
#endif

#endif
