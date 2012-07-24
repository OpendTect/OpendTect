#ifndef rounding_h
#define rounding_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2012
 RCS:		$Id: rounding.h,v 1.1 2012-07-24 18:53:30 cvskris Exp $
________________________________________________________________________

Macros and inline functions for rounding floats and doubles.

Most applicaiton developers will only need to know mNINT32, mNINT64 or
mRounded which are found in commondefs.h.

-*/

#include "plfdefs.h"
#include "plftypes.h"

#ifdef __cpp__

#define mRoundedImpl( typ ) \
inline typ round_##typ( double x ) \
{ return (typ) ((x)>0 ? (x)+.5 : (x)-.5); } \
 \
inline typ round##typ( float x ) \
{ return (typ) ((x)>0 ? (x)+.5f : (x)-.5f); } 

mRoundedImpl( od_int64 )
mRoundedImpl( od_uint64 )
mRoundedImpl( od_int32 )
mRoundedImpl( od_uint32 )
mRoundedImpl( short )
mRoundedImpl( unsigned )
mRoundedImpl( od_uint16 )

#endif

#endif
