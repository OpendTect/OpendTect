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

mExpClass(General) BasemapMarkers : public BasemapObject
{
public:
				BasemapMarkers();
				~BasemapMarkers();

    void			setMarkerStyle(int,
					       const MarkerStyle2D&) override;
    const MarkerStyle2D*	getMarkerStyle(int) const override
				{ return &markerstyle_;}

    TypeSet<Coord>&		positions() { return positions_; }
				/*!<Obtain lock if you are not main thread */

    void			updateGeometry() override;

    const char*			getType() const override { return "Markers"; }

    int				nrShapes() const override { return 1; }
    void			getPoints(int shapeidx,
					  TypeSet<Coord>&) const override;

protected:

    MarkerStyle2D		markerstyle_;
    TypeSet<Coord>		positions_;

};

