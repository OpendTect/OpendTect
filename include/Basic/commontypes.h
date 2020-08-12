#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/Salil
 Date:		Oct 2013
 RCS:		$Id$
________________________________________________________________________

*/

#ifndef commondefs_h
# include "commondefs.h"
#endif

class IOPar;
class Color;
class BinID;
class Coord;
class Coord3;
class TrcKey;
class IdxPair;
class MultiID;
class FixedString;
class BufferString;
namespace OD { class String; }
template<class T> class TypeSet;
template<class T> class Interval;
template<class T> class ObjectSet;
template<class T> class StepInterval;

typedef od_int32	Index_Type;
typedef od_int32	Index_Delta_Type;
typedef unsigned short	PortNr_Type;
typedef od_int32	PID_Type;

enum CaseSensitivity	{ CaseSensitive=0, CaseInsensitive=1 };
enum Pol2D3D		{ Only3D=-1, Both2DAnd3D=0, Only2D=1 };

namespace OD
{
enum DataRepType
    { AutoDataRep=0, SI8, UI8, SI16, UI16, SI32, UI32, F32, F64, SI64 };

template <class T> DataRepType GetDataRepType() { return AutoDataRep; }
#   define mDefDataRepTypeGetFn( ctyp, drtyp ) \
template <> inline DataRepType GetDataRepType<ctyp>() { return drtyp; }
mDefDataRepTypeGetFn( signed char, SI8 )
mDefDataRepTypeGetFn( unsigned char, UI8 )
mDefDataRepTypeGetFn( od_int16, SI16 )
mDefDataRepTypeGetFn( od_uint16, UI16 )
mDefDataRepTypeGetFn( od_int32, SI32 )
mDefDataRepTypeGetFn( od_uint32, UI32 )
mDefDataRepTypeGetFn( od_int64, SI64 )
mDefDataRepTypeGetFn( float, F32 )
mDefDataRepTypeGetFn( double, F64 )
mDefDataRepTypeGetFn( od_uint64, SI64 ) //!< pray

inline DataRepType GetDataRepType( bool isfp, bool issigned, int nbytes )
{
    if ( isfp )
	return nbytes>4 ? F64 : F32;
    if ( issigned )
	return nbytes<2 ? SI8 : (nbytes>4 ? SI64 : (nbytes==2 ? SI16 : SI32));
    else
	return nbytes<2 ? UI8 : (nbytes>4 ? SI64 : (nbytes==2 ? UI16 : UI32));
}
} // namespace OD


namespace Pos
{

class IdxPair;

typedef ::Index_Type	Index_Type;
typedef ::Index_Type	Index_Delta_Type;
typedef Index_Type	SurvID;
typedef Index_Type	LineID;
typedef Index_Type	TraceID;
typedef Index_Type	GeomID;

typedef double		Ordinate_Type;
typedef double		Distance_Type;

}

