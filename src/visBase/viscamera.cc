/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2002
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
    DrawCallback( visBase::Camera& cam )
        : camera_( cam )
    {}

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        camera_.triggerDrawCallBack( this, renderInfo );
    }

    //Just to avoid warning
    virtual void operator () (const osg::Camera&) const
    { }

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


#define col2f(rgb) float(col.rgb())/255

void Camera::setBackgroundColor( const Color& col )
{
    if ( !camera_ )
	return;

    const osg::Vec4 osgcol( col2f(r), col2f(g), col2f(b), 1.0 );
    camera_->setClearColor( osgcol );
}


Color Camera::getBackgroundColor() const
{
    if ( !camera_ )
	return Color::NoColor();

    const osg::Vec4 col = camera_->getClearColor();
    return Color( mNINT32(col.r()*255), mNINT32(col.g()*255),
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


}; // namespace visBase
