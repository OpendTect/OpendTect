/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizon3dextender.h"

#include "binidsurface.h"
#include "horizon3dtracker.h"
#include "mpeengine.h"


// MPE::Horizon3DExtenderBase

mImplFactory1Param( MPE::Horizon3DExtenderBase, EM::Horizon3D&,
		    MPE::Horizon3DExtenderBase::factory );

MPE::Horizon3DExtenderBase::Horizon3DExtenderBase( EM::Horizon3D& hor3d )
    : SectionExtender()
    , horizon_( hor3d )
{}


MPE::Horizon3DExtenderBase::~Horizon3DExtenderBase()
{}


MPE::Horizon3DExtenderBase*
	MPE::Horizon3DExtenderBase::createInstance( EM::Horizon3D& hor )
{
    const auto& horextfact = factory();
    BufferString typestr = horextfact.getDefaultName();
    if ( !horextfact.hasName(typestr.buf()) && !horextfact.isEmpty() )
	typestr = horextfact.getNames().last()->buf();

    return horextfact.create( typestr.buf(), hor );
}


void MPE::Horizon3DExtenderBase::setDirection( const TrcKeyValue& bdval )
{
    direction_ = bdval;
}


int MPE::Horizon3DExtenderBase::maxNrPosInExtArea() const
{
    return mCast( int, getExtBoundary().hsamp_.totalNr() );
}


void MPE::Horizon3DExtenderBase::preallocExtArea()
{
    const TrcKeySampling hrg = getExtBoundary().hsamp_;
    Geometry::BinIDSurface* bidsurf = horizon_.geometry().geometryElement();
    if ( bidsurf )
	bidsurf->expandWithUdf( hrg.start_,hrg.stop_ );
}


int MPE::Horizon3DExtenderBase::nextStep()
{
    if ( startpos_.isEmpty() )
	return Finished();

    const bool fourdirs = direction_.lineNr()==0 && direction_.trcNr()==0;
    const bool eightdirs = direction_.lineNr()==1 && direction_.trcNr()==1;

    TypeSet<TrcKey> sourcenodes( startpos_ );

    bool change = true;
    while ( change )
    {
	change = false;
	for ( const auto& sourcenode : sourcenodes )
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
		directions += RowCol( direction_.tk_.position() );
		directions += RowCol( direction_.lineNr()*-1,
				      direction_.trcNr()*-1 );
	    }

	    const EM::PosID pid( horizon_.id(), sourcenode.position() );
	    for ( int idy=0; idy<directions.size(); idy++ )
	    {
		const EM::PosID neighbor =
			horizon_.geometry().getNeighbor( pid, directions[idy] );

		const TrcKey neighbtk( BinID::fromInt64( neighbor.subID() ) );
		if ( !getExtBoundary().hsamp_.includes(neighbtk) )
		    continue;

		//If this is a better route to a node that is already
		//added, replace the route with this one

/*
		const int previndex = addedpos_.indexOf( neighbor );
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
			addedpossrc_[previndex] = srcbid;
			const float depth = getDepth( srcbid, neighbbid );
			horizon_.setZ( neighbbid, depth, setundo_ );
		    }
		    continue;
		}
*/
		if ( horizon_.isDefined(neighbor) )
		    continue;

		if ( !isExcludedPos(neighbtk) )
		{
		    const float depth = getDepth( sourcenode, neighbtk );
		    if ( !mIsUdf(depth) &&
			 horizon_.setZAndNodeSourceType(
			 neighbtk,depth,setundo_,EM::EMObject::Auto) )
		    {
			addTarget( neighbtk, sourcenode );
			change = true;
		    }
		}
	    }
	}
    }

    return Finished();
}


float MPE::Horizon3DExtenderBase::getDepth( const TrcKey& src,
					    const TrcKey& /* dest */) const
{
    return horizon_.getZ( src );
}


const TrcKeyZSampling& MPE::Horizon3DExtenderBase::getExtBoundary() const
{
    return extboundary_.isEmpty() || extboundary_.hsamp_.totalNr()==1
	    ? engine().activeVolume() : extboundary_;
}


// MPE::Horizon3DExtender

MPE::Horizon3DExtender::Horizon3DExtender( EM::Horizon3D& hor3d )
   : Horizon3DExtenderBase(hor3d)
{}


MPE::Horizon3DExtender::~Horizon3DExtender()
{}
