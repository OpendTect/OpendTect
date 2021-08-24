/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
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
    : visBase::VisualObjectImpl(true)
    , displaytrans_(0)
    , length_(1000)
    , firstloc_(*new Pick::Location)
    , oninlcrl_(true)
    , orientation_(0)
    , markers_(new visBase::MarkerSet())
    , lines_(new visBase::Lines())
{
    firstloc_.pos_ = Coord3::udf();

    markers_->setMaterial( 0 );
    markers_->setMarkerStyle( MarkerStyle3D::Sphere );
    markers_->setScreenSize( 2.5f );
    markers_->setMarkersSingleColor( getMaterial()->getColor() );
    markers_->ref();
    addChild( markers_->osgNode() );

    Geometry::RangePrimitiveSet* ps = Geometry::RangePrimitiveSet::create();
    ps->setRange( Interval<int>(0,1) );
   
    lines_->addPrimitiveSet( ps );
    lines_->setMaterial( 0 );
    lines_->ref();
    addChild( lines_->osgNode() );
    
    linestyle_ = lines_->addNodeState( new visBase::DrawStyle );
    linestyle_->ref();
    setLineWidth( 2 );
}


ScaleBar::~ScaleBar()
{
    delete &firstloc_;
    lines_->removeNodeState( linestyle_ );
    linestyle_->unRef();
    markers_->unRef();
    lines_->unRef();
    if ( displaytrans_ ) displaytrans_->unRef();
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
    if ( !loc.pos_.isDefined() || loc.dir_==Sphere() )
	return;

    pos_ = loc.pos_;
    const Coord3 pos2 = getSecondPos( loc );
    if ( !pos_.isDefined() || !pos2.isDefined() )
	return;

    markers_->setPos( 0, pos_ );
    markers_->setPos( 1, pos2 );
        
    lines_->getCoordinates()->setPos( 0, pos_ );
    lines_->getCoordinates()->setPos( 1, pos2 );
    lines_->dirtyCoordinates();
}


void ScaleBar::setColor( Color c )
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
	    pos = loc.pos_;
	    pos.z += length_;
	}
	else
	{
	    const Coord3 vector = spherical2Cartesian( loc.dir_, true );
	    const double signx = vector.x > 0 ? 1 : -1;
	    const double signy = vector.y > 0 ? 1 : -1;

	    const double l2 = length_*length_;
	    const double vx2 = vector.x*vector.x;
	    const double vy2 = vector.y*vector.y;
	    const double v2 = vx2 + vy2;
	    const double factor = l2 / v2;
	    const double dx2 = vx2 * factor;
	    const double dy2 = vy2 * factor;

	    pos = loc.pos_ +
		Coord3( signx*Math::Sqrt(dx2), signy*Math::Sqrt(dy2), 0 );
	}
    }
    else
    {
	pos = loc.pos_;
	if ( orientation_ == 0 )
	    pos.x += length_;
	else
	    pos.y += length_;
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
    if ( displaytrans_ )
	displaytrans_->ref();
}

}
