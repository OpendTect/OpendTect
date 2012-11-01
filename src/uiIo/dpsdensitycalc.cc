/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          Mar 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "arrayndimpl.h"
#include "dpsdensitycalc.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "survinfo.h"
#include "unitofmeasure.h"

DPSDensityCalcND::DPSDensityCalcND( const DataPointSet& dps,
				    const ObjectSet<AxisParam>& axisdatas,
       				    ArrayND<float>& freqdata )
    : ParallelTask( "Calclulating Density" )
    , dps_( dps )
    , freqdata_( freqdata )
    , axisdatas_( axisdatas )
    , nrdims_( axisdatas_.size() )
    , nrdone_( 0 )
{
    freqdata_.setAll( (float)0 );
}


od_int64 DPSDensityCalcND::nrIterations() const
{ return dps_.size(); }


bool DPSDensityCalcND::setFreqValue( const int* indexs )
{
    if ( !freqdata_.info().validPos(indexs) )
	return false;

    freqdata_.setND( indexs, freqdata_.getND(indexs) + (float)1 );

    return true;
}


float DPSDensityCalcND::getVal( int dcid, int drid ) const
{
    if ( dcid >= 0 )
    {
	const float val = dps_.value( dcid, drid );
	const UnitOfMeasure* mu = dps_.colDef( dcid ).unit_;
	return mu ? mu->userValue(val) : val;
    }
    else if ( dcid == -1 )
    {
	const float val = dps_.z( drid );
	return val*SI().zDomain().userFactor();
    }

    return dcid == (float) ( -3 ? dps_.coord(drid).x : dps_.coord(drid).y );
}


bool DPSDensityCalcND::getPositions( TypeSet<int>& indexs, int rid )
{
    for ( int idx=0; idx<nrdims_; idx++ )
    {
	AxisParam* axis = axisdatas_[idx];
	const float val = getVal( axis->colid_, rid );
	if ( mIsUdf(val) || !axis->valrange_.includes(val,true) ) return false;
	indexs += axis->valrange_.getIndex( val );
    }

    return true;
}


bool DPSDensityCalcND::doWork( od_int64 start, od_int64 stop, int )
{
    for ( od_int64 rid=start; rid<=stop; rid++ )
    {
	nrdone_++;
	if ( dps_.isInactive(rid) )
	    continue;

	TypeSet<int> indexs;
	
	if ( !getPositions(indexs,mCast(int,rid)) ) continue;

	if ( !setFreqValue(indexs.arr()) ) continue;
    }
    return true;
}
