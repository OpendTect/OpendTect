#ifndef general_H
#define general_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of genc.h with C++ stuff.
 RCS:		$Id: general.h,v 1.8 2004-10-15 09:43:17 bert Exp $
________________________________________________________________________

-*/

#ifndef genc_H
#include <genc.h>
#endif

#ifdef __cpp__

#include "fixstring.h"
#include "bufstring.h"

typedef BufferString			UserIDString;
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


		//! Defines policy for selection of things
enum Pol2D	{ No2D=-1, Both2DAnd3D=0, Only2D=1 };


#endif /* ifdef cpp */

#endif
