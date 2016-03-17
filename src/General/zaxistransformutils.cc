/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2007
________________________________________________________________________

-*/

#include "zaxistransformutils.h"

#include "binidvalue.h"
#include "datapointset.h"
#include "zaxistransform.h"

ZAxisTransformPointGenerator::ZAxisTransformPointGenerator(
						ZAxisTransform& zat )
    : transform_(zat)
    , voiid_(-1)
    , dps_(0)
{
    transform_.ref();
}


ZAxisTransformPointGenerator::~ZAxisTransformPointGenerator()
{
    deepErase( bidvalsets_ );
    transform_.unRef();
}


void ZAxisTransformPointGenerator::setInput( const TrcKeyZSampling& cs,
					     TaskRunner* taskr )
{
    tkzs_ = cs;
    iter_.setSampling( cs.hsamp_ );

    if ( transform_.needsVolumeOfInterest() )
    {
	if ( voiid_ < 0 )
	    voiid_ = transform_.addVolumeOfInterest( cs, true );
	else
	    transform_.setVolumeOfInterest( voiid_, cs, true );

	transform_.loadDataIfMissing( voiid_, taskr );
    }
}


bool ZAxisTransformPointGenerator::doPrepare( int nrthreads )
{
    deepErase( bidvalsets_ );
    for ( int idx=0; idx<nrthreads; idx++ )
	bidvalsets_ += new BinIDValueSet( dps_->bivSet().nrVals(),
				dps_->bivSet().allowsDuplicateIdxPairs() );
    return true;
}


bool ZAxisTransformPointGenerator::doWork(
			od_int64 start, od_int64 stop, int threadid )
{
    if ( !dps_ ) return false;
    BinIDValue curpos;
    curpos.val() = tkzs_.zsamp_.start;
    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	iter_.next(curpos);
	const float depth = transform_.transformBack( curpos );
	if ( mIsUdf(depth) )
	    continue;

	DataPointSet::Pos newpos( curpos, depth );
	DataPointSet::DataRow dtrow( newpos );
	TypeSet<float> vals;
	dtrow.getBVSValues( vals, dps_->is2D(), dps_->isMinimal() );
	bidvalsets_[threadid]->add( dtrow.binID(), vals );
    }

    dps_->dataChanged();
    return true;
}


bool ZAxisTransformPointGenerator::doFinish( bool success )
{
    for ( int idx=0; idx<bidvalsets_.size(); idx++ )
    {
	dps_->bivSet().append( *bidvalsets_[idx] );
    }

    dps_->dataChanged();
    return success;
}
