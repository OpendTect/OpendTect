#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		21-6-1996
 Contents:	Positions: Coordinates
________________________________________________________________________

-*/

#include "basicmod.h"
#include "geometry.h"
#include "undefval.h"

namespace Values {

/*!\brief Undefined Coord2d. */

template<>
mClass(Basic) Undef<Coord2d>
{
public:
    static Coord2d	val()			{ return Coord2d::udf(); }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( Coord2d crd )	{ return !crd.isDefined(); }
    static void		setUdf( Coord2d& crd )	{ crd = Coord2d::udf(); }
};


/*!\brief Undefined Coord3d. */

template<>
mClass(Basic) Undef<Coord3d>
{
public:
    static Coord3d	val()			{ return Coord3d::udf(); }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( Coord3d crd )	{ return !crd.isDefined(); }
    static void		setUdf( Coord3d& crd )	{ crd = Coord3d::udf(); }
};
    
    
    
template<>
mClass(Basic) Undef<Coord2f>
{
public:
    static Coord2f	val()			{ return Coord2f::udf(); }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( Coord2f crd )	{ return !crd.isDefined(); }
    static void		setUdf( Coord2f& crd )	{ crd = Coord2f::udf(); }
};


/*!
 \brief Undefined Coord3d.
 */

template<>
mClass(Basic) Undef<Coord3f>
{
public:
    static Coord3f	val()			{ return Coord3f::udf(); }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( Coord3f crd )	{ return !crd.isDefined(); }
    static void		setUdf( Coord3f& crd )	{ crd = Coord3f::udf(); }
};

} // namespace Values
