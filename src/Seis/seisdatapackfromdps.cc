/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		November 2017
________________________________________________________________________

-*/


#include "seisdatapackfromdps.h"

#include "arrayndimpl.h"
#include "bufstringset.h"
#include "datapointset.h"
#include "seisdatapack.h"
#include "trckey.h"

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
	if ( compidx < 0 && !dp_.addComponent(colnames.get(idx), true) )
	    continue;

	compidx = dp_.getComponentIdx( colnames.get(idx) );
	selcols_ += colid;
	selcomps_ += compidx;
    }
}


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
    const auto zsamp = dp_.zRange();
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
	    const auto globidx = dp_.is2D() ? dp_.globalIdx( dps_.bin2D(idx) )
					    : dp_.globalIdx( dps_.binID(idx) );
	    if ( globidx < 0 )
		continue;

	    const float zval = dps_.z( idx );
	    data[globidx*nrsamps + zsamp.getIndex(zval)] =
		colid == mZColID ? zval : dps_.value( colid, idx );
	}
    }

    return true;
}
