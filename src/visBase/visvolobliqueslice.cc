/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id$";


#include "visvolobliqueslice.h"

#include "visdataman.h"
#include "vistexture3.h"
#include "visselman.h"
#include "iopar.h"

#include "VolumeViz/nodes/SoObliqueSlice.h"

mCreateFactoryEntry( visBase::ObliqueSlice );

namespace visBase
{

ObliqueSlice::ObliqueSlice()
    : VisualObjectImpl( false )
    , slice_( new SoObliqueSlice )
{
    addChild( slice_ );
}


ObliqueSlice::~ObliqueSlice()
{}


Coord3 ObliqueSlice::getPosOnPlane() const
{ return getNormal()*slice_->plane.getValue().getDistanceFromOrigin(); } 


Coord3 ObliqueSlice::getNormal() const
{
    const SbVec3f normal = slice_->plane.getValue().getNormal();
    return Coord3( normal[0], normal[1], normal[2] ).normalize();
}


void ObliqueSlice::set( const Coord3& normal, const Coord3& pos )
{
    SbPlane plane( SbVec3f((float) normal.x,(float) normal.y,(float) normal.z),
	    	   SbVec3f((float) pos.x,(float) pos.y,(float) pos.z) );
    slice_->plane.setValue( plane );
}

} // namespace visBase
