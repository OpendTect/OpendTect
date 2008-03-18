#ifndef latlong_h
#define latlong_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		9-4-1996
 Contents:	Geographics lat/long <-> Coord transform (an estimate)
 RCS:		$Id: latlong.h,v 1.6 2008-03-18 15:38:44 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "position.h"


/*!\brief geographical coordinates as defined by Google Earth/Maps. */

class LatLong
{
public:
    			LatLong( double la=0, double lo=0 )
			    : lat_(la), lng_(lo)  {}
    			LatLong( const Coord& c ) { *this = transform(c);}
			operator Coord() const	  { return transform(*this); }

    static Coord	transform(const LatLong&); //!< Uses SI()
    static LatLong	transform(const Coord&);   //!< Uses SI()

    void		fill(char*) const;
    bool		use(const char*);

    double		lat_;
    double		lng_;

};


/*!\brief Estimates to/from LatLong coordinates.

  Needs both survey coordinate and lat/long for an anchor point in the survey.

 */

class LatLong2Coord
{
public:

			LatLong2Coord();
			LatLong2Coord(const Coord&,const LatLong&);
    bool		isOK() const	{ return !mIsUdf(lngdist_); }

    void		set(const LatLong&,const Coord&);
    void		setCoordsInFeet( bool yn )
			{ scalefac_ = yn ? 0.3048 : 1; }

    LatLong		transform(const Coord&) const;
    Coord		transform(const LatLong&) const;

    void		fill(char*) const;
    bool		use(const char*);

    Coord		refCoord() const	{ return refcoord_; }
    LatLong		refLatLong() const	{ return reflatlng_; }
    bool		coordsInFeet() const	{ return scalefac_ < 0.7; }

protected:

    Coord		refcoord_;
    LatLong		reflatlng_;
    double		scalefac_;

    double		latdist_;
    double		lngdist_;
};


#endif
