#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/Salil
 Date:		Oct 2013
________________________________________________________________________

*/

#ifndef commondefs_h
# include "commondefs.h"
#endif
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
typedef od_int32	Index_Delta_Type;

enum CaseSensitivity	{ CaseSensitive=0, CaseInsensitive=1 };
enum Pol2D3D		{ Only3D=-1, Both2DAnd3D=0, Only2D=1 };


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

namespace Geom
{
    template <class T> class Point3D;
    template <class T> class Point2D;
}

namespace OD
{
    enum FPDataRepType
	{ AutoFPRep=0, SI8, UI8, SI16, UI16, SI32, UI32, F32, F64, SI64 };
}

/*
template <typename T> mClass(Basic) Twins : public std::pair<T,T>
{
public:

    inline	    Twins()
			: std::pair<T,T>((T)0,(T)0)		{}
    inline	    Twins( T f, T s )
			: std::pair<T,T>(f,s)			{}
    inline bool	    operator ==( const Twins<T>& oth ) const
		    { return first == oth.first && second == oth.second; }

};
*/

template <typename T> using Twins = std::pair<T,T>;
typedef Twins<int>	int_pair;
typedef Twins<od_int64>	int64_pair;
typedef Twins<float>	float_pair;
typedef Twins<double>	double_pair;

template <typename T> mClass(Basic) Triplets : public Twins<T>
{
public:

    inline	    Triplets()
			: Twins<T>((T)0,(T)0), third((T)0)	{}
    inline	    Triplets( T f, T s, T t )
			: Twins<T>(f,s), third(t)		{}
    inline bool	    operator ==( const Triplets<T>& oth ) const
		    { return Twins<T>::operator ==(oth) && third == oth.third; }

    T		    third;
};

typedef Geom::Point3D<Pos::Ordinate_Type> Coord3d;
typedef Geom::Point3D<float> Coord3f;
typedef Geom::Point2D<Pos::Ordinate_Type> Coord2d;
typedef Geom::Point2D<float> Coord2f;

typedef Coord2d Coord;
typedef Coord3d Coord3;
