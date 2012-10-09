#ifndef basemapimpl_h
#define basemapimpl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basemap.h"

#include "draw.h"
#include "thread.h"

/*!Object that draws markers on a basemap */

mClass BaseMapMarkers : public BaseMapObject
{
public:
				BaseMapMarkers();
				~BaseMapMarkers();

    void			setMarkerStyle(const MarkerStyle2D&);
    const MarkerStyle2D*	getMarkerStyle(int) const
    				{ return &markerstyle_;}

    TypeSet<Coord>&		positions() { return positions_; }
    				/*!<Obtain lock if you are not main thread */

    void			updateGeometry();

    const char*			getType() const { return "Markers"; }

    int				nrShapes() const { return 1; }
    void			getPoints(int shapeidx,TypeSet<Coord>&) const; 
    char			connectPoints(int shapeidx) const{ return false; }

protected:
    MarkerStyle2D		markerstyle_;
    TypeSet<Coord>		positions_;
};


#endif
