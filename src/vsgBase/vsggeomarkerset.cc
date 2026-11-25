/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeomarkerset.h"

#include <vsg/app/Camera.h>
#include <vsg/app/RecordTraversal.h>
#include <vsg/maths/transform.h>
#include <vsg/state/ViewportState.h>
#include <vsg/utils/Builder.h>
#include <vsg/utils/Intersector.h>
#include <vsg/vk/State.h>

#include <algorithm>
#include <cmath>

namespace vsgGeo
{

MarkerSet::MarkerSet()
{}


void MarkerSet::setCamera( vsg::Camera* camera )
{
    camera_ = camera;
}


void MarkerSet::setMarkerSize( float pixels )
{
    markersize_ = pixels>0.f ? pixels : 1.f;
}


void MarkerSet::setColor( const vsg::vec4& color )
{
    color_ = color;
}


void MarkerSet::ensureShape()
{
    if ( shape_ )
	return;

    vsg::ref_ptr<vsg::Builder> builder = vsg::Builder::create();
    vsg::GeometryInfo geom;
    geom.color = color_;
    vsg::StateInfo state;
    state.lighting = true;
    shape_ = builder->createSphere( geom, state );
}


vsg::ref_ptr<vsg::Node> MarkerSet::addPos( const vsg::dvec3& pos )
{
    ensureShape();
    vsg::ref_ptr<vsg::MatrixTransform> xform =
		vsg::MatrixTransform::create( vsg::translate(pos) );
    xform->setValue( markerIndexKey(), int(markers_.size()) );
    xform->addChild( shape_ );
    addChild( xform );
    markers_.push_back( Marker{ pos, xform } );
    return xform;
}


void MarkerSet::setPos( int idx, const vsg::dvec3& pos )
{
    if ( idx<0 || idx>=int(markers_.size()) )
	return;

    markers_[idx].pos_ = pos;
    markers_[idx].xform_->matrix = vsg::translate( pos );
}


vsg::ref_ptr<vsg::Node> MarkerSet::removePos( int idx )
{
    if ( idx<0 || idx>=int(markers_.size()) )
	return {};

    vsg::ref_ptr<vsg::Node> removed = markers_[idx].xform_;
    auto& kids = children;
    for ( auto it=kids.begin(); it!=kids.end(); ++it )
    {
	if ( it->get()==removed.get() )
	{
	    kids.erase( it );
	    break;
	}
    }

    markers_.erase( markers_.begin()+idx );
    for ( int i=idx; i<int(markers_.size()); ++i )
	markers_[i].xform_->setValue( markerIndexKey(), i );

    return removed;
}


vsg::dvec3 MarkerSet::screenScale( const vsg::dmat4& modelview,
				   const vsg::dmat4& projection,
				   const vsg::dvec3& pos,
				   double viewheight ) const
{
    const auto axislen = []( const vsg::dvec4& column )
    {
	return std::max( 1e-12, vsg::length(vsg::dvec3(column.x,
						       column.y,
						       column.z)) );
    };
    const double ax = axislen( modelview[0] );
    const double ay = axislen( modelview[1] );
    const double az = axislen( modelview[2] );

    const double yscale = std::abs( projection[1][1] );
    if ( yscale<1e-12 || viewheight<1. )
	return vsg::dvec3( 1., 1., 1. );

    double worldperpixel = (2./yscale) / viewheight;
    if ( projection[2][3]!=0. )
    {
	const vsg::dvec4 eye = modelview * vsg::dvec4( pos, 1. );
	const double dist = std::max( 1e-6, -eye.z );
	worldperpixel = (2.*dist/yscale) / viewheight;
    }

    const double world = double(markersize_) * worldperpixel;
    return vsg::dvec3( world/ax, world/ay, world/az );
}


bool MarkerSet::applyScreenScale( const vsg::dmat4& modelview,
				  const vsg::dmat4& projection,
				  double viewheight ) const
{
    if ( markers_.empty() || viewheight<1. )
	return false;

    for ( Marker& marker : markers_ )
    {
	const vsg::dvec3 scale =
		screenScale( modelview, projection, marker.pos_, viewheight );
	marker.xform_->matrix = vsg::translate( marker.pos_ ) *
				 vsg::scale( scale.x, scale.y, scale.z );
    }

    return true;
}


bool MarkerSet::applyScreenScale( vsg::RecordTraversal& rt ) const
{
    vsg::ref_ptr<vsg::Camera> camera = camera_.ref_ptr();
    const vsg::State* state = rt.getState();
    if ( !camera || !camera->viewportState ||
	 camera->viewportState->viewports.empty() || !state )
	return false;

    const double height =
		std::abs( camera->viewportState->viewports[0].height );
    return applyScreenScale( state->modelviewMatrixStack.top(),
			     state->projectionMatrixStack.top(), height );
}


bool MarkerSet::applyPickScale( vsg::Intersector& intersector ) const
{
    vsg::ref_ptr<vsg::Camera> camera = camera_.ref_ptr();
    if ( !camera || !camera->viewMatrix || !camera->projectionMatrix ||
	 !camera->viewportState || camera->viewportState->viewports.empty() )
	return false;

    const auto& stack = intersector.localToWorldStack();
    if ( stack.empty() )
	return false;

    const vsg::dmat4& world = stack.back();
    const double height =
		std::abs( camera->viewportState->viewports[0].height );
    return applyScreenScale( camera->viewMatrix->transform()*world,
			     camera->projectionMatrix->transform(), height );
}


void MarkerSet::traverse( vsg::ConstVisitor& visitor ) const
{
    vsg::Intersector* intersector = dynamic_cast<vsg::Intersector*>( &visitor );
    const bool scaled = intersector && applyPickScale( *intersector );
    Inherit::traverse( visitor );
    if ( scaled )
	resetScale();
}


void MarkerSet::resetScale() const
{
    for ( Marker& marker : markers_ )
	marker.xform_->matrix = vsg::translate( marker.pos_ );
}


void MarkerSet::accept( vsg::RecordTraversal& rt ) const
{
    const bool scaled = applyScreenScale( rt );
    Inherit::traverse( rt );
    if ( scaled )
	resetScale();
}

} // namespace vsgGeo
