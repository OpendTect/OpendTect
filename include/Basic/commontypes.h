#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "commondefs.h"
#include "plftypes.h"

class IOPar;
class BinID;
class Coord;
class Coord3;
class TrcKey;
class IdxPair;
class MultiID;
class StringView;
class BufferString;
namespace OD { class String; }
template<class T> class TypeSet;
template<class T> class Interval;
template<class T> class ObjectSet;
template<class T> class StepInterval;

using Index_Type	= od_int32;
using DirtyCountType	= od_int64;
using PortNr_Type	= od_uint16;
using PID_Type		= od_int32;

enum CaseSensitivity	{ CaseSensitive=0, CaseInsensitive=1 };

namespace OD
{
enum GeomSystem
    { GeomSynth=-2, Geom3D=-1, Geom2D=0 };

enum Pol2D3D
    { Only3D=0, Both2DAnd3D=1, Only2D=2 };

enum SnapDir
    { SnapDownward=-1, SnapNearest=0, SnapUpward=1 };

enum DataRepType
    { AutoDataRep=0, SI8, UI8, SI16, UI16, SI32, UI32, F32, F64, SI64, UI64 };

template <class T> DataRepType GetDataRepType() { return AutoDataRep; }
#   define mDefDataRepTypeGetFn( ctyp, drtyp ) \
template <> inline DataRepType GetDataRepType<ctyp>() { return drtyp; }
mDefDataRepTypeGetFn( od_int8, SI8 )
mDefDataRepTypeGetFn( od_uint8, UI8 )
mDefDataRepTypeGetFn( od_int16, SI16 )
mDefDataRepTypeGetFn( od_uint16, UI16 )
mDefDataRepTypeGetFn( od_int32, SI32 )
mDefDataRepTypeGetFn( od_uint32, UI32 )
mDefDataRepTypeGetFn( od_int64, SI64 )
mDefDataRepTypeGetFn( float, F32 )
mDefDataRepTypeGetFn( double, F64 )
mDefDataRepTypeGetFn( od_uint64, UI64 )

inline DataRepType GetDataRepType( bool isfp, bool issigned, int nbytes )
{
    if ( isfp )
	return nbytes>4 ? F64 : F32;
    if ( issigned )
	return nbytes<2 ? SI8 : (nbytes>4 ? SI64 : (nbytes==2 ? SI16 : SI32));
    else
	return nbytes<2 ? UI8 : (nbytes>4 ? UI64 : (nbytes==2 ? UI16 : UI32));
}

} // namespace OD


namespace Pos
{
class IdxPair;
class GeomID;

using Index_Type = ::Index_Type;
using Index_Delta_Type = ::Index_Type;
using LineID = Index_Type;
using TraceID = Index_Type;

using Ordinate_Type = double;
using Distance_Type = double;

} // namespace Pos


// Not just convenience, these keep the logic in one place:

inline bool is2D( OD::GeomSystem gs )	{ return gs==OD::Geom2D; }
inline bool is3D( OD::GeomSystem gs )	{ return gs==OD::Geom3D; }
inline bool isSynthetic( OD::GeomSystem gs )   { return gs==OD::GeomSynth; }
inline void set2D( OD::GeomSystem& gs ) { gs = OD::Geom2D; }
inline void set3D( OD::GeomSystem& gs ) { gs = OD::Geom3D; }
inline void setSynthetic( OD::GeomSystem& gs ) { gs = OD::GeomSynth; }
