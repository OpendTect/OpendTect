#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2009
________________________________________________________________________

-*/

#include "generalmod.h"
#include "basemap.h"
#include "draw.h"

/*!Object that draws markers on a basemap */

mExpClass(General) BaseMapMarkers : public BaseMapObject
{
public:
				BaseMapMarkers();
				~BaseMapMarkers();

    void			setMarkerStyle(int,const OD::MarkerStyle2D&);
    const OD::MarkerStyle2D*	getMarkerStyle(int) const
				{ return &markerstyle_;}

    TypeSet<Coord>&		positions() { return positions_; }
				/*!<Obtain lock if you are not main thread */

    void			updateGeometry();

    const char*			getType() const { return "Markers"; }

    int				nrShapes() const { return 1; }
    void			getPoints(int shapeidx,TypeSet<Coord>&) const;

protected:

    OD::MarkerStyle2D		markerstyle_;
    TypeSet<Coord>		positions_;

};

