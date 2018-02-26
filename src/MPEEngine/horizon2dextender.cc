/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR	: K. Tingdahl
 * DATE		: May 2006
___________________________________________________________________

-*/


#include "horizon2dextender.h"

#include "emhorizon2d.h"
#include "horizon2dtracker.h"
#include "math2.h"
#include "survinfo.h"
#include "uistrings.h"

namespace MPE
{

Horizon2DExtender::Horizon2DExtender( EM::Horizon2D& hor )
    : SectionExtender()
    , hor2d_( hor )
    , anglethreshold_( 0.5 )
{
}


SectionExtender* Horizon2DExtender::create( EM::Object* emobj )
{
    mDynamicCastGet(EM::Horizon2D*,hor,emobj)
    return emobj && !hor ? 0 : new Horizon2DExtender( *hor );
}


void Horizon2DExtender::initClass()
{
    SectionExtender::factory().addCreator( create, Horizon2DTracker::keyword(),
						uiStrings::s2DHorizon() );
}


void Horizon2DExtender::setAngleThreshold( float rad )
{ anglethreshold_ = cos( rad ); }


float Horizon2DExtender::getAngleThreshold() const
{ return Math::ACos(anglethreshold_); }


void Horizon2DExtender::setDirection( const TrcKeyValue& dir )
{
    direction_ = dir;
    xydirection_ =
	SI().transform( BinID(0,0) ) - SI().transform( dir.tk_.binID() );
    const float abs = xydirection_.abs<float>();
    alldirs_ = mIsZero( abs, 1e-3 );
    if ( !alldirs_ )
	xydirection_ /= abs;
}


const TrcKeyValue* Horizon2DExtender::getDirection() const
{ return &direction_; }

void Horizon2DExtender::setGeomID( Pos::GeomID geomid )
{ geomid_ = geomid; }

Pos::GeomID Horizon2DExtender::geomID() const
{ return geomid_; }


int Horizon2DExtender::nextStep()
{
    for ( int idx=0; idx<startpos_.size(); idx++ )
    {
	addNeighbor( false, startpos_[idx] );
	addNeighbor( true, startpos_[idx] );
    }

    return 0;
}


void Horizon2DExtender::addNeighbor( bool upwards, const TrcKey& src )
{
    const StepInterval<int> colrange =
	hor2d_.geometry().colRange( geomid_ );
    TrcKey neighbor = src;
    neighbor.setTrcNr( neighbor.trcNr()
	    + (upwards ? colrange.step : -colrange.step) );
    if ( !colrange.includes(neighbor.trcNr(),false) )
	return;

    const TrcKeyZSampling& boundary = getExtBoundary();
    if ( !boundary.isEmpty() &&
	    !boundary.hsamp_.includes(neighbor.position()) )
	return;

    const bool hasz = hor2d_.hasZ( neighbor );
    const bool isintersection = hor2d_.isAttrib(
			neighbor, EM::Object::sIntersectionNode() );
    if ( hasz && !isintersection )
	return;

    hor2d_.setZ( neighbor, hor2d_.getZ(src), true );
    addTarget( neighbor, src );
}


float Horizon2DExtender::getDepth( const TrcKey& src, const TrcKey& ) const
{
    return hor2d_.getZ( src );
}

}  // namespace MPE
