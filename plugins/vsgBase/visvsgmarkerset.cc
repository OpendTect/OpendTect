/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgmarkerset.h"

#include "visvsgtransform.h"
#include "vsggeomarkerset.h"

#include "errmsg.h"

#include <vsg/maths/vec3.h>

namespace dGBVis
{

// visBase::MarkerSet multiplies the screen size by this before osgGeo
// draws it. The product is the diameter in pixels.
constexpr float cMarkerScaleFactor = 2.5f;


RefMan<MarkerSet> MarkerSet::create()
{
    RefMan<MarkerSet> ret = new MarkerSet;
    return ret;
}


MarkerSet::MarkerSet()
    : VisualObjectImpl(false)
{
    markers_ = vsgGeo::MarkerSet::create();
    setGroupNode( markers_.get() );
    setPickable( true, true );
}


MarkerSet::~MarkerSet()
{}


vsg::dvec3 MarkerSet::toDisplay( const Coord3& pos ) const
{
    Coord3 disp = pos;
    Transformation::transform( display_.ptr(), pos, disp );
    return vsg::dvec3( disp.x_, disp.y_, disp.z_ );
}


void MarkerSet::addPos( const Coord3& pos )
{
    if ( !isVisualizationThread() )
	{ pErrMsg("Not on the visualization thread"); }

    if ( !markers_ )
	return;

    positions_ += pos;
    vsg::ref_ptr<vsg::Node> node = markers_->addPos( toDisplay(pos) );
    compileNode( node );
    requestSingleRedraw();
}


void MarkerSet::setPos( int idx, const Coord3& pos )
{
    if ( !isVisualizationThread() )
	{ pErrMsg("Not on the visualization thread"); }

    if ( !markers_ || idx<0 || idx>=positions_.size() )
	return;

    positions_[idx] = pos;
    markers_->setPos( idx, toDisplay(pos) );
    requestSingleRedraw();
}


void MarkerSet::removePos( int idx )
{
    if ( !isVisualizationThread() )
	{ pErrMsg("Not on the visualization thread"); }

    if ( !markers_ || idx<0 || idx>=positions_.size() )
	return;

    vsg::ref_ptr<vsg::Node> removed = markers_->removePos( idx );
    if ( removed )
	releaseNode( removed.get() );
    positions_.removeSingle( idx );
    requestSingleRedraw();
}


void MarkerSet::setScreenSize( float pixels )
{
    if ( markers_ )
	markers_->setMarkerSize( cMarkerScaleFactor*pixels );
}


void MarkerSet::setMarkersSingleColor( const OD::Color& color )
{
    if ( !markers_ )
	return;

    markers_->setColor( toVec4(color,1.f) );
}


void MarkerSet::setCamera( vsg::Camera* camera )
{
    if ( markers_ )
	markers_->setCamera( camera );
}


void MarkerSet::setDisplayTransformation( const mDGBVisTrans* trans )
{
    display_ = trans;
    if ( !markers_ )
	return;

    for ( int idx=0; idx<positions_.size(); idx++ )
	markers_->setPos( idx, toDisplay(positions_[idx]) );

    requestSingleRedraw();
}

} // namespace dGBVis
