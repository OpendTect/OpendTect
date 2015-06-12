/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

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
    ExtenderFactory().addCreator( create, Horizon3DTracker::keyword() );
}


SectionExtender* Horizon3DExtender::create( EM::EMObject* emobj,
					    EM::SectionID sid )
{
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    return emobj && !hor ? 0 : new Horizon3DExtender( *hor, sid );
}


Horizon3DExtender::Horizon3DExtender( EM::Horizon3D& hor3d,
				      EM::SectionID sectionid )
   : BaseHorizon3DExtender( hor3d, sectionid )
{
}


BaseHorizon3DExtender::BaseHorizon3DExtender( EM::Horizon3D& hor3d,
					      EM::SectionID sectionid )
    : SectionExtender( sectionid )
    , horizon_( hor3d )
{}


void BaseHorizon3DExtender::setDirection( const TrcKeyValue& bdval )
{ direction_ =	bdval; }


int BaseHorizon3DExtender::maxNrPosInExtArea() const
{ return mCast( int, getExtBoundary().hsamp_.totalNr() ); }


void BaseHorizon3DExtender::preallocExtArea()
{
    const TrcKeySampling hrg = getExtBoundary().hsamp_;
    Geometry::BinIDSurface* bidsurf = horizon_.geometry().sectionGeometry(sid_);
    if ( bidsurf ) bidsurf->expandWithUdf(hrg.start_,hrg.stop_);
}


int BaseHorizon3DExtender::nextStep()
{
    const bool alldirs = direction_.lineNr()==0 && direction_.trcNr()==0;

    TypeSet<BinID> sourcenodes;

    for ( int idx=0; idx<startpos_.size(); idx++ )
    {
	BinID dummy = BinID::fromInt64( startpos_[idx] );
	sourcenodes += dummy;
    }

    if ( sourcenodes.size() == 0 )
	return 0;

    bool change = true;
    while ( change )
    {
	change = false;
	for ( int idx=0; idx<sourcenodes.size(); idx++ )
	{
	    const BinID& srcbid = sourcenodes[idx];

	    TypeSet<RowCol> directions;
	    if ( !alldirs )
	    {
		directions += RowCol( direction_.tk_.pos() );
		directions += RowCol( direction_.lineNr()*-1,
				      direction_.trcNr()*-1 );
	    }
	    else
	    {
		directions += RowCol( 0, 1 );
		directions += RowCol( 0, -1 );
		directions += RowCol( 1, 0 );
		directions += RowCol( -1, 0 );
	    }

	    const EM::PosID pid( horizon_.id(), sid_, srcbid.toInt64() );
	    for ( int idy=0; idy<directions.size(); idy++ )
	    {
		const EM::PosID neighbor =
			horizon_.geometry().getNeighbor( pid, directions[idy] );

		if ( neighbor.sectionID() != sid_ )
		    continue;

		const BinID neighbbid = BinID::fromInt64( neighbor.subID() );
		if ( !getExtBoundary().hsamp_.includes(neighbbid) )
		    continue;

		//If this is a better route to a node that is already
		//added, replace the route with this one

		const int previndex = addedpos_.indexOf( neighbor.subID() );
		if ( previndex!=-1 )
		{
		    const RowCol step( horizon_.geometry().step() );
		    const od_int64 serc = addedpossrc_[previndex];
		    const RowCol oldsrc( RowCol::fromInt64(serc)/step );
		    const RowCol dst( RowCol::fromInt64(serc)/step );
		    const RowCol cursrc( srcbid/step );

		    const int olddist = (int)oldsrc.sqDistTo(dst);
		    if ( cursrc.sqDistTo(dst) < olddist )
		    {
			addedpossrc_[previndex] = srcbid.toInt64();
			const float depth = getDepth( srcbid, neighbbid );
			horizon_.setZ( neighbbid, depth, setundo_ );
		    }
		    continue;
		}

		if ( horizon_.isDefined(neighbor) )
		    continue;

		if ( !isExcludedPos(neighbor.subID()) )
		{
		    const float depth = getDepth( srcbid, neighbbid );
		    if ( horizon_.setZ(neighbbid,depth,setundo_) )
		    {
			addTarget( neighbor.subID(), srcbid.toInt64() );
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
{ return extboundary_.isEmpty() ? engine().activeVolume() : extboundary_; }

} // namespace MPE
