/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : 23-06-2003
-*/

static const char* rcsID = "$Id: visdetail.cc,v 1.6 2007-10-10 03:59:24 cvsnanne Exp $";

#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoPointDetail.h>

#include "visdetail.h"
#include "viscoord.h"


namespace visBase
{

DetailType Detail::getDetailType()
{
    return detailtype;
}


int FaceDetail::getClosestIdx( const Coordinates* coordinates,
			       const Coord3& pickedpoint ) const
{
    const int nrpoints = facedetail->getNumPoints();
    if ( !nrpoints ) return -1;
	    
    const SoPointDetail* pointdetail = facedetail->getPoint(0);

    float mindist;
    int closestidx;
    for ( int idx=0; idx<nrpoints; idx++ )
    {
        const int coordidx = pointdetail[idx].getCoordinateIndex();
        float dist = pickedpoint.distTo( coordinates->getPos(coordidx,true) );
        if ( dist < mindist || !idx )
        {
           mindist = dist;
           closestidx = coordidx;
        }
    }

    return closestidx;
}

} // namespace visBase
