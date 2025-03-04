#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "coord.h"

namespace Coords { class CoordSystem; }


/*!
\brief Geographical coordinates in Decimal Degrees
       but with conv to deg, min, sec.
*/

mExpClass(Basic) LatLong
{
public:
			LatLong();			// isUdf()
			LatLong(double la,double lo);
			LatLong(const LatLong&);
			~LatLong();

    LatLong&		operator=(const LatLong&);
    bool		operator==(const LatLong&) const;
    bool		operator!=(const LatLong&) const;

    double		latitude() const		{ return lat_; }
    void		setLatitude( double lat )	{ lat_ = lat; }
    double		longitude() const		{ return lng_; }
    void		setLongitude( double lng )	{ lng_ = lng; }
    void		set( double lat, double lng )	{ lat_=lat; lng_=lng; }
    void		setUdf()			{ *this = udf(); }

    bool		isNull() const { return lat_==0 && lng_==0; }
    bool		isDefined() const {return !mIsUdf(lat_)&&!mIsUdf(lng_);}
    bool		isUdf() const { return mIsUdf(lat_) || mIsUdf(lng_); }
    static LatLong	udf() { return LatLong(mUdf(double),mUdf(double)); }

    void		setFromCoord(const Coord&);
			/*!<Coord should have x=Latitude, y=Longitude
			 * as stipulated by the ISO 6709 standard. */
    Coord		asCoord() const;
			/*!<Returned Coord has x=Latitude, y=Longitude
			 * as stipulated by the ISO 6709 standard. */

    static LatLong	fromCoord(const Coord&);
			/*!<Coord should have x=Latitude, y=Longitude
			 * as stipulated by the ISO 6709 standard. */
    static Coord	toCoord(const LatLong&);
			/*!<Returned Coord has x=Latitude, y=Longitude
			 * as stipulated by the ISO 6709 standard. */

    static Coord	transform(const LatLong&,bool towgs84=false,
				  const Coords::CoordSystem* si=0);
    static LatLong	transform(const Coord&,bool towgs84=false,
				  const Coords::CoordSystem* si=0);

    const char*		toString() const;
    bool		fromString(const char*);

    void		getDMS(bool lat,int&,int&,float&) const;
    void		setDMS(bool lat,int,int,float);

    bool		setFromString(const char*,bool lat);
    static bool		isDMSString(const BufferString&);

    double		lat_;
    double		lng_;

    mDeprecated		("Obsolete: use LatLong::transform instead")
    explicit		LatLong( const Coord& c ) { *this = transform(c); }
			//Using SI()
protected:
    bool		parseDMSString(const BufferString&,bool lat);
};


/*!
\brief Estimates to/from LatLong coordinates.

  Needs both survey coordinates and lat/long for an anchor point in the survey.
*/

mExpClass(Basic) LatLong2Coord
{
public:
			LatLong2Coord();
			LatLong2Coord(const Coord&,const LatLong&);
			~LatLong2Coord();

    bool		operator ==(const LatLong2Coord&) const;
    bool		operator !=(const LatLong2Coord&) const;

    bool		isOK() const	{ return !mIsUdf(lngdist_); }

    void		set(const LatLong&,const Coord&);

    LatLong		transform(const Coord&) const;
    Coord		transform(const LatLong&) const;

    const char*		toString() const;
    bool		fromString(const char*);

    Coord		refCoord() const	{ return refcoord_; }
    LatLong		refLatLong() const	{ return reflatlng_; }

protected:

    Coord		refcoord_;
    LatLong		reflatlng_;

    double		latdist_;
    double		lngdist_;
    double		scalefac_;
};
