/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisdatapackfromdps.h"

#include "arrayndimpl.h"
#include "bufstringset.h"
#include "datapointset.h"
#include "seisdatapack.h"

#define mZColID 9999

SeisDataPackFromDPS::SeisDataPackFromDPS( const DataPointSet& dps,
						RegularSeisDataPack& dp,
					  const BufferStringSet& colnames )
    : ParallelTask("Extracting Seismic Cube")
    , dps_(dps),dp_(dp)
{
    for ( int idx=0; idx<colnames.size(); idx++ )
    {
	DataPointSet::ColID colid = dps_.indexOf( colnames.get(idx) );
	if ( colid < 0 )
	{
	    if ( colnames.get(idx) == "Z" )
		colid = mZColID;
	    else
		continue;
	}

	int compidx = dp_.getComponentIdx( colnames.get(idx) );
	if ( compidx < 0 && !dp_.addComponent(colnames.get(idx)) )
	    continue;

	compidx = dp_.getComponentIdx( colnames.get(idx) );
	selcols_ += colid;
	selcomps_ += compidx;
    }
}


SeisDataPackFromDPS::~SeisDataPackFromDPS()
{}


od_int64 SeisDataPackFromDPS::nrIterations() const
{
    return dps_.size();
}


bool SeisDataPackFromDPS::doPrepare( int nrthreads )
{
    return true;
}


bool SeisDataPackFromDPS::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    const StepInterval<float>& zsamp = dp_.zRange();
    const int nrsamps = zsamp.nrSteps() + 1;
    for ( int compidx=0; compidx<selcomps_.size(); compidx++ )
    {
	Array3DImpl<float>& arr = dp_.data( selcomps_[compidx] );
	float* data = arr.getData();
	if ( !data )
	    continue;

	DataPointSet::ColID colid = selcols_[compidx];
	for ( int idx=mCast(int,start); idx<=stop; idx++ )
	{
	    const TrcKey tkpos( dps_.binID(idx) );
	    const int globidx = dp_.getGlobalIdx( tkpos );
	    if ( globidx < 0 )
		continue;

	    const float zval = dps_.z( idx );
	    data[globidx*nrsamps + zsamp.nearestIndex(zval)] =
		colid == mZColID ? zval : dps_.value( colid, idx );
	}
    }

    return true;
}
