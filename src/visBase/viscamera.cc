/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscamera.cc,v 1.14 2004-01-05 09:43:23 kristofer Exp $";

#include "viscamera.h"
#include "iopar.h"

mCreateFactoryEntry( visBase::Camera );

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


SoNode* visBase::Camera::getInventorNode()
{ return camera; }


void visBase::Camera::setPosition(const Coord3& pos)
{
    camera->position.setValue( pos.x, pos.y, pos.z );
}


Coord3 visBase::Camera::position() const
{
    SbVec3f pos = camera->position.getValue();
    Coord3 res;
    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];
    return res;
}


void visBase::Camera::setOrientation( const Coord3& dir, float angle )
{
    camera->orientation.setValue( dir.x, dir.y, dir.z, angle );
}


void visBase::Camera::getOrientation( Coord3& dir, float& angle )
{
    SbVec3f axis;
    camera->orientation.getValue( axis, angle);
    dir.x = axis[0];
    dir.y = axis[1];
    dir.z = axis[2];
}


void visBase::Camera::pointAt(const Coord3& pos)
{
    camera->pointAt( SbVec3f( pos.x, pos.y, pos.z ));
}


void visBase::Camera::pointAt(const Coord3& pos,
			      const Coord3& upvector)
{
    camera->pointAt( SbVec3f( pos.x, pos.y, pos.z ),
	    	     SbVec3f( upvector.x, upvector.y, upvector.z ));
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


void visBase::Camera::setStereoAdjustment(float n)
{
    camera->setStereoAdjustment( n );
}

float visBase::Camera::getStereoAdjustment() const
{
    return camera->getStereoAdjustment();
}

void visBase::Camera::setBalanceAdjustment(float n)
{
    camera->setBalanceAdjustment( n );
}

float visBase::Camera::getBalanceAdjustment() const
{
    return camera->getBalanceAdjustment();
}

int visBase::Camera::usePar( const IOPar& iopar )
{
    int res = DataObject::usePar( iopar );
    if ( res != 1 ) return res;

    Coord3 pos;
    if ( iopar.get( posstr, pos.x, pos.y, pos.z ) )
	setPosition( pos );

    double angle;
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

    return 1;
}

Coord3 visBase::Camera::centerFrustrum()
{
   
    float distancetopoint = ((( farDistance() - nearDistance() ) / 2) +
                                 nearDistance() );
    Coord3 orientation;
    float angle;
    getOrientation(orientation, angle);
    float vectorlength = ( sqrt (( orientation.x * orientation.x ) +
                                 ( orientation.y * orientation.y ) +
                                 ( orientation.z * orientation.z )));
     
    Coord3 currentposition = position();
    Coord3 position (((( distancetopoint / vectorlength ) * orientation.x ) +
	                 currentposition.x ),
                     ((( distancetopoint / vectorlength ) * orientation.y ) +
	                 currentposition.y ),
                     ((( distancetopoint / vectorlength ) * orientation.z ) +
                         currentposition.z ));
    return position;

}

float visBase::Camera::frustrumRadius()
{

    float distancetopoint = ((( farDistance() - nearDistance() ) / 2) +
	                        nearDistance() );
    float widthangle = ( heightAngle() * aspectRatio() );
    float height = ( farDistance()  *
                   ( tan(( heightAngle() * 180) / M_PI) / 2 ));
    float width  = ( farDistance()  *
                   ( tan(( widthangle * 180) / M_PI ) / 2 ));
    float distance = farDistance() - distancetopoint;
    float radius = ( sqrt (( distance * distance ) +
                           ( width * width ) +
                           ( height * height )));
    return radius;		
}

void visBase::Camera::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( iopar, saveids );
    Coord3 pos = position();
    iopar.set( posstr, pos.x, pos.y, pos.z );
    
    SbVec3f axis;
    float angle;
    camera->orientation.getValue( axis, angle );
    iopar.set( orientationstr, axis[0], axis[1], axis[2], angle );

    iopar.set( aspectratiostr, aspectRatio() );
    iopar.set( heightanglestr, heightAngle() );
    iopar.set( neardistancestr, (int)(nearDistance()+.5) );
    iopar.set( fardistancestr, (int)(farDistance()+.5) );
    iopar.set( focaldistancestr, focalDistance() );
}

