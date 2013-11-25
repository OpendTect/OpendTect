/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "viscamera.h"
#include "iopar.h"
#include "keystrs.h"

#include <osg/Camera>


mCreateFactoryEntry( visBase::Camera );

namespace visBase
{

const char* Camera::sKeyPosition() 	{ return sKey::Position(); }
const char* Camera::sKeyOrientation() 	{ return "Orientation"; }
const char* Camera::sKeyAspectRatio() 	{ return "Aspect ratio"; }
const char* Camera::sKeyNearDistance() 	{ return "Near Distance"; }
const char* Camera::sKeyFarDistance() 	{ return "Far Distance"; }
const char* Camera::sKeyFocalDistance()	{ return "Focal Distance"; }

class DrawCallback : public osg::Camera::DrawCallback
{
public:
    DrawCallback( visBase::Camera& cam )
        : camera_( cam )
    {}

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        camera_.triggerDrawCallBack( this, renderInfo );
    }

private:
    visBase::Camera&	camera_;
};


Camera::Camera()
    : camera_( new osg::Camera )
    , preDraw( this )
    , postDraw( this )
    , renderinfo_( 0 )
    , postdraw_( new DrawCallback( *this ) )
    , predraw_( new DrawCallback( *this ) )
{
    postdraw_->ref();
    predraw_->ref();

    camera_->getOrCreateStateSet()->setGlobalDefaults();
    camera_->setProjectionResizePolicy( osg::Camera::FIXED );

    camera_->setPreDrawCallback( predraw_ );
    camera_->setPostDrawCallback( postdraw_ );
    setOsgNode( camera_ );
}


Camera::~Camera()
{
    postdraw_->unref();
    predraw_->unref();
}


osg::Camera* Camera::osgCamera() const
{
    return camera_;
}


void Camera::setPosition(const Coord3& pos)
{
    pErrMsg("Not impl");
}


Coord3 Camera::position() const
{
    pErrMsg("Not impl");
    return Coord3::udf();
}


void Camera::setOrientation( const Coord3& dir, float angle )
{
    pErrMsg("Not impl");
}


void Camera::getOrientation( Coord3& dir, float& angle ) const
{
    pErrMsg("Not impl");
    dir = Coord3::udf();
}


void Camera::pointAt( const Coord3& pos )
{
    pErrMsg("Not impl");
}


void Camera::pointAt(const Coord3& pos, const Coord3& upvector )
{
    pErrMsg("Not impl");
}


void Camera::setAspectRatio( float n )
{
    pErrMsg("Not impl");
}


float Camera::aspectRatio() const
{
    pErrMsg("Not impl");
    return mUdf(float);
}


void Camera::setNearDistance( float n )
{
    pErrMsg("Not impl");
}


float Camera::nearDistance() const
{
    pErrMsg("Not impl");
    return mUdf(float);
}


void Camera::setFarDistance( float n )
{
    pErrMsg("Not impl");
}


float Camera::farDistance() const
{
    pErrMsg("Not impl");
    return mUdf(float);
}


void Camera::setFocalDistance(float n)
{
    pErrMsg("Not impl");
}


float Camera::focalDistance() const
{
    pErrMsg("Not impl");
    return mUdf(float);
}


void Camera::setStereoAdjustment(float n)
{
    pErrMsg("Not impl");
}

float Camera::getStereoAdjustment() const
{
    pErrMsg("Not impl");
    return mUdf(float);
}


void Camera::setBalanceAdjustment(float n)
{
    pErrMsg("Not impl");
}

float Camera::getBalanceAdjustment() const
{
    pErrMsg("Not impl");
    return mUdf(float);
}


int Camera::usePar( const IOPar& iopar )
{
    Coord3 pos;
    if ( iopar.get( sKeyPosition(), pos ) )
	setPosition( pos );

        double angle;
    if ( iopar.get( sKeyOrientation(), pos.x, pos.y, pos.z, angle ) )
	setOrientation( pos, angle );

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


void Camera::fillPar( IOPar& iopar ) const
{
    iopar.set( sKeyPosition(), position() );
    

    float angle;
    Coord3 orientation;
    getOrientation( orientation, angle );
    iopar.set( sKeyOrientation(),
	       orientation[0], orientation[1], orientation[2], (double) angle );

    iopar.set( sKeyAspectRatio(), aspectRatio() );
    iopar.set( sKeyNearDistance(), (int)(nearDistance()+.5) );
    iopar.set( sKeyFarDistance(), (int)(farDistance()+.5) );
    iopar.set( sKeyFocalDistance(), focalDistance() );
}


void Camera::triggerDrawCallBack( const DrawCallback* src,
                                  const osg::RenderInfo& ri )
{
    renderinfo_ = &ri;
    if ( src==predraw_ )
    {
        preDraw.trigger();
    }
    else if ( src==postdraw_ )
    {
        postDraw.trigger();
    }

    renderinfo_ = 0;
}


}; // namespace visBase
