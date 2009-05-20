/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiedata.cc,v 1.2 2009-05-20 14:27:30 cvsbruno Exp $";

#include "welltiedata.h"
#include "welltieunitfactors.h"
#include "arrayndimpl.h"
#include "survinfo.h"
#include "sorting.h"
#include "posvecdataset.h"
#include "datapointset.h"



WellTieDataMGR::WellTieDataMGR( const WellTieParams* pms )
    		: params_(*pms)
{
    workdata_.setColNames(params_.colnms_);
    dispdata_.setColNames(params_.colnms_);
}


WellTieDataMGR::~WellTieDataMGR()
{
    workdata_.clearData();
    dispdata_.clearData();
}


void WellTieDataMGR::resetData()
{
    resetData( workdata_, params_.worksize_ );
    resetData( dispdata_, params_.dispsize_ );
}


void WellTieDataMGR::resetData( WellTieDataSet& dataset, int size )
{
    dataset.clearData();
    dataset.setColNr( params_.nrdatacols_ );
    dataset.setLength( size );
    dataset.createDataArrays();
}


void WellTieDataMGR::setWork2DispData()
{
    rescaleData( workdata_, dispdata_, 6, params_.step_ );
}


void WellTieDataMGR::rescaleData( const WellTieDataSet& olddata,
				  WellTieDataSet& newdata,	
				  int colnr, int step  )
{
    for ( int colidx=0; colidx<colnr; colidx++)
    {
	for ( int idx=0; idx<params_.dispsize_; idx++)
	{
	    const float val = olddata.get( colidx )->get( (int)step*idx );
	    newdata.get(colidx)->setValue( idx, val );
	}
    }
}


void WellTieDataMGR::getSortedDPSDataAlongZ( const DataPointSet& dps,
       				  	     Array1DImpl<float>& vals )
{
    TypeSet<float> zvals, tmpvals;
    for ( int idx=0; idx<dps.size(); idx++ )
	zvals += dps.z(idx);

    const int sz = zvals.size();
    mAllocVarLenArr( int, zidxs, sz );
    for ( int idx=0; idx<sz; idx++ )
	zidxs[idx] = idx;

    sort_coupled( zvals.arr(), mVarLenArr(zidxs), sz );

    for ( int colidx=0; colidx<dps.nrCols(); colidx++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	    tmpvals += dps.getValues(zidxs[idx])[colidx];
    }

    memcpy(vals.getData(), tmpvals.arr(), vals.info().getSize(0)*sizeof(float));
}




const int WellTieDataSet::findTimeIdx( float dah )
{
    int idx=0;
    while ( get(0,idx)<dah )
	idx++;
    return idx;	
}


void WellTieDataSet::clearData()
{
    for ( int idx=data_.size()-1; idx>=0; idx-- )
	delete ( data_.remove(idx) );
}


void WellTieDataSet::createDataArrays()
{
    for ( int idx=0; idx<colnr_; idx++)
	data_ += new Array1DImpl<float>( datasz_ );
}



const int WellTieDataSet::getIdx( float time )
{
    const float step = SI().zStep();
    const int idx = int ( (time-data_[1]->get(0))/step );
    return idx;
}


const float WellTieDataSet::get( const char* colnm, int idx )
{
    if ( get(colnm) )
	return get(colnm)->get(idx);
    return 0;
}


const float WellTieDataSet::get( int colidx, int validx )
{
    if ( get(colidx) )
	return get(colidx)->get(validx);
    return 0;
}


const float WellTieDataSet::getExtremVal( const char* colnm, bool ismax )
{
    float maxval,             minval;
    maxval = get(colnm, 0);   minval = maxval;

    for ( int idz=0; idz<datasz_; idz++)
    {
	float val =  get(colnm, idz);
	if ( maxval < val && !mIsUdf( val ) )
	    maxval = val;
	if ( minval > val && !mIsUdf(val) )
	    minval = val;
    }
    return ismax? maxval:minval;
}


const int WellTieDataSet::getColIdx( const char* colname )
{
    if ( !colname ) return 0;
    for ( int idx=0; idx<colnameset_.size(); idx++ )
    {
	if ( !strcmp(colname,*colnameset_[idx]) )
	    return idx;
    }
    return 0;
}


