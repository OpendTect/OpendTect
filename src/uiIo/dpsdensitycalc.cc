/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          Mar 2010
 RCS:           $Id: dpsdensitycalc.cc,v 1.1 2010-03-03 10:05:56 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "dpsdensitycalc.h"

#include "uidatapointset.h"

#include "arrayndimpl.h"

DPSDensityCalcND::DPSDensityCalcND( const uiDataPointSet& uidps,
				    const ObjectSet<AxisParam>& axisdatas )
    : ParallelTask( "Calclulating Density" )
    , uidps_( uidps )
    , freqdata_( 0 )
    , axisdatas_( axisdatas )
    , nrdims_( axisdatas_.size() )
    , nrdone_( 0 )
{
    ArrayNDInfoImpl arrinfo( nrdims_ );
    for ( int idx=0; idx<nrdims_; idx++ )
	arrinfo.setSize( idx, axisdatas_[idx]->nrbins_ );
    freqdata_ = new ArrayNDImpl<float>( arrinfo );
    freqdata_->setAll( (float)0 );
}


od_int64 DPSDensityCalcND::nrIterations() const
{ return uidps_.pointSet().size(); }


/*void DPSDensityCalcND::setNrBins( const TypeSet<int>& nrbins )
{
    mDynamicCastGet(ArrayNDImpl<float>*,arrimpl,freqdata_)
    arrimpl->setSize( nrbins.arr() );
    for ( int idx=0; idx<nrdims_; idx++ )
	axisdatas_[idx].nrbins_ = nrbins[idx];
    freqdata_->setAll( (float)0 );
}*/


bool DPSDensityCalcND::setFreqValue( const int* indexs )
{
    if ( !freqdata_->info().validPos(indexs) )
	return false;
    freqdata_->setND( indexs, freqdata_->getND(indexs) + (float)1 );

    return true;
}


bool DPSDensityCalcND::getPositions( TypeSet<int>& indexs, int rid )
{
    for ( int idx=0; idx<nrdims_; idx++ )
    {
	AxisParam* axis = axisdatas_[idx];
	const float val = uidps_.getValue( axis->colid_, rid, true );
	if ( mIsUdf(val) || !axis->valrange_.includes(val) ) return false;
	indexs += axis->valrange_.getIndex( val ) + 1;
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


void DPSDensityCalcND::getFreqData( ArrayND<float>& freqdata ) const
{
    mDynamicCastGet(ArrayNDImpl<float>*,freqdataimpl,&freqdata)
    if ( !freqdataimpl || (freqdata.info().getNDim() != nrdims_) ) return;

    TypeSet<int> sizes;
    for ( int idx=0; idx<nrdims_; idx++ )
	sizes += freqdata_->info().getSize(idx);
    freqdataimpl->setSize( sizes.arr() );

    freqdata.setStorage( freqdata_->getStorage() );
}
