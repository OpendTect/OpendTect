/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiedata.cc,v 1.11 2009-07-17 12:08:23 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "survinfo.h"
#include "sorting.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"

#include "welltiedata.h"
#include "welltied2tmodelmanager.h"
#include "welltiepickset.h"
#include "welltieunitfactors.h"



WellTieDataSetMGR::WellTieDataSetMGR( const WellTieParams::DataParams* pms, 
				      WellTieData* data ) 
    		: params_(*pms)
		, datasets_(data->datasets_)  
{
    for ( int idx=0; idx<data->nrdataset_; idx++ )
	datasets_ += new WellTieDataSet();

    for ( int idx=0; idx<datasets_.size(); idx++ )
	datasets_[idx]->setColNames(params_.colnms_);
}


WellTieDataSetMGR::~WellTieDataSetMGR()
{
    for ( int idx=datasets_.size()-1; idx>=0; idx-- )
	datasets_[idx]->clearData();
}


void WellTieDataSetMGR::resetData()
{
    int size[] = { params_.worksize_, params_.dispsize_, params_.corrsize_, 0 };

    for ( int idx=0; idx<datasets_.size(); idx++ )
	resetData( *datasets_[idx], size[idx] );
}


void WellTieDataSetMGR::resetData( WellTieDataSet& dataset, int size )
{
    dataset.clearData();
    dataset.setColNr( params_.nrdatacols_ );
    dataset.setLength( size );
    dataset.createDataArrays();
}

//resampledata at "step" sampling rate
void WellTieDataSetMGR::rescaleData( const WellTieDataSet& olddata,
				  WellTieDataSet& newdata,	
				  int colnr, int step  )
{
    for ( int colidx=0; colidx<colnr; colidx++)
    {
	for ( int idx=0; idx<newdata.getLength(); idx++)
	{
	    const float val = olddata.get( colidx )->get( (int)step*idx );
	    newdata.get(colidx)->setValue( idx, val );
	}
    }
}


//recompute data between timestart and timestop
void WellTieDataSetMGR::rescaleData( const WellTieDataSet& olddata,
				     WellTieDataSet& newdata, int colnr, 
				     float timestart, float timestop )
{
    int startidx = olddata.getIdx( timestart );
    int stopidx  = olddata.getIdx( timestop  );
    if ( startidx<0 || startidx>=stopidx  )
	return;

    for ( int idx=0; idx<colnr; idx++)
	newdata.setArrayBetweenIdxs( *olddata.get(idx), 
				     *newdata.get(idx), 
				     startidx, stopidx );
}


void WellTieDataSetMGR::getSortedDPSDataAlongZ( const DataPointSet& dps,
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



const int WellTieDataSet::getIdxFromDah( float dah ) const
{
    int idx=0;
    while ( get(0,idx)<dah )
	idx++;
    return idx;	
}


void WellTieDataSet::clearData()
{
    deepErase( data_ );
}


void WellTieDataSet::createDataArrays()
{
    for ( int idx=0; idx<colnr_; idx++)
	data_ += new Array1DImpl<float>( datasz_ );
}



const int WellTieDataSet::getIdx( float time ) const
{
    const float step = SI().zStep();
    const int idx = int ( (time-data_[1]->get(0))/step );
    return idx;
}


const float WellTieDataSet::get( const char* colnm, int idx ) const  
{
    if ( get(colnm) )
	return get(colnm)->get(idx);
    return 0;
}


const float WellTieDataSet::get( int colidx, int validx ) const
{
    if ( get(colidx) )
	return get(colidx)->get(validx);
    return 0;
}


const float WellTieDataSet::getExtremVal( const char* colnm, bool ismax ) const
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


const int WellTieDataSet::getColIdx( const char* colname ) const
{ return colnameset_.indexOf( colname  ); }


void WellTieDataSet::setArrayBetweenIdxs( const Array1DImpl<float>& olddata,
					  Array1DImpl<float>& newdata,
					  int startidx, int stopidx )
{
    const int olddatasz = olddata.info().getSize(0);
    const int newdatasz = newdata.info().getSize(0);
    for ( int idx=0; idx<newdatasz; idx++ )
    {
	float val = 0;
	if ( idx+startidx < olddatasz )
	    val = olddata.get( idx + startidx );
	if ( mIsUdf(val) )
	    newdata.setValue( idx, 0 );
	else
	    newdata.setValue( idx, val ); 
    }
}


void WellTieDataSet::set( const char* colnm, int idx, float val )
{
    if ( get(colnm) )
	get(colnm)->set(idx, val);
}




WellTieDataHolder::WellTieDataHolder( WellTieParams* params, 
				      Well::Data* wd, const WellTieSetup& s )
    	: params_(params)	
	, wd_(wd) 
	, setup_(s)	  
{
    uipms_   = &params_->uipms_;
    dpms_    = &params_->dpms_;
    pickmgr_ = new WellTiePickSetMGR( wd_ );
    d2tmgr_  = new WellTieD2TModelMGR( wd_, params_ );
    datamgr_ = new WellTieDataSetMGR( &params_->dpms_, &data_ );
}


WellTieDataHolder::~WellTieDataHolder()
{
    delete datamgr_;
    delete pickmgr_;
    delete d2tmgr_;
    delete params_;
}
