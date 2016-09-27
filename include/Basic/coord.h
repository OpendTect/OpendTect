#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		21-6-1996
 Contents:	Positions: Coordinates
________________________________________________________________________

-*/

#include "basicmod.h"
#include "geometry.h"
#include "undefval.h"

namespace Values {

/*!
\brief Undefined Coord.
*/

template<>
mClass(Basic) Undef<Coord>
{
public:
    static Coord	val()			{ return Coord::udf(); }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( Coord crd )	{ return !crd.isDefined(); }
    static void		setUdf( Coord& crd )	{ crd = Coord::udf(); }
};


/*!
\brief Undefined Coord3.
*/

template<>
mClass(Basic) Undef<Coord3>
{
public:
    static Coord3	val()			{ return Coord3::udf(); }
    static bool		hasUdf()		{ return true; }
    static bool		isUdf( Coord3 crd )	{ return !crd.isDefined(); }
    static void		setUdf( Coord3& crd )	{ crd = Coord3::udf(); }
};

} // namespace Values


