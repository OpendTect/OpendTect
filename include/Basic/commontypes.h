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
typedef unsigned short	PortNr_Type;
typedef od_int32	PID_Type;

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


template <class T>
class Twins
{
public:

    inline Twins()
		: pair_(0,0)		{}
    inline Twins( T f, T s )
		: pair_(f,s)		{}
		mImplSimpleEqOpers1Memb(Twins<T>,pair_)

    inline std::pair<T,T> pair() const	{ return pair_; }
    inline std::pair<T,T>& pair()	{ return pair_; }

    inline T&	first()			{ return pair_.first; }
    inline T	first() const		{ return pair_.first; }
    inline T&	second()		{ return pair_.second; }
    inline T	second() const		{ return pair_.second; }

    int		size() const		{ return 2; }
    inline T&	operator[]( int idx )
		{ return idx<1 ? first() : second();}
    inline T	operator[]( int idx ) const
		{ return idx<1 ? first() : second();}

protected:

    std::pair<T,T>	pair_;

};

typedef Twins<int>		int_twins;
typedef Twins<od_int64>		int64_twins;
typedef Twins<float>		float_twins;
typedef Twins<double>		double_twins;


template <class T>
class Triplets : public Twins<T>
{
public:

    inline Triplets()
		: third_(0)			{}
    inline Triplets( T f, T s, T t )
		: Twins<T>(f,s), third_(t)	{}
		mImplSimpleEqOpers2Memb(Triplets<T>,pair_,third_)

    inline T&	third()		{ return third_; }
    inline T	third() const	{ return third_; }

    int		size() const	{ return 3; }
    inline T&	operator[]( int idx )
		{ return idx<1 ? this->first() : (idx>1 ? this->third()
							: this->second());}
    inline T	operator[]( int idx ) const
		{ return idx<1 ? this->first() : (idx>1 ? this->third()
							: this->second());}

protected:

    T		third_;

};

typedef Triplets<int>		int_triplet;
typedef Triplets<od_int64>	int64_triplet;
typedef Triplets<float>		float_triplet;
typedef Triplets<double>	double_triplet;


class IJPos : public int_twins
{
public:

		    IJPos() : int_twins(-1,-1)		{}
    explicit	    IJPos( Index_Type ii, Index_Type jj=-1 )
			: int_twins(ii,jj)		{}
		    mImplSimpleEqOpers2Memb(IJPos,i(),j())

    inline bool	    operator>( const IJPos& oth ) const
		    { return i()>oth.i() || (i()==oth.i() && j()>oth.j()); }
    inline bool	    operator<( const IJPos& oth ) const
		    { return i()<oth.i() || (i()==oth.i() && j()<oth.j()); }

    void	    reset()			{ i() = j() = -1; }
    inline bool	    isValid() const		{ return i() > -1 && j() > -1; }

    Index_Type&	    i()		{ return first(); }
    Index_Type	    i() const	{ return first(); }
    Index_Type&	    j()		{ return second(); }
    Index_Type	    j() const	{ return second(); }

};


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
