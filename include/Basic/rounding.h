#ifndef rounding_h
#define rounding_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2012
 RCS:		$Id: rounding.h,v 1.2 2012-07-24 19:17:02 cvskris Exp $
________________________________________________________________________

Macros and inline functions for rounding floats and doubles.

Most applicaiton developers will only need to know mNINT32, mNINT64 or
mRounded which are found in commondefs.h.

-*/

#include "plfdefs.h"
#include "plftypes.h"

#ifdef __cpp__

template <class RT> inline
RT roundOff( double x ) { return (RT) ((x)>0 ? (x)+.5 : (x)-.5); }

template <class RT> inline
RT roundOff( float x ) { return (RT) ((x)>0 ? (x)+.5f : (x)-.5f); }

#endif

#endif
