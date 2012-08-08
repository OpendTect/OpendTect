/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2006
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: horizon2dextender.cc,v 1.15 2012-08-08 09:01:28 cvsaneesh Exp $";

#include "horizon2dextender.h"

#include "emhorizon2d.h"
#include "horizon2dtracker.h"
#include "math2.h"
#include "survinfo.h"

namespace MPE 
{

Horizon2DExtender::Horizon2DExtender( EM::Horizon2D& hor,
				      const EM::SectionID& sid )
    : SectionExtender( sid )
    , surface_( hor )
    , anglethreshold_( 0.5 )
{
}


SectionExtender* Horizon2DExtender::create( EM::EMObject* emobj,
						const EM::SectionID& sid )
{
    mDynamicCastGet(EM::Horizon2D*,hor,emobj)
    return emobj && !hor ? 0 : new Horizon2DExtender( *hor, sid );
}


void Horizon2DExtender::initClass()
{
    ExtenderFactory().addCreator( create, Horizon2DTracker::keyword() );
}


void Horizon2DExtender::setAngleThreshold( float rad )
{ anglethreshold_ = cos( rad ); }


float Horizon2DExtender::getAngleThreshold() const
{ return Math::ACos(anglethreshold_); }


void Horizon2DExtender::setDirection( const BinIDValue& dir )
{
    direction_ = dir;
    xydirection_ = SI().transform( BinID(0,0) ) - SI().transform( dir.binid );
    const double abs = xydirection_.abs();
    alldirs_ = mIsZero( abs, 1e-3 );
    if ( !alldirs_ )
	xydirection_ /= abs;
}


int Horizon2DExtender::nextStep()
{
    for ( int idx=0; idx<startpos_.size(); idx++ )
    {
	addNeighbor( false, startpos_[idx] );
	addNeighbor( true, startpos_[idx] );
    }

    return 0;
}


void Horizon2DExtender::addNeighbor( bool upwards, const EM::SubID& srcsubid )
{
    BinID srcbid = BinID::fromInt64( srcsubid );
    const StepInterval<int> colrange =
	surface_.geometry().colRange( sid_, geomid_ );
    EM::SubID neighborsubid;
    Coord3 neighborpos;
    BinID neighbrbid = srcbid;
    const CubeSampling& boundary = getExtBoundary();

    do 
    {
	neighbrbid += BinID( 0, upwards ? colrange.step : -colrange.step );
	if ( !colrange.includes(neighbrbid.crl,false) )
	    return;
	if ( !boundary.isEmpty() && !boundary.hrg.includes(BinID(neighbrbid)) )
	    return;
	neighborsubid = neighbrbid.toInt64();
	neighborpos = surface_.getPos( sid_, neighborsubid );
    }
    while ( !Coord(neighborpos).isDefined() );

    if ( neighborpos.isDefined() )
	return;

    const Coord3 sourcepos = surface_.getPos( sid_, srcsubid );

    if ( !alldirs_ )
    {
	const Coord dir = neighborpos - sourcepos;
	const double dirabs = dir.abs();
	if ( !mIsZero(dirabs,1e-3) )
	{
	    const Coord normdir = dir/dirabs;
	    const double cosangle = normdir.dot(xydirection_);
	    if ( cosangle<anglethreshold_ )
		return;
	}
    }

    Coord3 refpos = surface_.getPos( sid_, neighborsubid );
    refpos.z = getDepth( srcsubid, neighborsubid );
    surface_.setPos( sid_, neighborsubid, refpos, true );

    addTarget( neighborsubid, srcsubid );
}


float Horizon2DExtender::getDepth( const EM::SubID& srcrc,
				       const EM::SubID& destrc ) const
{
    return surface_.getPos( sid_, srcrc ).z;
}


};  // namespace MPE
