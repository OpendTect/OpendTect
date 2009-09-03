/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiedata.cc,v 1.16 2009-09-03 14:09:28 cvsbruno Exp $";

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

namespace WellTie
{

DataSetMGR::DataSetMGR( WellTie::DataHolder& dh ) 
    		: params_(*dh.dpms())
		, datasets_(dh.data().datasets_)  
{
    for ( int idx=0; idx<dh.data().nrdataset_; idx++ )
	datasets_ += new WellTie::DataSet();

    for ( int idx=0; idx<datasets_.size(); idx++ )
	datasets_[idx]->setColNames(params_.colnms_);
}


DataSetMGR::~DataSetMGR()
{
    for ( int idx=datasets_.size()-1; idx>=0; idx-- )
	datasets_[idx]->clearData();
    deepErase( datasets_ );
}


void DataSetMGR::resetData()
{
    int size[] = { params_.worksize_, params_.dispsize_, params_.corrsize_, 0 };

    for ( int idx=0; idx<datasets_.size(); idx++ )
	resetData( *datasets_[idx], size[idx] );
}


void DataSetMGR::resetData( WellTie::DataSet& dataset, int size )
{
    dataset.clearData();
    dataset.setColNr( params_.nrdatacols_ );
    dataset.setLength( size );
    dataset.createDataArrays();
}

//resampledata at "step" sampling rate
void DataSetMGR::rescaleData( const WellTie::DataSet& olddata,
			      WellTie::DataSet& newdata,	
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
void DataSetMGR::rescaleData( const WellTie::DataSet& olddata,
			     WellTie::DataSet& newdata, int colnr, 
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


void DataSetMGR::getSortedDPSDataAlongZ( const DataPointSet& dps,
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



const int DataSet::getIdxFromDah( float dah ) const
{
    int idx=0;
    while ( get(0,idx)<dah )
	idx++;
    return idx;	
}


void DataSet::clearData()
{
    deepErase( data_ );
}


void DataSet::createDataArrays()
{
    for ( int idx=0; idx<colnr_; idx++)
	data_ += new Array1DImpl<float>( datasz_ );
}


const int DataSet::getIdx( float time ) const
{
    const float step = SI().zStep();
    const int idx = int ( (time-data_[1]->get(0))/step );
    return idx;
}


const float DataSet::get( const char* colnm, int idx ) const  
{
    if ( get(colnm) )
	return get(colnm)->get(idx);
    return 0;
}


const float DataSet::get( int colidx, int validx ) const
{
    if ( get(colidx) )
	return get(colidx)->get(validx);
    return 0;
}


const float DataSet::getExtremVal( const char* colnm, bool ismax ) const
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


const int DataSet::getColIdx( const char* colname ) const
{ return colnameset_.indexOf( colname  ); }


void DataSet::setArrayBetweenIdxs( const Array1DImpl<float>& olddata,
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


void DataSet::set( const char* colnm, int idx, float val )
{
    if ( get(colnm) )
	get(colnm)->set(idx, val);
}




DataHolder::DataHolder( WellTie::Params* params, Well::Data* wd, 
			const WellTie::Setup& s )
    	: params_(params)	
	, wd_(wd) 
	, setup_(s)
	, factors_(s.unitfactors_) 	   
{
    uipms_   = &params_->uipms_;
    dpms_    = &params_->dpms_;
    data_    = new WellTie::Data();
    pickmgr_ = new WellTie::PickSetMGR( wd_ );
    geocalc_ = new WellTie::GeoCalculator( *this );
    d2tmgr_  = new WellTie::D2TModelMGR( *this );
    datamgr_ = new WellTie::DataSetMGR( *this );
}


DataHolder::~DataHolder()
{
    delete datamgr_;
    delete data_;
    delete pickmgr_;
    delete d2tmgr_;
    delete params_;
}

}; //namespace WellTie
