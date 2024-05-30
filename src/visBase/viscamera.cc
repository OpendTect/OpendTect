/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "viscamera.h"

#include "iopar.h"
#include "keystrs.h"
#include "visosg.h"

#include <osg/Camera>


mCreateFactoryEntry( visBase::Camera );

namespace visBase
{

class DrawCallback : public osg::Camera::DrawCallback
{
public:
    DrawCallback( Camera& cam )
        : camera_( cam )
    {}

    void operator () (osg::RenderInfo& renderInfo) const override
    {
        camera_.triggerDrawCallBack( this, renderInfo );
    }

    //Just to avoid warning
    void operator () (const osg::Camera&) const override
    {}

private:
    Camera&	camera_;
};


Camera::Camera()
    : camera_(new osg::Camera)
    , preDraw(this)
    , postDraw(this)
    , postdraw_(new DrawCallback(*this))
    , predraw_(new DrawCallback(*this))
{
    refOsgPtr( predraw_ );
    refOsgPtr( postdraw_ );

    camera_->getOrCreateStateSet()->setGlobalDefaults();
    camera_->setProjectionResizePolicy( osg::Camera::FIXED );

    camera_->setPreDrawCallback( predraw_ );
    camera_->setPostDrawCallback( postdraw_ );
    setOsgNode( camera_ );
}


Camera::~Camera()
{
    unRefOsgPtr( predraw_ );
    unRefOsgPtr( postdraw_ );
}


osg::Camera* Camera::osgCamera() const
{
    return camera_;
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

    renderinfo_ = nullptr;
}


#define col2f(rgb) float(col.rgb())/255

void Camera::setBackgroundColor( const OD::Color& col )
{
    if ( !camera_ )
	return;

    const osg::Vec4 osgcol( col2f(r), col2f(g), col2f(b), 1.0 );
    camera_->setClearColor( osgcol );
}


OD::Color Camera::getBackgroundColor() const
{
    if ( !camera_ )
	return OD::Color::NoColor();

    const osg::Vec4 col = camera_->getClearColor();
    return OD::Color( mNINT32(col.r()*255), mNINT32(col.g()*255),
		      mNINT32(col.b()*255) );
}


Coord3 Camera::getTranslation() const
{
    osg::Vec3d	curscale;
    osg::Vec3d	curtrans;
    osg::Quat	currot;
    osg::Quat	curso;
    camera_->getViewMatrix().decompose( curtrans, currot, curscale, curso );
    return Conv::to<Coord3>( curtrans );
}


Coord3 Camera::getScale() const
{
    osg::Vec3d	curscale;
    osg::Vec3d	curtrans;
    osg::Quat	currot;
    osg::Quat	curso;
    camera_->getViewMatrix().decompose( curtrans, currot, curscale, curso );
    return Conv::to<Coord3>( curscale );
}


void Camera::getRotation( Coord3& vec, double& angle ) const
{
    osg::Vec3d	curscale;
    osg::Vec3d	curtrans;
    osg::Quat	currot;
    osg::Quat	curso;
    camera_->getViewMatrix().decompose( curtrans, currot, curscale, curso );
    osg::Vec3d osgvec;
    currot.getRotate( angle, osgvec );
    vec = Conv::to<Coord3>( osgvec );
}


void Camera::getLookAtMatrix( Coord3& eye, Coord3& center, Coord3& up ) const
{
    osg::Vec3d osgeye,osgcenter,osgup;
    camera_->getViewMatrixAsLookAt( osgeye, osgcenter, osgup );
    eye = Conv::to<Coord3>(osgeye);
    center = Conv::to<Coord3>(osgcenter);
    up = Conv::to<Coord3>(osgup);
}


} // namespace visBase
