#ifndef basemapimpl_h
#define basemapimpl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2009
 RCS:		$Id: basemapimpl.h,v 1.2 2012-08-03 13:00:21 cvskris Exp $
________________________________________________________________________

-*/

#include "generalmod.h"
#include "basemap.h"

#include "draw.h"
#include "thread.h"

/*!Object that draws markers on a basemap */

mClass(General) BaseMapMarkers : public BaseMapObject
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

