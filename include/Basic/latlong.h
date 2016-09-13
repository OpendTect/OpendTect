#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		2008
 Contents:	Geographics lat/long <-> Coord transform (an estimate)
________________________________________________________________________

-*/
 
#include "basicmod.h"
#include "coord.h"


/*!
\brief Geographical coordinates, decimal but with conv to deg, min, sec.
*/

mExpClass(Basic) LatLong
{
public:
			LatLong( double la=0, double lo=0 )
			    : lat_(la), lng_(lo)  {}

			LatLong( const Coord& c ) { *this = transform(c);}
			operator Coord() const	  { return transform(*this); }

    bool		isDefined() const {return !mIsUdf(lat_)&&!mIsUdf(lng_);}
    static LatLong	udf() { return LatLong(mUdf(double),mUdf(double)); }

    static Coord	transform(const LatLong&); //!< Uses SI()
    static LatLong	transform(const Coord&);   //!< Uses SI()

    const char*		toString() const;
    bool		fromString(const char*);

    void		getDMS(bool lat,int&,int&,float&) const;
    void		setDMS(bool lat,int,int,float);

    double		lat_;
    double		lng_;
};
