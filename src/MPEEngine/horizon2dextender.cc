/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizon2dextender.h"

#include "emhorizon2d.h"
#include "horizon2dtracker.h"
#include "math2.h"
#include "survinfo.h"

namespace MPE
{

Horizon2DExtender::Horizon2DExtender( EM::Horizon2D& hor )
    : SectionExtender()
    , hor2d_( hor )
{
}


Horizon2DExtender::~Horizon2DExtender()
{}


SectionExtender* Horizon2DExtender::create( EM::EMObject* emobj )
{
    mDynamicCastGet(EM::Horizon2D*,hor,emobj)
    return hor ? new Horizon2DExtender( *hor ) : nullptr;
}


void Horizon2DExtender::initClass()
{
    ExtenderFactory().addCreator( create, Horizon2DTracker::keyword() );
}


void Horizon2DExtender::setAngleThreshold( float rad )
{ anglethreshold_ = cos( rad ); }


float Horizon2DExtender::getAngleThreshold() const
{ return Math::ACos(anglethreshold_); }


void Horizon2DExtender::setDirection( const TrcKeyValue& dir )
{
    direction_ = dir;
    xydirection_ =
	SI().transform( BinID(0,0) ) - SI().transform( dir.tk_.position() );
    const double abs = xydirection_.abs();
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
    const StepInterval<int> colrange = hor2d_.geometry().colRange( geomid_ );
    TrcKey neighbor = src;
    const TrcKey& cstneighbor = const_cast<const TrcKey&>( neighbor );
    neighbor.setTrcNr( cstneighbor.trcNr() +
		       (upwards ? colrange.step : -colrange.step) );
    if ( !colrange.includes(cstneighbor.trcNr(),false) )
	return;

    const TrcKeyZSampling& boundary = getExtBoundary();
    if ( !boundary.isEmpty() && !boundary.hsamp_.includes(cstneighbor) )
	return;

    const bool hasz = hor2d_.hasZ( cstneighbor );
    const bool isintersection = hor2d_.isAttrib(
			cstneighbor, EM::EMObject::sIntersectionNode() );
    if ( hasz && !isintersection )
	return;

    hor2d_.setZAndNodeSourceType(
	cstneighbor, hor2d_.getZ(src), true, EM::EMObject::Auto );
    addTarget( cstneighbor, src );
}


float Horizon2DExtender::getDepth( const TrcKey& src, const TrcKey& ) const
{
    return hor2d_.getZ( src );
}

}  // namespace MPE
