/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/


#include "horizon3dextender.h"

#include "binidsurface.h"
#include "emfault.h"
#include "emhorizon3d.h"
#include "horizon3dtracker.h"
#include "mpeengine.h"
#include "survinfo.h"

namespace MPE
{

void Horizon3DExtender::initClass()
{
    SectionExtender::factory().addCreator( create, Horizon3DTracker::keyword(),
						tr("3D Horizon Tracker") );
}


SectionExtender* Horizon3DExtender::create( EM::Object* emobj )
{
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    return emobj && !hor ? 0 : new Horizon3DExtender( *hor );
}


Horizon3DExtender::Horizon3DExtender( EM::Horizon3D& hor3d )
   : BaseHorizon3DExtender( hor3d )
{
}


BaseHorizon3DExtender::BaseHorizon3DExtender( EM::Horizon3D& hor3d )
    : SectionExtender()
    , horizon_( hor3d )
{}


void BaseHorizon3DExtender::setDirection( const TrcKeyValue& bdval )
{ direction_ =	bdval; }


int BaseHorizon3DExtender::maxNrPosInExtArea() const
{ return mCast( int, getExtBoundary().hsamp_.totalNr() ); }


void BaseHorizon3DExtender::preallocExtArea()
{
    const TrcKeySampling hrg = getExtBoundary().hsamp_;
    Geometry::BinIDSurface* bidsurf = horizon_.geometry().geometryElement();
    if ( bidsurf ) bidsurf->expandWithUdf( hrg.start_,hrg.stop_ );
}


int BaseHorizon3DExtender::nextStep()
{
    const bool fourdirs = direction_.lineNr()==0 && direction_.trcNr()==0;
    const bool eightdirs = direction_.lineNr()==1 && direction_.trcNr()==1;

    TypeSet<BinID> sourcenodes;

    for ( int idx=0; idx<startpos_.size(); idx++ )
	sourcenodes += startpos_[idx].binID();

    if ( sourcenodes.size() == 0 )
	return 0;

    bool change = true;
    while ( change )
    {
	change = false;
	for ( int idx=0; idx<sourcenodes.size(); idx++ )
	{
	    TypeSet<RowCol> directions;
	    if ( fourdirs || eightdirs )
	    {
		directions += RowCol( 0, 1 );
		directions += RowCol( 0, -1 );
		directions += RowCol( 1, 0 );
		directions += RowCol( -1, 0 );
		if ( eightdirs )
		{
		    directions += RowCol( 1, 1 );
		    directions += RowCol( 1, -1 );
		    directions += RowCol( -1, 1 );
		    directions += RowCol( -1, -1 );
		}
	    }
	    else
	    {
		directions += RowCol( direction_.tk_.binID() );
		directions += RowCol( direction_.lineNr()*-1,
				      direction_.trcNr()*-1 );
	    }

	    const BinID& srcbid = sourcenodes[idx];
	    const EM::PosID pid = EM::PosID::getFromRowCol( srcbid );
	    for ( int idy=0; idy<directions.size(); idy++ )
	    {
		const EM::PosID neighbor =
			horizon_.geometry().getNeighbor( pid, directions[idy] );

		const BinID neighbbid = neighbor.getBinID();
		if ( !getExtBoundary().hsamp_.includes(neighbbid) )
		    continue;

		//If this is a better route to a node that is already
		//added, replace the route with this one

		if ( horizon_.isDefined(neighbor) )
		    continue;

		if ( !isExcludedPos(TrcKey(neighbbid)) )
		{
		    const float depth = getDepth( TrcKey(srcbid),
						  TrcKey(neighbbid) );
		    if ( !mIsUdf(depth) && horizon_.setZ(neighbbid,depth,
								    setundo_) )
		    {
			addTarget( TrcKey(neighbbid), TrcKey(srcbid) );
			change = true;
		    }
		}
	    }
	}
    }

    return 0;
}


float BaseHorizon3DExtender::getDepth( const TrcKey& src,
				       const TrcKey& ) const
{
    return horizon_.getZ( src );
}


const TrcKeyZSampling& BaseHorizon3DExtender::getExtBoundary() const
{ return extboundary_.isEmpty() || extboundary_.hsamp_.totalNr()==1
	? engine().activeVolume() : extboundary_; }

} // namespace MPE
