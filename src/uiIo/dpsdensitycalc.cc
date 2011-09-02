/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          Mar 2010
 RCS:           $Id: dpsdensitycalc.cc,v 1.4 2011-09-02 13:04:02 cvskris Exp $
________________________________________________________________________

-*/

#include "dpsdensitycalc.h"
#include "uidatapointset.h"
#include "arrayndimpl.h"

DPSDensityCalcND::DPSDensityCalcND( const uiDataPointSet& uidps,
				    const ObjectSet<AxisParam>& axisdatas,
       				    ArrayND<float>& freqdata )
    : ParallelTask( "Calclulating Density" )
    , uidps_( uidps )
    , freqdata_( freqdata )
    , axisdatas_( axisdatas )
    , nrdims_( axisdatas_.size() )
    , nrdone_( 0 )
{
    freqdata_.setAll( (float)0 );
}


od_int64 DPSDensityCalcND::nrIterations() const
{ return uidps_.pointSet().size(); }


bool DPSDensityCalcND::setFreqValue( const int* indexs )
{
    if ( !freqdata_.info().validPos(indexs) )
	return false;

    freqdata_.setND( indexs, freqdata_.getND(indexs) + (float)1 );

    return true;
}


bool DPSDensityCalcND::getPositions( TypeSet<int>& indexs, int rid )
{
    for ( int idx=0; idx<nrdims_; idx++ )
    {
	AxisParam* axis = axisdatas_[idx];
	const float val = uidps_.getValue( axis->colid_, rid, true );
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
	if ( uidps_.pointSet().isInactive(rid) )
	    continue;

	TypeSet<int> indexs;
	
	if ( !getPositions(indexs,rid) ) continue;

	if ( !setFreqValue(indexs.arr()) ) continue;
    }
    return true;
}
