#ifndef latlong_h
#define latlong_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		9-4-1996
 Contents:	Geographics lat/long <-> Coord transform (an estimate)
 RCS:		$Id: latlong.h,v 1.2 2007-11-08 10:10:11 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "position.h"

/*!\brief Stores geographical coordinates (as defined by Google Earth). */

class LatLong
{
public:
    			LatLong( double la=0, double lo=0 )
			    : lat_(la), lng_(lo)  {}
    			LatLong( const Coord& c ) { *this = transform(c);}
			operator Coord() const	  { return transform(*this); }

    static Coord	transform(const LatLong&); //!< Uses SI()
    static LatLong	transform(const Coord&);   //!< Uses SI()

    bool		use(const char*);
    void		fill(char*) const;

    double		lat_;
    double		lng_;

};


/*!\brief Estimates to/from geographical coordinates.
  Needs both survey coordinate and lat/long for a point in the survey.

 */

class LatLong2Coord
{
public:

			LatLong2Coord();
			LatLong2Coord(const Coord&,const LatLong&);

    void		set(const LatLong&,const Coord&);
    void		setCoordsInFeet( bool yn )
			{ scalefac_ = yn ? 3.2808399 : 1; }

    LatLong		transform(const Coord&) const;
    Coord		transform(const LatLong&) const;

protected:

    Coord		refcoord_;
    LatLong		reflatlng_;
    double		latdist_;
    double		lngdist_;
    double		scalefac_;

};


#endif
