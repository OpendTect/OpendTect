#ifndef general_H
#define general_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of genc.h with C++ stuff.
 RCS:		$Id: general.h,v 1.1.1.2 1999-09-16 09:19:02 arend Exp $
________________________________________________________________________

@$*/

#ifndef genc_H
#include <genc.h>
#endif

#ifdef __cpp__

#include <fixstring.h>

typedef FixedString<mMaxUserIDLength+1>	UserIDString;
typedef FixedString<mMaxUnitIDLength+1>	UnitIDString;
typedef FixedString<PATH_LENGTH+1>	FileNameString;

template <class T>
inline void Swap( T& a, T& b ) { T tmp = a; a = b; b = tmp; }


/* ifdef cpp */
#endif

#endif
