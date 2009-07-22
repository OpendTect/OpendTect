/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 23-06-2003
-*/

static const char* rcsID = "$Id: visdetail.cc,v 1.7 2009-07-22 16:01:44 cvsbert Exp $";

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
