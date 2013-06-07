/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "viscamera.h"
#include "iopar.h"
#include "keystrs.h"

#include "UTMCamera.h"
#include "Inventor/nodes/SoGroup.h"

mCreateFactoryEntry( visBase::Camera );

namespace visBase
{

const char* Camera::sKeyPosition() 	{ return sKey::Position; }
const char* Camera::sKeyOrientation() 	{ return "Orientation"; }
const char* Camera::sKeyAspectRatio() 	{ return "Aspect ratio"; }
const char* Camera::sKeyNearDistance() 	{ return "Near Distance"; }
const char* Camera::sKeyFarDistance() 	{ return "Far Distance"; }
const char* Camera::sKeyFocalDistance()	{ return "Focal Distance"; }


Camera::Camera()
    : group( new SoGroup )
{
    group->ref();
    group->addChild( new UTMCamera );
}


Camera::~Camera()
{
    group->unref();
}


SoNode* Camera::gtInvntrNode()
{ return group; }


void Camera::setPosition(const Coord3& pos)
{
    SoCamera* camera = getCamera();
    camera->position.setValue( pos.x, pos.y, pos.z );
}


Coord3 Camera::position() const
{
    const SoCamera* camera = getCamera();
    SbVec3f pos = camera->position.getValue();
    Coord3 res;
    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];
    return res;
}


void Camera::setOrientation( const Coord3& dir, float angle )
{
    SoCamera* camera = getCamera();
    camera->orientation.setValue( dir.x, dir.y, dir.z, angle );
}


void Camera::getOrientation( Coord3& dir, float& angle )
{
    const SoCamera* camera = getCamera();
    SbVec3f axis;
    camera->orientation.getValue( axis, angle );
    dir.x = axis[0];
    dir.y = axis[1];
    dir.z = axis[2];
}


void Camera::pointAt( const Coord3& pos )
{
    SoCamera* camera = getCamera();
    camera->pointAt( SbVec3f(pos.x,pos.y,pos.z) );
}


void Camera::pointAt(const Coord3& pos, const Coord3& upvector )
{
    SoCamera* camera = getCamera();
    camera->pointAt( SbVec3f( pos.x, pos.y, pos.z ),
	    	     SbVec3f( upvector.x, upvector.y, upvector.z ));
}


void Camera::setAspectRatio( float n )
{
    SoCamera* camera = getCamera();
    camera->aspectRatio.setValue(n);
}


float Camera::aspectRatio() const
{
    const SoCamera* camera = getCamera();
    return camera->aspectRatio.getValue();
}


void Camera::setNearDistance( float n )
{
    SoCamera* camera = getCamera();
    camera->nearDistance.setValue(n);
}


float Camera::nearDistance() const
{
    const SoCamera* camera = getCamera();
    return camera->nearDistance.getValue();
}


void Camera::setFarDistance( float n )
{
    SoCamera* camera = getCamera();
    camera->farDistance.setValue(n);
}


float Camera::farDistance() const
{
    const SoCamera* camera = getCamera();
    return camera->farDistance.getValue();
}


void Camera::setFocalDistance(float n)
{
    SoCamera* camera = getCamera();
    camera->focalDistance.setValue(n);
}


float Camera::focalDistance() const
{
    const SoCamera* camera = getCamera();
    return camera->focalDistance.getValue();
}


void Camera::setStereoAdjustment(float n)
{
    SoCamera* camera = getCamera();
    camera->setStereoAdjustment( n );
}

float Camera::getStereoAdjustment() const
{
    const SoCamera* camera = getCamera();
    return camera->getStereoAdjustment();
}

void Camera::setBalanceAdjustment(float n)
{
    SoCamera* camera = getCamera();
    camera->setBalanceAdjustment( n );
}

float Camera::getBalanceAdjustment() const
{
    const SoCamera* camera = getCamera();
    return camera->getBalanceAdjustment();
}

int Camera::usePar( const IOPar& iopar )
{
    int res = DataObject::usePar( iopar );
    if ( res != 1 ) return res;

    Coord3 pos;
    if ( iopar.get( sKeyPosition(), pos ) )
	setPosition( pos );

    SoCamera* camera = getCamera();
    double angle;
    if ( iopar.get( sKeyOrientation(), pos.x, pos.y, pos.z, angle ) )
	camera->orientation.setValue( SbVec3f(pos.x,pos.y,pos.z), angle );

    float val;
    if ( iopar.get(sKeyAspectRatio(),val) )
	setAspectRatio( val );

    if ( iopar.get(sKeyNearDistance(),val) )
	setNearDistance( val );

    if ( iopar.get(sKeyFarDistance(),val) )
	setFarDistance( val );

    if ( iopar.get(sKeyFocalDistance(),val) )
	setFocalDistance( val );

    return 1;
}


void Camera::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( iopar, saveids );

    iopar.set( sKeyPosition(), position() );
    
    SbVec3f axis;
    float angle;
    const SoCamera* camera = getCamera();
    camera->orientation.getValue( axis, angle );
    iopar.set( sKeyOrientation(), axis[0], axis[1], axis[2], angle );

    iopar.set( sKeyAspectRatio(), aspectRatio() );
    iopar.set( sKeyNearDistance(), (int)(nearDistance()+.5) );
    iopar.set( sKeyFarDistance(), (int)(farDistance()+.5) );
    iopar.set( sKeyFocalDistance(), focalDistance() );
}


void Camera::fillPar( IOPar& iopar, const SoCamera* socamera ) const
{
    TypeSet<int> dummy;
    DataObject::fillPar( iopar, dummy );

    SbVec3f pos = socamera->position.getValue();
    iopar.set( sKeyPosition(), Coord3(pos[0],pos[1],pos[2]) );
    
    SbVec3f axis;
    float angle;
    socamera->orientation.getValue( axis, angle );
    iopar.set( sKeyOrientation(), axis[0], axis[1], axis[2], angle );

    iopar.set( sKeyAspectRatio(), socamera->aspectRatio.getValue() );
    iopar.set( sKeyNearDistance(), mNINT32(socamera->nearDistance.getValue()) );
    iopar.set( sKeyFarDistance(), mNINT32(socamera->farDistance.getValue()) );
    iopar.set( sKeyFocalDistance(), socamera->focalDistance.getValue() );
}


SoCamera* Camera::getCamera()
{
    return dynamic_cast<SoCamera*>(group->getChild(0));
}


const SoCamera* Camera::getCamera() const
{ return const_cast<Camera*>(this)->getCamera(); }

}; // namespace visBase
