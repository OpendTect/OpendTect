#ifndef coordvalue_h
#define coordvalue_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/Salil
 Date:		Oct 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "coord.h"

/*!
\brief 3D coordinate and a value.
*/

mExpClass(Basic) Coord3Value
{
public:
    		Coord3Value( double x=0, double y=0, double z=0,
			     float v=mUdf(float) );
		Coord3Value( const Coord3& c, float v=mUdf(float) );

    bool	operator==( const Coord3Value& cv ) const
		{ return cv.coord == coord; }
    bool	operator!=( const Coord3Value& cv ) const
		{ return cv.coord != coord; }

    Coord3	coord;
    float	value;

};


#endif
