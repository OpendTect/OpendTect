/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeogradientbackground.h"

#include "vsggeotriangles.h"

#include <algorithm>
#include <cmath>

namespace
{

constexpr int cNrSteps = 32;
constexpr int cNrVertices = cNrSteps * cNrSteps * 6;

} // anonymous namespace


namespace vsgGeo
{

GradientBackground::GradientBackground()
    : vertices_(vsg::vec3Array::create(cNrVertices))
    , colors_(vsg::vec4Array::create(cNrVertices))
{
    vertices_->properties.dataVariance = vsg::DYNAMIC_DATA;
    colors_->properties.dataVariance = vsg::DYNAMIC_DATA;
    update();
    vsg::ref_ptr<vsg::Node> node = createTriangleList( vertices_, colors_ );
    if ( node )
	addChild( node );
}


void GradientBackground::setColors( const vsg::vec4& center,
				    const vsg::vec4& corner )
{
    center_ = center;
    corner_ = corner;
    update();
}


void GradientBackground::setSize( float width, float height )
{
    if ( width==width_ && height==height_ )
	return;

    width_ = std::max( 1.f, width );
    height_ = std::max( 1.f, height );
    update();
}


void GradientBackground::setVertex( int idx, float x, float y )
{
    const float dx = x - width_ / 2.f;
    const float dy = y - height_ / 2.f;
    const float frac = std::min( 1.f, std::sqrt(dx*dx+dy*dy) / halfdiagonal_ );
    vertices_->at( idx ) = vsg::vec3( x, y, 0.f );
    colors_->at( idx ) = center_ + ( corner_ - center_ ) * frac;
}


void GradientBackground::update()
{
    halfdiagonal_ = std::sqrt( width_*width_ + height_*height_ ) / 2.f;
    int idx = 0;
    for ( int row=0; row<cNrSteps; row++ )
    {
	const float y0 = height_ * row / cNrSteps;
	const float y1 = height_ * (row+1) / cNrSteps;
	for ( int col=0; col<cNrSteps; col++ )
	{
	    const float x0 = width_ * col / cNrSteps;
	    const float x1 = width_ * (col+1) / cNrSteps;
	    setVertex( idx++, x0, y0 );
	    setVertex( idx++, x1, y0 );
	    setVertex( idx++, x1, y1 );
	    setVertex( idx++, x0, y0 );
	    setVertex( idx++, x1, y1 );
	    setVertex( idx++, x0, y1 );
	}
    }

    vertices_->dirty();
    colors_->dirty();
}

} // namespace vsgGeo
