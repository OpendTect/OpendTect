/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeopick.h"

#include <vsg/utils/LineSegmentIntersector.h>
#include <vsg/utils/PolytopeIntersector.h>

#include <limits>

namespace vsgGeo
{

void windowRay( const vsg::Camera& camera, int x, int y,
		vsg::dvec3& start, vsg::dvec3& end )
{
    const VkViewport& viewport = camera.getViewport();
    const double width = viewport.width>0. ? viewport.width : 1.;
    const double height = viewport.height>0. ? viewport.height : 1.;
    const double ndcx = (double(x)-viewport.x) / width * 2. - 1.;
    const double ndcy = (double(y)-viewport.y) / height * 2. - 1.;

    const vsg::dmat4 clipToWorld = vsg::inverse(
	camera.projectionMatrix->transform() *
	camera.viewMatrix->transform() );

    auto unproject = [&]( double ndcz )
    {
	const vsg::dvec4 clip = clipToWorld * vsg::dvec4( ndcx, ndcy,
							  ndcz, 1. );
	const double w = clip.w==0. ? 1. : clip.w;
	return vsg::dvec3( clip.x/w, clip.y/w, clip.z/w );
    };

    start = unproject( 0. );
    end = unproject( 1. );
}


template <class Intersection>
static Pick toPick( const Intersection& hit )
{
    Pick ret;
    ret.valid = true;
    ret.nodePath = hit->nodePath;
    ret.worldPoint = hit->worldIntersection;
    return ret;
}


static double distanceTo( const vsg::dvec3& eye, const vsg::dvec3& point )
{
    return vsg::length( point - eye );
}


static vsg::dvec3 cameraEye( const vsg::Camera& camera )
{
    const vsg::dvec4 eye = vsg::inverse( camera.viewMatrix->transform() ) *
			   vsg::dvec4( 0., 0., 0., 1. );
    return vsg::dvec3( eye.x, eye.y, eye.z );
}


Pick intersect( const vsg::Camera& camera, vsg::Node& root,
		int x, int y, vsg::Mask traversalmask, double pixelradius )
{
    vsg::dvec3 raystart, farpt;
    windowRay( camera, x, y, raystart, farpt );
    const vsg::dvec3 eye = cameraEye( camera );
    const double seglen = vsg::length( farpt - raystart );
    const double eps = 1e-6 * (seglen>0. ? seglen : 1.);

    vsg::ref_ptr<vsg::LineSegmentIntersector> lines =
		vsg::LineSegmentIntersector::create( camera, x, y );
    lines->traversalMask = traversalmask;
    root.accept( *lines );

    const double radius = pixelradius>0. ? pixelradius : 1.;
    vsg::ref_ptr<vsg::PolytopeIntersector> polys =
		vsg::PolytopeIntersector::create( camera,
			double(x)-radius, double(y)-radius,
			double(x)+radius, double(y)+radius );
    polys->traversalMask = traversalmask;
    root.accept( *polys );

    const vsg::LineSegmentIntersector::Intersection* linehit = nullptr;
    double linedist = std::numeric_limits<double>::max();
    for ( const auto& hit : lines->intersections )
    {
	if ( !hit )
	    continue;

	const double dist = distanceTo( eye, hit->worldIntersection );
	if ( dist < linedist )
	{
	    linedist = dist;
	    linehit = hit.get();
	}
    }

    const vsg::PolytopeIntersector::Intersection* polyhit = nullptr;
    double polydist = std::numeric_limits<double>::max();
    for ( const auto& hit : polys->intersections )
    {
	if ( !hit )
	    continue;

	const double dist = distanceTo( eye, hit->worldIntersection );
	if ( dist < polydist )
	{
	    polydist = dist;
	    polyhit = hit.get();
	}
    }

    Pick ret;
    if ( linehit && polyhit )
    {
	if ( polydist <= linedist+eps )
	    ret = toPick( polyhit );
	else
	    ret = toPick( linehit );
    }
    else if ( linehit )
	ret = toPick( linehit );
    else if ( polyhit )
	ret = toPick( polyhit );

    return ret;
}

} // namespace vsgGeo
