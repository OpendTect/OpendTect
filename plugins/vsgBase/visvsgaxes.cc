/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgaxes.h"

#include "visvsgcamera.h"
#include "visvsgdataman.h"
#include "visvsgfontcreator.h"
#include "math2.h"
#include "vsggeoaxes.h"

mCreateDGBVisFactoryEntry( dGBVis::Axes );

namespace dGBVis
{

Axes::Axes()
    : axesnode_(vsgGeo::AxesNode::create())
    , pixeldensity_(getDefaultPixelDensity())
{
    ref();
    mAttachCB( FontCreator::instance().fontLoaded, Axes::fontLoadedCB );
    unRefNoDelete();
    setVsgNode( axesnode_.get() );
}


Axes::~Axes()
{
    detachAllNotifiers();
}


void Axes::prepare()
{
    axesnode_->setUpdater( DataObject::getUpdater() );
}


void Axes::setStyle( Style style )
{
    prepare();
    axesnode_->setStyle( style==Modern ? vsgGeo::AxesNode::Style::Modern
				       : vsgGeo::AxesNode::Style::Classic );
    requestSingleRedraw();
}


Axes::Style Axes::getStyle() const
{
    return axesnode_->style()==vsgGeo::AxesNode::Style::Modern ? Modern
							       : Classic;
}


float Axes::getMargin() const
{
    return axesnode_->getMargin();
}


float Axes::getRadius() const
{
    return axesnode_->getRadius();
}


void Axes::setRadius( float rad )
{
    prepare();
    axesnode_->setRadius( rad );
    requestSingleRedraw();
}


float Axes::getLength() const
{
    return axesnode_->getLength();
}


void Axes::setLength( float len )
{
    prepare();
    axesnode_->setLength( len );
    requestSingleRedraw();
}


void Axes::setPosition( float x, float y )
{
    axesnode_->setPosition( vsg::vec2(x,y) );
    requestSingleRedraw();
}


void Axes::setSize( float rad, float len )
{
    prepare();
    axesnode_->setSize( vsg::vec2(rad,len) );
    requestSingleRedraw();
}


void Axes::setAnnotationColor( const OD::Color& annotcolor )
{
    axesnode_->setAnnotationColor( toVec4(annotcolor,1.f) );
    requestSingleRedraw();
}


void Axes::setAnnotationTextSize( int size )
{
    const float sizefactor = pixeldensity_/getDefaultPixelDensity();
    prepare();
    axesnode_->setAnnotationTextSize( size*sizefactor );
    annottextsize_ = size;
    requestSingleRedraw();
}


void Axes::setAnnotationFont( const FontData& fd )
{
    fontdata_ = fd;
    FontCreator& creator = FontCreator::instance();
    vsg::ref_ptr<vsg::Font> font = creator.get( fontdata_ );
    prepare();
    if ( font )
	axesnode_->setAnnotationFont( font );
    else
	creator.request( fontdata_ );

    setAnnotationTextSize( fd.pointSize() );
    requestSingleRedraw();
}


void Axes::fontLoadedCB( CallBacker* )
{
    vsg::ref_ptr<vsg::Font> font = FontCreator::instance().get( fontdata_ );
    if ( !font )
	return;

    prepare();
    axesnode_->setAnnotationFont( font );
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


void Axes::setPrimaryCamera( Camera* camera )
{
    primarycamera_ = camera;
    axesnode_->setPrimaryCamera(
		vsg::ref_ptr<vsg::Camera>(camera ? camera->vsgCamera()
						 : nullptr) );
}

} // namespace dGBVis
