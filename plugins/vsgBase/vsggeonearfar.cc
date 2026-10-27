/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeonearfar.h"

#include <vsg/app/ViewMatrix.h>
#include <vsg/utils/ComputeBounds.h>

#include <limits>

namespace vsgGeo
{

void NearFar::setCamera( vsg::ref_ptr<vsg::Camera> camera )
{
    camera_ = camera;
}


void NearFar::setScene( vsg::ref_ptr<vsg::Node> scene )
{
    scene_ = scene;
}


void NearFar::setOffscreen( vsg::ref_ptr<vsg::Perspective> persp )
{
    offscreen_ = persp;
}


void NearFar::boundsChanged()
{
    boundsdirty_.store( true );
}


void NearFar::apply( vsg::Perspective& persp, double znear, double zfar )
{
    persp.nearDistance = znear;
    persp.farDistance = zfar;
}


void NearFar::run()
{
    vsg::ref_ptr<vsg::Node> scene = scene_.ref_ptr();
    vsg::ref_ptr<vsg::Camera> camera = camera_.ref_ptr();
    if ( !scene || !camera )
	return;

    if ( boundsdirty_.exchange(false) )
    {
	vsg::ref_ptr<vsg::ComputeBounds> cb = vsg::ComputeBounds::create();
	scene->accept( *cb );
	if ( cb->bounds.valid() )
	{
	    bounds_ = cb->bounds;
	    hasbounds_ = true;
	}
    }

    if ( !hasbounds_ )
	return;

    auto* lookat = dynamic_cast<vsg::LookAt*>( camera->viewMatrix.get() );
    auto* persp = dynamic_cast<vsg::Perspective*>(
					camera->projectionMatrix.get() );
    if ( !lookat || !persp )
	return;

    const vsg::dmat4 view = lookat->transform();
    double nearest = std::numeric_limits<double>::max();
    double farthest = -std::numeric_limits<double>::max();
    for ( int idx=0; idx<8; idx++ )
    {
	const vsg::dvec3 corner( idx&1 ? bounds_.max.x : bounds_.min.x,
				 idx&2 ? bounds_.max.y : bounds_.min.y,
				 idx&4 ? bounds_.max.z : bounds_.min.z );
	const vsg::dvec4 eye = view * vsg::dvec4( corner, 1.0 );
	const double depth = -eye.z;
	if ( depth<nearest )
	    nearest = depth;
	if ( depth>farthest )
	    farthest = depth;
    }

    // Nothing in front of the camera.
    if ( !(farthest>0.0) )
	return;

    constexpr double cNearPull = 0.98;
    constexpr double cFarPush = 1.02;
    constexpr double cNearFarRatio = 0.002;
    double znear = nearest * cNearPull;
    const double zfar = farthest * cFarPush;
    const double minnear = farthest * cNearFarRatio;
    if ( znear<minnear )
	znear = minnear;
    if ( !(znear>0.0) || !(zfar>znear) )
	return;

    apply( *persp, znear, zfar );
    if ( vsg::ref_ptr<vsg::Perspective> offscreen = offscreen_.ref_ptr() )
	apply( *offscreen, znear, zfar );
}

} // namespace vsgGeo
