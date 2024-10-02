/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visscalebar.h"

#include "pickset.h"
#include "survinfo.h"
#include "viscoord.h"
#include "visdatagroup.h"
#include "visdrawstyle.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vislines.h"
#include "vistransform.h"


mCreateFactoryEntry( visBase::ScaleBar );

namespace visBase
{

ScaleBar::ScaleBar()
    : VisualObjectImpl(true)
    , firstloc_(*new Pick::Location(Coord3::udf()))
{
    ref();
    markers_ = MarkerSet::create();
    lines_ = Lines::create();
    firstloc_.setPos( Coord3::udf() );

    markers_->setMaterial( nullptr );
    markers_->setMarkerStyle( MarkerStyle3D::Sphere );
    markers_->setScreenSize( 2.5f );
    markers_->setMarkersSingleColor( getMaterial()->getColor() );
    addChild( markers_->osgNode() );

    RefMan<Geometry::RangePrimitiveSet> ps =
					Geometry::RangePrimitiveSet::create();
    ps->setRange( Interval<int>(0,1) );

    lines_->addPrimitiveSet( ps );
    lines_->setMaterial( nullptr );
    addChild( lines_->osgNode() );

    RefMan<DrawStyle> drawstyle = DrawStyle::create();
    linestyle_ = lines_->addNodeState( drawstyle.ptr() );
    setLineWidth( 2 );
    unRefNoDelete();
}


ScaleBar::~ScaleBar()
{
    delete &firstloc_;
    lines_->removeNodeState( linestyle_ );
}


void ScaleBar::setLength( double l )
{
    if ( length_ == l )
	return;

    length_ = l;
    updateVis( firstloc_ );
}


void ScaleBar::setOnInlCrl( bool yn )
{
    if ( oninlcrl_ == yn )
	return;

    oninlcrl_ = yn;
    updateVis( firstloc_ );
}


void ScaleBar::setOrientation( int orient )
{
    if ( orientation_ == orient )
	return;

    orientation_ = orient;
    updateVis( firstloc_ );
}


void ScaleBar::setPick( const Pick::Location& loc )
{
    firstloc_ = loc;
    updateVis( loc );
}


void ScaleBar::updateVis( const Pick::Location& loc )
{
    if ( !loc.pos().isDefined() || loc.dir()==Sphere() )
	return;

    pos_ = loc.pos();
    const Coord3 pos2 = getSecondPos( loc );
    if ( !pos_.isDefined() || !pos2.isDefined() )
	return;

    markers_->setPos( 0, pos_ );
    markers_->setPos( 1, pos2 );

    lines_->getCoordinates()->setPos( 0, pos_ );
    lines_->getCoordinates()->setPos( 1, pos2 );
    lines_->dirtyCoordinates();
}


void ScaleBar::setColor( OD::Color c )
{
    markers_->setMarkersSingleColor( c );
}


Coord3 ScaleBar::getSecondPos( const Pick::Location& loc ) const
{
    Coord3 pos = Coord3::udf();
    if ( oninlcrl_ )
    {
	if ( orientation_ == 1 )
	{
	    pos = loc.pos();
            pos.z_ += length_;
	}
	else
	{
	    const Coord3 vector = spherical2Cartesian( loc.dir(), true );
            const double signx = vector.x_ > 0 ? 1 : -1;
            const double signy = vector.y_ > 0 ? 1 : -1;

	    const double l2 = length_*length_;
            const double vx2 = vector.x_*vector.x_;
            const double vy2 = vector.y_*vector.y_;
	    const double v2 = vx2 + vy2;
	    const double factor = l2 / v2;
	    const double dx2 = vx2 * factor;
	    const double dy2 = vy2 * factor;

	    pos = loc.pos() +
		Coord3( signx*Math::Sqrt(dx2), signy*Math::Sqrt(dy2), 0 );
	}
    }
    else
    {
	pos = loc.pos();
	if ( orientation_ == 0 )
            pos.x_ += length_;
	else
            pos.y_ += length_;
    }

    return pos;
}


void ScaleBar::setLineWidth( int width )
{
    linestyle_->setLineStyle( OD::LineStyle(OD::LineStyle::Solid,width) );
    requestSingleRedraw();
}


void ScaleBar::setDisplayTransformation( const mVisTrans* nt )
{
    if ( displaytrans_ )
	return;

    markers_->setDisplayTransformation( nt );
    lines_->setDisplayTransformation( nt );
    displaytrans_ = nt;
}

} // namespace visBase
