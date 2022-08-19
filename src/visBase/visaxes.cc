/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visaxes.h"

#include "color.h"
#include "viscamera.h"
#include "vistext.h"

#include <osg/Camera>
#include <osgGeo/AxesNode>

mCreateFactoryEntry( visBase::Axes );


namespace visBase
{

Axes::Axes()
    : axesnode_(new osgGeo::AxesNode)
    , mastercamera_(0)
    , pixeldensity_( getDefaultPixelDensity() )
    , annottextsize_(18)
{
    setOsgNode( axesnode_ );
}


Axes::~Axes()
{
   unRefPtr( mastercamera_ );
}


float Axes::getRadius() const
{
    return axesnode_->getRadius();
}


void Axes::setRadius( float rad )
{
    axesnode_->setRadius( rad );
}


float Axes::getLength() const
{
    return axesnode_->getLength();
}


void Axes::setLength( float len )
{
    axesnode_->setLength( len );
}


void Axes::setPosition( float x, float y )
{
    axesnode_->setPosition( osg::Vec2(x,y) );
}


void Axes::setSize( float rad, float len )
{
    axesnode_->setSize( osg::Vec2(rad,len) );
}


void Axes::setAnnotationColor( const OD::Color& annotcolor )
{
#define mColTof(c) float( c/255.0f )

    osg::Vec4 anncol( mColTof(annotcolor.r()),
		      mColTof(annotcolor.g()),
		      mColTof(annotcolor.b()), 1.0f );
    axesnode_->setAnnotationColor( anncol );
}


void Axes::setAnnotationTextSize( int size )
{
    const float sizefactor = pixeldensity_/getDefaultPixelDensity();
    axesnode_->setAnnotationTextSize(size*sizefactor);
    annottextsize_ = size;
}


void Axes::setAnnotationFont( const FontData& fd )
{
    osgText::Font* newfont = OsgFontCreator::create( fd );
    axesnode_->setAnnotationFont( newfont );
    setAnnotationTextSize( fd.pointSize() );
    requestSingleRedraw();
}


void Axes::setPixelDensity( float dpi )
{
    if ( dpi==pixeldensity_ )
	return;

    DataObject::setPixelDensity( dpi );
    pixeldensity_ = dpi;
    setAnnotationTextSize( annottextsize_ );
}


void Axes::setPrimaryCamera( visBase::Camera* camera )
{
    mastercamera_ = camera;
    mastercamera_->ref();
    axesnode_->setPrimaryCamera( mastercamera_->osgCamera() );
}


void Axes::setMasterCamera( visBase::Camera* camera )
{ setPrimaryCamera( camera ); }

} // namespace visBase
