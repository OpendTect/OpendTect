/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscamera.cc,v 1.2 2002-03-18 14:21:28 kristofer Exp $";

#include "viscamera.h"
#include "geompos.h"
#include "iopar.h"

const char* visBase::Camera::posstr = "Position";
const char* visBase::Camera::orientationstr = "Orientation";
const char* visBase::Camera::aspectratiostr = "Aspect ratio";
const char* visBase::Camera::heightanglestr = "Height Angle";
const char* visBase::Camera::neardistancestr = "Near Distance";
const char* visBase::Camera::fardistancestr = "Far Distance";
const char* visBase::Camera::focaldistancestr = "Focal Distance";


#include "Inventor/nodes/SoPerspectiveCamera.h"


visBase::Camera::Camera()
    : camera( new SoPerspectiveCamera )
{}


visBase::Camera::~Camera()
{}


SoNode* visBase::Camera::getData()
{ return camera; }


void visBase::Camera::setPosition(const Geometry::Pos& pos)
{
    camera->position.setValue( pos.x, pos.y, pos.z );
}


Geometry::Pos visBase::Camera::position() const
{
    SbVec3f pos = camera->position.getValue();
    Geometry::Pos res;
    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];
    return res;
}


void visBase::Camera::pointAt(const Geometry::Pos& pos)
{
    camera->pointAt( SbVec3f( pos.x, pos.y, pos.z ));
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


bool visBase::Camera::usePar( const IOPar& iopar )
{
    if ( !SceneObject::usePar( iopar ) ) return false;

    Geometry::Pos pos;
    if ( iopar.get( posstr, pos.x, pos.y, pos.z ) )
	setPosition( pos );

    float angle;
    if ( iopar.get( orientationstr, pos.x, pos.y, pos.z, angle ) )
	camera->orientation.setValue( SbVec3f( pos.x, pos.y, pos.z ), angle );

    float val;
    if ( iopar.get( aspectratiostr, val ))
	setAspectRatio( val );

    if ( iopar.get( heightanglestr, val ))
	setHeightAngle( val );

    if ( iopar.get( neardistancestr, val ))
	setNearDistance( val );

    if ( iopar.get( fardistancestr, val ))
	setFarDistance( val );

    if ( iopar.get( focaldistancestr, val ))
	setFocalDistance( val );

    return true;
}


void visBase::Camera::fillPar( IOPar& iopar ) const
{
    SceneObject::fillPar( iopar );
    Geometry::Pos pos = position();
    iopar.set( posstr, pos.x, pos.y, pos.z );
    
    SbVec3f axis;
    float angle;
    camera->orientation.getValue( axis, angle );
    iopar.set( orientationstr, axis[0], axis[1], axis[2], angle );

    iopar.set( aspectratiostr, aspectRatio() );
    iopar.set( heightanglestr, heightAngle() );
    iopar.set( neardistancestr, nearDistance() );
    iopar.set( fardistancestr, farDistance() );
    iopar.set( focaldistancestr, focalDistance() );
}

