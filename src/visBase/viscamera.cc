/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscamera.cc,v 1.1 2002-02-22 09:19:32 kristofer Exp $";

#include "viscamera.h"

#include "Inventor/nodes/SoPerspectiveCamera.h"


visBase::Camera::Camera()
    : camera( new SoPerspectiveCamera )
{}


visBase::Camera::~Camera()
{}


SoNode* visBase::Camera::getData()
{ return camera; }


void visBase::Camera::setPosition(float x, float y, float z )
{
    camera->position.setValue( x, y, z );
}


float visBase::Camera::position( int dim ) const
{
    return camera->position.getValue()[dim];
}


void visBase::Camera::pointAt( float x,float y,float z )
{
    camera->pointAt( SbVec3f( x, y, z ));
}


void visBase::Camera::setAspectRatio(float n)
{
    camera->aspectRatio.setValue(n);
}


float visBase::Camera::aspectRatio() const
{
    return camera->aspectRatio.getValue();
}


void visBase::Camera::setHeightAngle(float n)
{
    camera->heightAngle.setValue(n);
}


float visBase::Camera::heightAngle() const
{
    return camera->heightAngle.getValue();
}


void visBase::Camera::setNearDistance(float n)
{
    camera->nearDistance.setValue(n);
}


float visBase::Camera::nearDistance() const
{
    return camera->nearDistance.getValue();
}


void visBase::Camera::setFarDistance(float n)
{
    camera->farDistance.setValue(n);
}


float visBase::Camera::farDistance() const
{
    return camera->farDistance.getValue();
}


void visBase::Camera::setFocalDistance(float n)
{
    camera->focalDistance.setValue(n);
}


float visBase::Camera::focalDistance() const
{
    return camera->focalDistance.getValue();
}
