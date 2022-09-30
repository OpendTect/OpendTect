/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arrayndimpl.h"
#include "dpsdensitycalc.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "survinfo.h"
#include "unitofmeasure.h"

mDefineEnumUtils(DPSDensityCalcND,CalcAreaType,"Calculation Area Type")
{
    "Whole region",
    "Selected region",
    "Non Selected region",
    0
};


DPSDensityCalcND::DPSDensityCalcND( const DataPointSet& dps,
				    const ObjectSet<AxisParam>& axisdatas,
				    ArrayND<float>& freqdata,
				    CalcAreaType areatype )
    : ParallelTask( "Calculating Density" )
    , dps_( dps )
    , freqdata_( freqdata )
    , axisdatas_( axisdatas )
    , nrdims_( axisdatas_.size() )
    , nrdone_( 0 )
    , grp_( 0 )
    , areatype_( areatype )
{
    freqdata_.setAll( (float)0 );
}


DPSDensityCalcND::~DPSDensityCalcND()
{}


uiString DPSDensityCalcND::uiNrDoneText() const
{
    return tr("Points done");
}


od_int64 DPSDensityCalcND::nrIterations() const
{
    return dps_.size();
}


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
	return dps_.value( dcid, drid );
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
	DataPointSet::RowID dpsrid = mCast(DataPointSet::RowID,rid);
	if ( dps_.isInactive(dpsrid) ||
	     (grp_>0 && dps_.group(dpsrid) != grp_) )
	    continue;

	const bool isselected = dps_.isSelected( dpsrid );
	if ( (areatype_==DPSDensityCalcND::Selected && !isselected) ||
	     (areatype_==DPSDensityCalcND::NonSelected && isselected) )
	    continue;

	TypeSet<int> indexs;
	if ( !getPositions(indexs,dpsrid) )
	    continue;

	setFreqValue( indexs.arr() );
    }

    return true;
}
