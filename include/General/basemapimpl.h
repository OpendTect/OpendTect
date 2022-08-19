#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
