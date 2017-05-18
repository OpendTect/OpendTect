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

namespace Coords { class PositionSystem; }


/*!
\brief Geographical coordinates in Decimal Degrees
       but with conv to deg, min, sec.
*/

mExpClass(Basic) LatLong
{
public:
			LatLong( double la=0, double lo=0 )
			    : lat_(la), lng_(lo)  {}
			LatLong( const LatLong& ll )
			    : lat_(ll.lat_), lng_(ll.lng_)	{}

    bool		operator ==(const LatLong&) const;

			LatLong( const Coord& c ) { *this = transform(c); }
			operator Coord() const	  { return transform(*this); }
			//Using SI()

    bool		isDefined() const {return !mIsUdf(lat_)&&!mIsUdf(lng_);}
    static LatLong	udf() { return LatLong(mUdf(double),mUdf(double)); }

    static Coord	transform(const LatLong&,bool towgs84=false,
				  const Coords::PositionSystem* si=0);
    static LatLong	transform(const Coord&,bool towgs84=false,
				  const Coords::PositionSystem* si=0);

    const char*		toString() const;
    bool		fromString(const char*);

    void		getDMS(bool lat,int&,int&,float&) const;
    void		setDMS(bool lat,int,int,float);

    double		lat_;
    double		lng_;
};
