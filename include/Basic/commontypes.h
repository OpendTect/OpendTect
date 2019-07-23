#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/Salil
 Date:		Oct 2013
________________________________________________________________________

*/

#include "commondefs.h"
#include <utility>

class IOPar;
class Color;
class BinID;
class TrcKey;
class IdxPair;
class DBKey;
class FixedString;
class BufferString;
namespace OD { class String; }
template<class T> class TypeSet;
template<class T> class Interval;
template<class T> class ObjectSet;
template<class T> class StepInterval;

typedef od_int32	Index_Type;
typedef od_int64	DirtyCountType;

namespace Pos
{

    class IdxPair;

    typedef ::Index_Type	Index_Type;
    typedef Index_Type		TraceNr_Type;

    typedef float		Z_Type;

    typedef double		Ordinate_Type;
    typedef double		Distance_Type;

}

namespace Geom
{
    template <class T> class Point3D;
    template <class T> class Point2D;
}

typedef Geom::Point3D<Pos::Ordinate_Type>	Coord3d;
typedef Geom::Point3D<float>			Coord3f;
typedef Geom::Point2D<Pos::Ordinate_Type>	Coord2d;
typedef Geom::Point2D<float>			Coord2f;
typedef Coord2d					Coord;
typedef Coord3d					Coord3;

namespace Survey
{
    class Geometry;
    class Geometry2D;
    class Geometry3D;
    class GeometryManager;
}
typedef Survey::Geometry			SurvGeom;
typedef Survey::Geometry2D			SurvGeom2D;
typedef Survey::Geometry3D			SurvGeom3D;
typedef Survey::GeometryManager			SurvGM;


template <typename T> using Twins = std::pair<T,T>;
typedef Twins<int>	int_pair;
typedef Twins<od_int64>	int64_pair;
typedef Twins<float>	float_pair;
typedef Twins<double>	double_pair;


enum CaseSensitivity	{ CaseSensitive=0, CaseInsensitive=1 };

namespace OD
{

enum GeomSystem
    { SynthGeom=-2, VolBasedGeom=-1, LineBasedGeom=0 };

enum Pol2D3D
    { Only3D=0, Both2DAnd3D=1, Only2D=2 };

enum SurvLimitType
    { FullSurvey=0, UsrWork=1 };

enum SnapDir
    { SnapDownward=-1, SnapNearest=0, SnapUpward=1 };

enum SliceType
    { InlineSlice=0, CrosslineSlice=1, ZSlice=2 };

enum DataRepType
    { AutoDataRep=0, SI8, UI8, SI16, UI16, SI32, UI32, F32, F64, SI64 };

template <class T> DataRepType GetDataRepType()	{ return AutoDataRep; }
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


// Not just convenience, these keep the logic in one place:

inline bool is2D( OD::GeomSystem gs )	{ return gs==OD::LineBasedGeom; }
inline bool is3D( OD::GeomSystem gs )	{ return gs==OD::VolBasedGeom; }
inline void set2D( OD::GeomSystem& gs )	{ gs = OD::LineBasedGeom; }
inline void set3D( OD::GeomSystem& gs )	{ gs = OD::VolBasedGeom; }

inline bool has2D( OD::Pol2D3D pol )	{ return pol!=OD::Only3D; }
inline bool has3D( OD::Pol2D3D pol )	{ return pol!=OD::Only2D; }

inline bool isWork( OD::SurvLimitType lt ) { return lt==OD::UsrWork; }
inline bool isFull( OD::SurvLimitType lt ) { return lt==OD::FullSurvey; }

inline bool isFP( OD::DataRepType dr )
{ return dr == OD::F32 || dr == OD::F64; }
inline bool isSigned( OD::DataRepType dr )
{ return dr != OD::UI8 && dr != OD::UI16 && dr != OD::UI32; }
inline int nrBytes( OD::DataRepType dr )
{ return dr < OD::SI16 ? 1 : (dr < OD::SI32 ? 2 : (dr > OD::F32 ? 8 : 4)); }
