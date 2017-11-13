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


/*!\brief draws markers on a basemap */

mExpClass(General) BaseMapMarkers : public BaseMapObject
{
public:
				BaseMapMarkers();
				~BaseMapMarkers();

    void			setMarkerStyle(int,const MarkerStyle&);
    const MarkerStyle*		markerStyle(int) const
				{ return &markerstyle_;}

    TypeSet<Coord>&		positions() { return positions_; }
				/*!<Obtain lock if you are not main thread */

    void			updateGeometry();

    virtual int			nrShapes() const	{ return 1; }
    void			getPoints(int,TypeSet<Coord>&) const;

protected:

    OD::MarkerStyle2D		markerstyle_;
    TypeSet<Coord>		positions_;

};
