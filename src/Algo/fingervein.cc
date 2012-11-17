/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bo Zhang/Yuancheng Liu
 * DATE     : July 2012
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "fingervein.h"

#include "arrayndimpl.h"
#include "conncomponents.h"
#include "convolve2d.h"
#include "executor.h"
#include "math2.h"
#include "pca.h"
#include "task.h"

#include "statruncalc.h"
#include "stattype.h"
#include  <iostream>

#define mNrThinning 80	

class VeinSliceCalculator : public ParallelTask
{
public:
VeinSliceCalculator( const Array3D<float>& input, float threshold, bool above,
	int minfltlength, int sigma, float percent, Array3D<bool>& output )
    : input_(input)
    , threshold_(threshold)
    , isabove_(above)
    , minfaultlength_(minfltlength)  
    , sigma_(sigma)
    , percent_(percent)
    , output_(output)
{}

protected:

const char* message() const	{ return "Vein calculation on slices."; }
od_int64 nrIterations() const	{ return input_.info().getSize(2); }
bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int isz = input_.info().getSize(0);
    const int csz = input_.info().getSize(1);
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, attr,
	                Array2DImpl<float> (isz,csz) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_bina,
	                Array2DImpl<bool> (isz,csz) );
    const bool is_t_slic = true;

    for ( int idz= mCast(int,start); idz<=stop && shouldContinue(); idz++ )
    {
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
		attr->set( idx, idy, input_.get(idx,idy,idz) );
	}

	FaultOrientation::compute2DVeinBinary( *attr, threshold_, isabove_,
		minfaultlength_, sigma_, percent_, is_t_slic, *vein_bina, 0 );
	
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
		output_.set( idx, idy, idz, vein_bina->get(idx,idy) );
	}	

	addToNrDone(1);
    }

    return true;
}

const Array3D<float>&	input_;
Array3D<bool>&		output_;
float			threshold_;
bool			isabove_;
int 			sigma_; 
float			percent_;
int			minfaultlength_;
};

class AzimuthPcaCalculator : public ParallelTask
{
public:
AzimuthPcaCalculator( const Array3D<bool>& conf_base,
	const Array3D<bool>& conf_upgr, int elem_leng, float null_val,
	Array3D<float>& azimuth_pca )
    : conf_base_(conf_base)
    , conf_upgr_(conf_upgr)  
    , elem_leng_(elem_leng)
    , null_val_(null_val)
    , azimuth_pca_(azimuth_pca)
{ 
    azimuth_pca.setAll( null_val ); 
}

protected:

const char* message() const	{ return "Azimuth PCA calculation on slices."; }
od_int64 nrIterations() const	{ return conf_base_.info().getSize(2); }

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int isz = conf_base_.info().getSize(0);
    const int csz = conf_base_.info().getSize(1);
    
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, base_bina_sect,
	    Array2DImpl<bool> (isz,csz) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, upgr_bina_sect,
	    Array2DImpl<bool> (isz,csz) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, azimuth_sect,
	    Array2DImpl<float> (isz,csz) );

    for ( int idz= mCast(int,start); idz<=stop && shouldContinue(); idz++ )
    {
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
	    {
		base_bina_sect->set( idx, idy, conf_base_.get(idx,idy,idz) );
		upgr_bina_sect->set( idx, idy, conf_upgr_.get(idx,idy,idz) );
	    }
	}

	FaultOrientation::computeComponentAngle( *base_bina_sect, 
		*upgr_bina_sect, elem_leng_, null_val_, *azimuth_sect );
	
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
	    {
		azimuth_pca_.set( idx, idy, idz, azimuth_sect->get(idx,idy) );
	    }
	}

	addToNrDone(1);
    }
    return true;
}

const Array3D<bool>&	conf_base_;
const Array3D<bool>&	conf_upgr_;
int			elem_leng_;
float			null_val_;
Array3D<float>&		azimuth_pca_;
};


FingerVein::FingerVein( const Array2D<float>& input, float threshold, 
	bool isabove, bool istimeslice, Array2D<bool>& output )
    : input_(input)
    , output_(output)
    , threshold_(threshold)  
    , isabove_(isabove)  
    , istimeslice_(istimeslice)  
{ 
}


bool FingerVein::compute( bool domerge, bool dothinning, 
	int minfltlength, float overlaprate, int sigma, float thresholdpercent,
	TaskRunner* tr )
{
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, score,
	    Array2DImpl<float> (input_.info()) );
    if ( !score ) return false;

    if ( !FaultOrientation::computeMaxCurvature( input_, sigma, istimeslice_,
		*score, tr ) )
	return false;

    const int datasz = (int) input_.info().getTotalSz();
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, tmparr,
	    Array2DImpl<float> (input_.info()) );
    if ( !tmparr ) return false;
    tmparr->copyFrom( *score );
    float* arr = tmparr->getData();
    sort_array(arr,datasz);
    const od_int64 thresholdidx = (od_int64)(thresholdpercent*datasz);
    const float score_threshold = arr[0]<arr[datasz-1] ? 
	arr[thresholdidx] : arr[datasz-1-thresholdidx] ;
    
    /*Stats::CalcSetup scs;
    scs.require(Stats::Median);
    Stats::RunCalc<float> rc( scs );
    for ( od_int64 idx=0; idx<datasz; idx++ )
    {
	if ( !mIsUdf(arr[idx]) && arr[idx]>0 )
	    rc.addValue( arr[idx]);
    }
    const float md_score = rc.median(); //use for added condition, not now*/

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, score_binary,
	    Array2DImpl<bool> (input_.info()) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, input_hard_threshold,
	    Array2DImpl<bool> (input_.info()) );
    if ( !score_binary || !input_hard_threshold ) 
	return false;

    const float* inputarr = input_.getData();
    const float* scoredata = score->getData();
    bool* scorebinarydata = score_binary->getData();
    bool* inputbinary = input_hard_threshold->getData();
    for ( od_int64 idx=0; idx<datasz; idx++ )
    {
	if ( mIsUdf(inputarr[idx]) )
	{
	    scorebinarydata[idx] = inputbinary[idx] = false;
	}
	else
	{
    	    scorebinarydata[idx] = scoredata[idx]>score_threshold;
    	    inputbinary[idx] = isabove_ ? inputarr[idx]>threshold_ 
					: inputarr[idx]<threshold_;
	}
    }

    if ( domerge )
    {
	if ( dothinning )
    	    thinning( *input_hard_threshold );
	removeSmallComponents(*input_hard_threshold, minfltlength, overlaprate);
    }

    const bool* thinedinputarr = input_hard_threshold->getData();
    bool* mergedata = output_.getData();
    for ( od_int64 idx=0; idx<datasz; idx++ )
    {
	if ( domerge && !scorebinarydata[idx] && thinedinputarr[idx] ) 
	    //&& scoredata[idx]>md_score )
	    mergedata[idx] = 1;
	else
	    mergedata[idx] = scorebinarydata[idx];
    }

    if ( dothinning )
    	thinning( output_ );
    removeSmallComponents( output_, minfltlength, overlaprate, true );
    
    return true;
}


void FingerVein::removeSmallComponents( Array2D<bool>& data, int minfltlength, 
	float overlaprate, bool savecomps )
{
    ConnComponents cc( data );
    cc.compute();

    if ( savecomps )
    {
	validconncomps_.erase();
	nrcomps_.erase();
	compids_.erase();
    }

    const int nrcomps = cc.nrComponents();
    bool* outputarr = data.getData();
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	const TypeSet<int>* comp = cc.getComponent( idx );
	if ( !comp ) continue;

	const int nrnodes = comp->size();
	if ( nrnodes<minfltlength || cc.overLapRate(idx)>overlaprate )
	{
	    for ( int idy=0; idy<nrnodes; idy++ )
    		outputarr[(*comp)[idy]] = 0;
	}
	else if ( savecomps && (*comp)[0] )
	{
	    nrcomps_ += nrnodes;
	    for ( int idy=0; idy<nrnodes; idy++ )
    		compids_ += (*comp)[idy];

	    validconncomps_ += TypeSet<int>(*comp);
	}
    }
}


void FingerVein::thinning( Array2D<bool>& res, bool useskeleton )
{
    if ( useskeleton )
    {    
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<char> >, tmp,
		Array2DImpl<char> (res.info()) );
	if ( !tmp ) return;

	bool* inp = res.getData();
	char* data = tmp->getData();
	const int sz = mCast( int, res.info().getTotalSz() );
	for ( int idx=0; idx<sz; idx++ )
	    data[idx] = inp[idx] ? 1 : 0;

	FaultOrientation::skeletonHilditch( *tmp );
	for ( int idx=0; idx<sz; idx++ )
	    inp[idx] = data[idx]>0;
    }
    else
    {
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, tmp,
		Array2DImpl<bool> (res.info()) );
	if ( !tmp ) return;
	tmp->copyFrom( res );
	FaultOrientation::thinning( *tmp, res );
    }
}


#define mNull_val -5


FaultOrientation::FaultOrientation()
    : threshold_(0)
    , isfltabove_(false)  
    , minfaultlength_(10)
    , sigma_(3)
    , percent_(0.94)
    , azimuth_stable_(0)
    , dip_stable_(0)
    , conf_low_(0)
    , conf_med_(0)
    , conf_high_(0) 
{} 


FaultOrientation::~FaultOrientation()
{
    cleanAll();
}


void FaultOrientation::cleanAll()
{
    delete azimuth_stable_; azimuth_stable_ = 0;
    delete dip_stable_;	dip_stable_ = 0;
    delete conf_low_; conf_low_ = 0;
    delete conf_med_; conf_med_ = 0;
    delete conf_high_; conf_high_ = 0;
}


void FaultOrientation::setParameters( int sigma, float vein_percentage )
{
    if ( sigma>0 )
	sigma_ = sigma;

    if ( vein_percentage<0 || vein_percentage>1 )
	return;

    percent_ = vein_percentage;
}


void FaultOrientation::setThreshold( float threshold, bool isabove )
{
    threshold_ = threshold;
    isfltabove_ = isabove;
}


void FaultOrientation::setMinFaultLength( int minlenght )
{ minfaultlength_ = minlenght; }


const Array3D<bool>* FaultOrientation::getFaultConfidence( ConfidenceLevel cl )
{ return cl==Low ? conf_low_ : (cl==High ? conf_high_ : conf_med_ ); }


bool FaultOrientation::compute( const Array3D<float>& input, bool forazimuth,
	bool fordip, TaskRunner* tr )
{
    cleanAll();
    const int isz = input.info().getSize(0);
    const int csz = input.info().getSize(1);
    const int zsz = input.info().getSize(2);
    if ( forazimuth )
    	azimuth_stable_ = new Array3DImpl<float>( isz, csz, zsz );
    if ( fordip )
    	dip_stable_ = new Array3DImpl<float>( isz, csz, zsz );
    conf_low_ = new Array3DImpl<bool>( isz, csz, zsz );
    conf_med_ = new Array3DImpl<bool>( isz, csz, zsz );
    conf_high_ = new Array3DImpl<bool>( isz, csz, zsz );
    conf_low_->setAll( false );
    conf_med_->setAll( false );
    conf_high_->setAll( false );

    if ( isz==1 || csz==1 || zsz==1 )
	return compute2D( input, tr );

    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, vein_bina_t0,
	    Array3DImpl<bool> (isz, csz, zsz) );
    computeVeinSlices( input, *vein_bina_t0, tr );

    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, vein_bina_0,
	    Array3DImpl<bool> (isz, csz, zsz) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, vein_bina_45,
	    Array3DImpl<bool> (isz, csz, zsz) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, vein_bina_90,
	    Array3DImpl<bool> (isz, csz, zsz) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, vein_bina_135,
	    Array3DImpl<bool> (isz, csz, zsz) );
    computeVerticalVeinSlice( input, *vein_bina_0, *vein_bina_45, 
	    *vein_bina_90, *vein_bina_135 );

    setFaultConfidence( *vein_bina_t0, *vein_bina_0, *vein_bina_45, 
	    *vein_bina_90, *vein_bina_135, *conf_low_, *conf_med_, *conf_high_);
    
    if ( !forazimuth )
	return true;

    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, azimuth_pca,
	    Array3DImpl<float> (isz, csz, zsz) );
    computeAzimuthPCA( *conf_low_, *conf_med_, minfaultlength_, mNull_val, 
	    *azimuth_pca, tr );
    stabilizeAzimuth( *conf_low_, *azimuth_pca, minfaultlength_, mNull_val, 
	    *azimuth_stable_ );

    if ( fordip )
    {
	mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, dip_pca,
		Array3DImpl<float> (isz, csz, zsz) );
	const int wind_size = 4*sigma_;
	computeDipPCA( *conf_low_, *conf_med_, *azimuth_stable_, wind_size,
		minfaultlength_, mNull_val, *dip_pca );
	stablizeDip( *conf_low_, *azimuth_stable_, *dip_pca, wind_size,
		minfaultlength_, mNull_val, *dip_stable_ );
    }

    return true;
}


bool FaultOrientation::compute2D( const Array3D<float>& input, TaskRunner* tr )
{
    const int isz = input.info().getSize(0);
    const int csz = input.info().getSize(1);
    const int zsz = input.info().getSize(2);
    const bool isinl = isz==1;
    const bool is_t_slic = zsz==1;
    const int xsz = isinl ? csz : isz;
    const int ysz = is_t_slic ? csz : zsz;

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, attr_sect,
	                Array2DImpl<float> (xsz,ysz) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_bina,
	                Array2DImpl<bool> (xsz,ysz) );
    for ( int idx=0; idx<xsz; idx++ )
    {
	for ( int idy=0; idy<ysz; idy++ )
	{
	    float val = isinl ? input.get(0,idx,idy) : (is_t_slic ? 
		    input.get(idx,idy,0) : input.get(idx,0,idy) );
	    attr_sect->set( idx, idy, val );
	}
    }
    compute2DVeinBinary( *attr_sect, threshold_, isfltabove_, minfaultlength_, 
	    sigma_, percent_, is_t_slic, *vein_bina, tr );
    
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, azimuth_pca_stab,
	    Array2DImpl<float> (xsz,ysz) );
    const bool doazimuth = azimuth_stable_;
    if ( doazimuth )
    {
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, azimuth_pca,
		Array2DImpl<float> (xsz,ysz) );
	azimuth_pca->setAll( mNull_val );
	computeComponentAngle( *vein_bina, *vein_bina, minfaultlength_, 
		mNull_val, *azimuth_pca );

    	const int elem_leng_new = minfaultlength_*4;
    	const float uppr_perc = 0.85;
    	const float lowr_perc = 0.15;
    	const float angl_tole = 10;
    	stabilizeAngleSection( *vein_bina, *azimuth_pca, elem_leng_new, 
		uppr_perc, lowr_perc, angl_tole, mNull_val, *azimuth_pca_stab);
    }

    if ( isinl )
    {
	for ( int i=0; i<xsz; i++ )
	{
	    for ( int j=0; j<ysz; j++ )
	    {
		if ( doazimuth )
    		    azimuth_stable_->set( 0, i, j, azimuth_pca_stab->get(i,j) );
		conf_low_->set( 0, i, j, vein_bina->get(i,j) );
		conf_med_->set( 0, i, j, vein_bina->get(i,j) );
		conf_high_->set( 0, i, j, vein_bina->get(i,j) );
	    }
	}
    }
    else if ( is_t_slic ) 
    {
	for ( int i=0; i<xsz; i++ )
	{
	    for ( int j=0; j<ysz; j++ )
	    {
		if ( doazimuth )
    		    azimuth_stable_->set( i, j, 0, azimuth_pca_stab->get(i,j) );
		conf_low_->set( i, j, 0, vein_bina->get(i,j) );
		conf_med_->set( i, j, 0, vein_bina->get(i,j) );
		conf_high_->set( i, j, 0, vein_bina->get(i,j) );
	    }
	}
    }
    else 
    {
	for ( int i=0; i<xsz; i++ )
	{
	    for ( int j=0; j<ysz; j++ )
	    {
		if ( doazimuth )
    		    azimuth_stable_->set( i, 0, j, azimuth_pca_stab->get(i,j) );
		conf_low_->set( i, 0, j, vein_bina->get(i,j) );
		conf_med_->set( i, 0, j, vein_bina->get(i,j) );
		conf_high_->set( i ,0, j, vein_bina->get(i,j) );
	    }
	}
    }

    return true;
}


void FaultOrientation::stablizeDip( const Array3D<bool>& conf_bina,
	const Array3D<float>& azimuth_pca, const Array3D<float>& dip_pca,
	int wind_size, int elem_leng, float null_val, 
	Array3D<float>& dip_pca_stab )
{
    const int nwidth = conf_bina.info().getSize(0);
    const int nhight = conf_bina.info().getSize(1);
    const int nt = conf_bina.info().getSize(2);
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, pre4_dip_pca0,
	    Array3DImpl<float>(nwidth,nhight,nt) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, pre4_dip_pca1,
	    Array3DImpl<float>(nwidth,nhight,nt) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, pre4_dip_pca2,
	    Array3DImpl<float>(nwidth,nhight,nt) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, pre4_dip_pca3,
	    Array3DImpl<float>(nwidth,nhight,nt) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, odd_flag,
	    Array3DImpl<bool>(nwidth,nhight,nt) );
    odd_flag->setAll( 0 );
    pre4_dip_pca0->setAll( null_val );
    pre4_dip_pca1->setAll( null_val );
    pre4_dip_pca2->setAll( null_val );
    pre4_dip_pca3->setAll( null_val );

    const int elem_leng_new = elem_leng*4;
    const float uppr_perc = 0.85;
    const float lower_perc = 0.15;
    const float angle_tole = 10;
    
    /*pre-dip calculation along width direction(inline-), angle zone 1*/
    for ( int jhight=0; jhight<nhight; jhight++ )
    {
	/*collect the attribute value of current section*/
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, base_bina_sect,
	    Array2DImpl<bool> (nwidth,nt) );
    	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect,
	    Array2DImpl<float> (nwidth,nt) );
    	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect_stab,
	    Array2DImpl<float> (nwidth,nt) );
	for ( int jwidth=0; jwidth<nwidth; jwidth++ )
	{
	    for ( int jn=0; jn<nt; jn++ )
	    {
		base_bina_sect->set(jwidth,jn, conf_bina.get(jwidth,jhight,jn));
		dip_sect->set( jwidth, jn, dip_pca.get(jwidth,jhight,jn) );
	    }
	}

	stabilizeAngleSection( *base_bina_sect, *dip_sect, elem_leng_new,
	    uppr_perc, lower_perc, angle_tole, null_val, *dip_sect_stab );
	
        /*map the dip value from dip_sect to pre4_dip_pca*/
	for ( int kwidth=0; kwidth<nwidth; kwidth++ )
	{
	    for ( int kt=0; kt<nt; kt++ )
	    {
		pre4_dip_pca0->set( kwidth, jhight, kt,
		    dip_sect_stab->get(kwidth,kt) );
		if ( !mIsEqual(dip_sect->get(kwidth,kt),
			      dip_sect_stab->get(kwidth,kt), 1e-3) )
		    odd_flag->set( kwidth, jhight, kt, 1 );
	    }
	}
     }

    /*pre-dip calculation along width direction(xline-), angle zone 3*/
    for ( int jwidth=0; jwidth<nwidth; jwidth++ )
    {
	/*collect the attribute value of current section*/
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, base_bina_sect,
	    Array2DImpl<bool> (nhight,nt) );
    	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect,
	    Array2DImpl<float> (nhight,nt) );
    	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect_stab,
	    Array2DImpl<float> (nhight,nt) );
	for ( int jhight=0; jhight<nhight; jhight++ )
	{
	    for ( int jn=0; jn<nt; jn++ )
	    {
		base_bina_sect->set(jhight,jn, conf_bina.get(jwidth,jhight,jn));
		dip_sect->set( jhight, jn, dip_pca.get(jwidth,jhight,jn) );
	    }
	}
	
	stabilizeAngleSection( *base_bina_sect, *dip_sect, elem_leng_new,
	    uppr_perc, lower_perc, angle_tole, null_val, *dip_sect_stab );
        /*map the dip value from dip_sect to pre4_dip_pca*/
	for ( int khight=0; khight<nhight; khight++ )
	{
	    for ( int kt=0; kt<nt; kt++ )
	    {
		pre4_dip_pca2->set( jwidth, khight, kt,
		    dip_sect_stab->get(khight,kt) );
		if ( !mIsEqual(dip_sect->get(khight,kt),
			       dip_sect_stab->get(khight,kt),1e-3) )
		    odd_flag->set( jwidth, khight, kt, 1 );
	    }
	}
     }

    /*pre-dip calculation along width direction(3pi/4,/), angle zone 2*/
    const int nsection = nwidth+nhight-1;
    for ( int jsec=0; jsec<nsection; jsec++ )
    {
	int jhight_start, jwidth_start, nwidth_sect;
	/*the section located in the left lower part*/
	if ( jsec<nhight ) 
	{
	    /*get the width of current section*/
	    jhight_start = nhight-jsec-1;
	    jwidth_start = 0;
	    nwidth_sect = 0;
	    int kwidth = jwidth_start;
	    for ( int khight=jhight_start; khight<nhight; khight++ )
	    {
		if ( kwidth>=nwidth )
		    break;

		kwidth++;
    		nwidth_sect++;
	    }
	}
	else /*the section located in the right upper part*/
	{
	    /*get the width of current section*/
	    jhight_start = 0;
	    jwidth_start = jsec-nhight;
	    nwidth_sect = 0;
	    int khight = jhight_start;
	    for ( int kwidth=jwidth_start; kwidth<nwidth; kwidth++ )
	    {
		if ( khight>=nhight ) break;

		khight++;
		nwidth_sect++;
	    }
	}

	/*check the section width*/
	if  ( nwidth_sect<wind_size )
	    continue;
	
	/*collect the attribue value of current section */
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, base_bina_sect,
		Array2DImpl<bool> (nwidth_sect,nt) );
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect,
		Array2DImpl<float> (nwidth_sect,nt) );
	TypeSet<int> width_index_map;
	TypeSet<int> hight_index_map;
	int jwidth = jwidth_start;
	int khight = jhight_start;
	for ( int kwidth_sect=0; kwidth_sect<nwidth_sect; kwidth_sect++ )
	{
	    width_index_map += jwidth;
	    hight_index_map += khight;
	    for ( int k=0; k<nt; k++ )
	    {
		base_bina_sect->set( kwidth_sect, k, 
				conf_bina.get(jwidth,khight,k) );
		dip_sect->set( kwidth_sect, k, dip_pca.get(jwidth,khight,k) );
	    }
	    jwidth = jwidth+1;
	    khight = khight+1;
	}
     
        /*get the component index and component length*/
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect_stab,
		Array2DImpl<float> (nwidth_sect,nt) );
	stabilizeAngleSection( *base_bina_sect, *dip_sect, elem_leng_new,
	    uppr_perc, lower_perc, angle_tole, null_val, *dip_sect_stab );
        /*map the dip value from dip_sect to pre4_dip_pca*/
	for ( int kwidth=0; kwidth<nwidth_sect; kwidth++ )
	{
	    int iwidth = width_index_map[kwidth];
	    int ihight = hight_index_map[kwidth];
	    for ( int kt=0; kt<nt; kt++ )
	    {
		pre4_dip_pca1->set( iwidth, ihight, kt,
			dip_sect_stab->get(kwidth,kt) );
		if ( !mIsEqual(dip_sect->get(kwidth,kt),
			      dip_sect_stab->get(kwidth,kt), 1e-3) )
		odd_flag->set( iwidth, ihight, kt, 1 );
	    }
	}
    }

    /*pre-dip calculation along width direction(pi/4-,\), angle zone 4*/
    for ( int jsec=0; jsec<nsection; jsec++ )
    {
	int jhight_start, jwidth_start, nwidth_sect;
	/*the section located in the upper left part*/
	if ( jsec<nhight ) 
	{
	    /*get the width of current section*/
	    jhight_start = jsec;
	    jwidth_start = 0;
	    nwidth_sect = 0;
	    int kwidth = jwidth_start;
	    for ( int khight=jhight_start; khight>=0; khight-- )
	    {
		if ( kwidth>=nwidth || khight<0 )
		    break;

		kwidth++;
    		nwidth_sect++;
	    }
	}
	else /*the section located in the lower right part*/
	{
	    /*get the width of current section*/
	    jhight_start = nhight-1;
	    jwidth_start = jsec-nhight;
	    nwidth_sect = 0;
	    int khight = jhight_start;
	    for ( int kwidth=jwidth_start; kwidth<nwidth; kwidth++ )
	    {
		if ( khight<0 ) break;

		khight--;
		nwidth_sect++;
	    }
	}

	/*check the section width*/
	if  ( nwidth_sect<wind_size )
	    continue;
	
	/*collect the attribue value of current section */
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, base_bina_sect,
		Array2DImpl<bool> (nwidth_sect,nt) );
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect,
		Array2DImpl<float> (nwidth_sect,nt) );
	TypeSet<int> width_index_map;
	TypeSet<int> hight_index_map;
	int jwidth = jwidth_start;
	int khight = jhight_start;
	for ( int kwidth_sect=0; kwidth_sect<nwidth_sect; kwidth_sect++ )
	{
	    width_index_map += jwidth;
	    hight_index_map += khight;
	    for ( int k=0; k<nt; k++ )
	    {
		base_bina_sect->set( kwidth_sect, k, 
				conf_bina.get(jwidth,khight,k) );
		dip_sect->set( kwidth_sect, k, dip_pca.get(jwidth,khight,k) );
	    }
	    jwidth = jwidth+1;
	    khight = khight-1;
	}
     
        /*stablise the dip*/
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect_stab,
		Array2DImpl<float> (nwidth_sect,nt) );
	stabilizeAngleSection( *base_bina_sect, *dip_sect, elem_leng_new,
	    uppr_perc, lower_perc, angle_tole, null_val, *dip_sect_stab );
        /*map the dip value from dip_sect to pre4_dip_pca*/
	for ( int kwidth=0; kwidth<nwidth_sect; kwidth++ )
	{
	    int iwidth = width_index_map[kwidth];
	    int ihight = hight_index_map[kwidth];
	    for ( int kt=0; kt<nt; kt++ )
	    {
		pre4_dip_pca3->set( iwidth, ihight, kt,
			dip_sect_stab->get(kwidth,kt) );
		if ( !mIsEqual(dip_sect->get(kwidth,kt),
			      dip_sect_stab->get(kwidth,kt), 1e-3) )
		odd_flag->set( iwidth, ihight, kt, 1 );
	    }
	}
    }

    /*Get the dip based on current azimuth value*/
    /*Pre-generate angle and arc set*/
    TypeSet<float> arc_set, angle_set;
    for ( int idx=0; idx<4; idx++ )
    {
	arc_set += idx*M_PI_4f;
	angle_set += idx*45.f;
    }

    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, azimuth_pca_arc,
		Array3DImpl<float> (azimuth_pca.info()) );
    const float angle2arc = M_PI/180;
    const float arc2angle = 180/M_PI;
    for ( int idx=0; idx<nwidth; idx++ )
    {
	for ( int idy=0; idy<nhight; idy++ )
	{
	    for ( int idz=0; idz<nt; idz++ )
	    {
		azimuth_pca_arc->set( idx, idy, idz,
				angle2arc*azimuth_pca.get(idx,idy,idz) );
	    }
	}
    }

    for ( int jwidth=0; jwidth<nwidth; jwidth++ )
    {
	for ( int jhight=0; jhight<nhight; jhight++ )
	{
	    for ( int jt=0; jt<nt; jt++ )
	    {
		if ( !odd_flag->get(jwidth,jhight,jt) )
		{
		    dip_pca_stab.set( jwidth, jhight, jt, 
			dip_pca.get(jwidth,jhight,jt) );
		    continue;
		}

		/*Get the two vertical sections*/
		const float azim_curr = azimuth_pca.get(jwidth,jhight,jt);
		float phi_angle[4];
		if ( azim_curr>90 )
		{
		    phi_angle[0] = 180-azim_curr;
		    phi_angle[1] = azim_curr-45;
		    phi_angle[2] = azim_curr-90;
		    phi_angle[3] = fabs(135-azim_curr);
		}
		else
		{
		    phi_angle[0] = azim_curr;
		    phi_angle[1] = fabs(azim_curr-45);
		    phi_angle[2] = 90-azim_curr;
		    phi_angle[3] = 135-azim_curr;
		}

		TypeSet<float> angle_diff_sort;
		TypeSet<int> asort_indx;
		for ( int idx=0; idx<4; idx++ )
		{
		    if ( phi_angle[idx]>90 )
			phi_angle[idx] = 180-phi_angle[idx];
		    asort_indx += idx;
		    angle_diff_sort += phi_angle[idx];
		}
		sort_coupled( angle_diff_sort.arr(), asort_indx.arr(), 4 );

		int win_angle_indx[2];
		for ( int ja=0; ja<2; ja++ )
		{
		    const int aindx = asort_indx[ja+2];	
		    win_angle_indx[ja] = aindx;
		}

		/*Map the dip of vertical to true dip*/
		int null_count = 0;
		int null_flag[] = {1,1};
		float theta1_angl[] = {0,0};
		for ( int ja=0; ja<2; ja++ )
		{
		    const int aindx = win_angle_indx[ja];
		    float theta_angle;
		    if ( aindx==0 )
		    	theta_angle =pre4_dip_pca0->get(jwidth,jhight,jt); 
		    else if ( aindx==1 )
		    	theta_angle =pre4_dip_pca1->get(jwidth,jhight,jt); 
		    else if ( aindx==2 )
		    	theta_angle =pre4_dip_pca2->get(jwidth,jhight,jt); 
		    else 
		    	theta_angle =pre4_dip_pca3->get(jwidth,jhight,jt);

		    if ( fabs(theta_angle-null_val)<0.5 )
		    {
			null_flag[ja] = 0;
			null_count++;
			theta1_angl[ja] = 0;
			continue;
		    }
		    
		    float theta2_arc = theta_angle * angle2arc;
		    float para_a2 = cos( theta2_arc );
    		    float para_b = sin( theta2_arc );
    		    float para_a1 = para_a2 * 
			    sin( phi_angle[aindx] * angle2arc );
    		    float theta1_arc = atan( para_b/para_a1 );
    		    theta1_angl[ja] = theta1_arc*arc2angle;
		    if ( theta1_angl[ja]<0 )
			theta1_angl[ja] = 180 + theta1_angl[ja];
		}

		float aver_dip = null_val;
		if ( null_count<2 )
		{
		    aver_dip = theta1_angl[0] * null_flag[0] +
			       theta1_angl[1] * null_flag[1];
    		    aver_dip = aver_dip/(2-null_count);
		}
		dip_pca_stab.set( jwidth, jhight, jt, aver_dip );
	    }
	}
    }
}

void FaultOrientation::computeDipPCA( const Array3D<bool>& conf_base,
	const Array3D<bool>& conf_upgr, const Array3D<float>&  azimuth_pca,
	int wind_size, int elem_leng, float null_val, Array3D<float>& dip_pca )
{
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, pre4_dip_pca0,
	    Array3DImpl<float> (dip_pca.info()) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, pre4_dip_pca1,
	    Array3DImpl<float> (dip_pca.info()) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, pre4_dip_pca2,
	    Array3DImpl<float> (dip_pca.info()) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, pre4_dip_pca3,
	    Array3DImpl<float> (dip_pca.info()) );
    pre4_dip_pca0->setAll( null_val );
    pre4_dip_pca1->setAll( null_val );
    pre4_dip_pca2->setAll( null_val );
    pre4_dip_pca3->setAll( null_val );

    const int nwidth = conf_base.info().getSize(0);
    const int nhight = conf_base.info().getSize(1);
    const int nt = conf_base.info().getSize(2);
    
    /*pre-dip calculation along width direction(inline-), angle zone 1*/
    for ( int jhight=0; jhight<nhight; jhight++ )
    {
	/*collect the attribute value of current section*/
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, base_bina_sect,
	    Array2DImpl<bool> (nwidth,nt) );
    	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, upgr_bina_sect,
	    Array2DImpl<bool> (nwidth,nt) );
	for ( int jwidth=0; jwidth<nwidth; jwidth++ )
	{
	    for ( int jn=0; jn<nt; jn++ )
	    {
		base_bina_sect->set(jwidth,jn, conf_base.get(jwidth,jhight,jn));
		upgr_bina_sect->set(jwidth,jn, conf_upgr.get(jwidth,jhight,jn));
	    }
	}
	
        /*get the component index and component length*/
    	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect,
	    Array2DImpl<float> (nwidth,nt) );
	computeComponentAngle( *base_bina_sect, *upgr_bina_sect, 
		elem_leng, null_val, *dip_sect );
        /*map the dip value from dip_sect to pre4_dip_pca*/
	for ( int kwidth=0; kwidth<nwidth; kwidth++ )
	{
	    for ( int kt=0; kt<nt; kt++ )
		pre4_dip_pca0->set( kwidth,jhight,kt,dip_sect->get(kwidth,kt) );
	}
     }

    /*pre-dip calculation along width direction(xline-), angle zone 3*/
    for ( int jwidth=0; jwidth<nwidth; jwidth++ )
    {
	/*collect the attribute value of current section*/
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, base_bina_sect,
	    Array2DImpl<bool> (nhight,nt) );
    	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, upgr_bina_sect,
	    Array2DImpl<bool> (nhight,nt) );
	for ( int jhight=0; jhight<nhight; jhight++ )
	{
	    for ( int jn=0; jn<nt; jn++ )
	    {
		base_bina_sect->set(jhight,jn, conf_base.get(jwidth,jhight,jn));
		upgr_bina_sect->set(jhight,jn, conf_upgr.get(jwidth,jhight,jn));
	    }
	}
	
        /*get the component index and component length*/
    	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect,
	    Array2DImpl<float> (nhight,nt) );
	computeComponentAngle( *base_bina_sect, *upgr_bina_sect, 
		elem_leng, null_val, *dip_sect );
        /*map the dip value from dip_sect to pre4_dip_pca*/
	for ( int khight=0; khight<nhight; khight++ )
	{
	    for ( int kt=0; kt<nt; kt++ )
		pre4_dip_pca2->set( jwidth,khight,kt,dip_sect->get(khight,kt) );
	}
     }

    /*pre-dip calculation along width direction(3pi/4,/), angle zone 2*/
    const int nsection = nwidth+nhight-1;
    for ( int jsec=0; jsec<nsection; jsec++ )
    {
	int jhight_start, jwidth_start, nwidth_sect;
	/*the section located in the left lower part*/
	if ( jsec<nhight ) 
	{
	    /*get the width of current section*/
	    jhight_start = nhight-jsec-1;
	    jwidth_start = 0;
	    nwidth_sect = 0;
	    int kwidth = jwidth_start;
	    for ( int khight=jhight_start; khight<nhight; khight++ )
	    {
		if ( kwidth>=nwidth )
		    break;

		kwidth++;
    		nwidth_sect++;
	    }
	}
	else /*the section located in the right upper part*/
	{
	    /*get the width of current section*/
	    jhight_start = 0;
	    jwidth_start = jsec-nhight;
	    nwidth_sect = 0;
	    int khight = jhight_start;
	    for ( int kwidth=jwidth_start; kwidth<nwidth; kwidth++ )
	    {
		if ( khight>=nhight ) break;

		khight++;
		nwidth_sect++;
	    }
	}

	/*check the section width*/
	if  ( nwidth_sect<wind_size )
	    continue;
	
	/*collect the attribue value of current section */
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, base_bina_sect,
		Array2DImpl<bool> (nwidth_sect,nt) );
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, upgr_bina_sect,
		Array2DImpl<bool> (nwidth_sect,nt) );
	TypeSet<int> width_index_map;
	TypeSet<int> hight_index_map;
	int kwidth = jwidth_start;
	int khight = jhight_start;
	for ( int kwidth_sect=0; kwidth_sect<nwidth_sect; kwidth_sect++ )
	{
	    width_index_map += kwidth;
	    hight_index_map += khight;
	    for ( int k=0; k<nt; k++ )
	    {
		base_bina_sect->set( kwidth_sect, k,
		    conf_base.get(kwidth,khight,k) );
		upgr_bina_sect->set( kwidth_sect, k,
		    conf_upgr.get(kwidth,khight,k) );
	    }
	    kwidth = kwidth+1;
	    khight = khight+1;
	}
     
        /*get the component index and component length*/
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect,
		Array2DImpl<float> (nwidth_sect,nt) );
	computeComponentAngle( *base_bina_sect, *upgr_bina_sect, 
		elem_leng, null_val, *dip_sect );
        /*map the dip value from dip_sect to pre4_dip_pca*/
	for ( int jwidth=0; jwidth<nwidth_sect; jwidth++ )
	{
	    int iwidth = width_index_map[jwidth];
	    int ihight = hight_index_map[jwidth];
	    for ( int kt=0; kt<nt; kt++ )
		pre4_dip_pca1->set( iwidth,ihight,kt,dip_sect->get(jwidth,kt) );
	}
    }

    /*pre-dip calculation along width direction(pi/4-,\), angle zone 4*/
    for ( int jsec=0; jsec<nsection; jsec++ )
    {
	int jhight_start, jwidth_start, nwidth_sect;
	/*the section located in the upper left part*/
	if ( jsec<nhight ) 
	{
	    /*get the width of current section*/
	    jhight_start = jsec;
	    jwidth_start = 0;
	    nwidth_sect = 0;
	    int kwidth = jwidth_start;
	    for ( int khight=jhight_start; khight>=0; khight-- )
	    {
		if ( kwidth>=nwidth || khight<0 )
		    break;

		kwidth++;
    		nwidth_sect++;
	    }
	}
	else /*the section located in the lower right part*/
	{
	    /*get the width of current section*/
	    jhight_start = nhight-1;
	    jwidth_start = jsec-nhight;
	    nwidth_sect = 0;
	    int khight = jhight_start;
	    for ( int kwidth=jwidth_start; kwidth<nwidth; kwidth++ )
	    {
		if ( khight<0 ) break;

		khight--;
		nwidth_sect++;
	    }
	}

	/*check the section width*/
	if  ( nwidth_sect<wind_size )
	    continue;
	
	/*collect the attribue value of current section */
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, base_bina_sect,
		Array2DImpl<bool> (nwidth_sect,nt) );
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, upgr_bina_sect,
		Array2DImpl<bool> (nwidth_sect,nt) );
	TypeSet<int> width_index_map;
	TypeSet<int> hight_index_map;
	int kwidth = jwidth_start;
	int khight = jhight_start;
	for ( int kwidth_sect=0; kwidth_sect<nwidth_sect; kwidth_sect++ )
	{
	    width_index_map += kwidth;
	    hight_index_map += khight;
	    for ( int k=0; k<nt; k++ )
	    {
		base_bina_sect->set( kwidth_sect, k,
		    conf_base.get(kwidth,khight,k) );
		upgr_bina_sect->set( kwidth_sect, k,
		    conf_upgr.get(kwidth,khight,k) );
	    }
	    kwidth = kwidth+1;
	    khight = khight-1;
	}
     
        /*get the component index and component length*/
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, dip_sect,
		Array2DImpl<float> (nwidth_sect,nt) );
	computeComponentAngle( *base_bina_sect, *upgr_bina_sect, 
		elem_leng, null_val, *dip_sect );
        /*map the dip value from dip_sect to pre4_dip_pca*/
	for ( int jwidth=0; jwidth<nwidth_sect; jwidth++ )
	{
	    int iwidth = width_index_map[jwidth];
	    int ihight = hight_index_map[jwidth];
	    for ( int kt=0; kt<nt; kt++ )
		pre4_dip_pca3->set( iwidth,ihight,kt,dip_sect->get(jwidth,kt) );
	}
    }

    /*Get the dip based on current azimuth value*/
    /*Pre-generate angle and arc set*/
    TypeSet<float> arc_set, angle_set;
    for ( int idx=0; idx<4; idx++ )
    {
	arc_set += idx*M_PI_4f;
	angle_set += idx*45.f;
    }

    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, azimuth_pca_arc,
		Array3DImpl<float> (azimuth_pca.info()) );
    const float angle2arc = M_PI/180;
    const float arc2angle = 180/M_PI;
    for ( int idx=0; idx<nwidth; idx++ )
    {
	for ( int idy=0; idy<nhight; idy++ )
	{
	    for ( int idz=0; idz<nt; idz++ )
	    {
		azimuth_pca_arc->set( idx, idy, idz,
				angle2arc*azimuth_pca.get(idx,idy,idz) );
	    }
	}
    }

    for ( int jwidth=0; jwidth<nwidth; jwidth++ )
    {
	for ( int jhight=0; jhight<nhight; jhight++ )
	{
	    for ( int jt=0; jt<nt; jt++ )
	    {
		if ( !conf_upgr.get(jwidth,jhight,jt) )
		{
		    dip_pca.set( jwidth, jhight, jt, null_val );
		    continue;
		}

		/*Get the two vertical sections*/
		const float azim_curr = azimuth_pca.get(jwidth,jhight,jt);
		TypeSet<float> phi_angle(4,0);
		if ( azim_curr>90 )
		{
		    phi_angle[0] = 180-azim_curr;
		    phi_angle[1] = azim_curr-45;
		    phi_angle[2] = azim_curr-90;
		    phi_angle[3] = fabs(135-azim_curr);
		}
		else
		{
		    phi_angle[0] = azim_curr;
		    phi_angle[1] = fabs(azim_curr-45);
		    phi_angle[2] = 90-azim_curr;
		    phi_angle[3] = 135-azim_curr;
		}

		TypeSet<float> angle_diff_sort;
		TypeSet<int> asort_indx;
		for ( int idx=0; idx<4; idx++ )
		{
		    if ( phi_angle[idx]>90 )
			phi_angle[idx] = 180-phi_angle[idx];
		    phi_angle[idx] = phi_angle[idx]*angle2arc;
		    asort_indx += idx;
		    angle_diff_sort += phi_angle[idx];
		}
		sort_coupled( angle_diff_sort.arr(), asort_indx.arr(), 4 );

		TypeSet<int> win_angle_indx(2,0);
		for ( int ja=0; ja<2; ja++ )
		{
		    const int aindx = asort_indx[ja+2];	
		    win_angle_indx[ja] = aindx;
		}

		/*Map the dip of vertical to true dip*/
		int null_count = 0;
		int null_flag[] = {1,1};
		float theta1_angl[2] = {0,0};
		for ( int ja=0; ja<2; ja++ )
		{
		    const int aindx = win_angle_indx[ja];
		    float theta_angle;
		    if ( aindx==0 )
		    	theta_angle =pre4_dip_pca0->get(jwidth,jhight,jt); 
		    else if ( aindx==1 )
		    	theta_angle =pre4_dip_pca1->get(jwidth,jhight,jt); 
		    else if ( aindx==2 )
		    	theta_angle =pre4_dip_pca2->get(jwidth,jhight,jt); 
		    else 
		    	theta_angle =pre4_dip_pca3->get(jwidth,jhight,jt);

		    if ( fabs(theta_angle-null_val)<0.5 )
		    {
			null_flag[ja] = 0;
			null_count++;
			theta1_angl[ja] = 0;
			continue;
		    }
		    
		    float theta2_arc = theta_angle*angle2arc;
		    float para_a2 = cos(theta2_arc);
    		    float para_b = sin(theta2_arc);
    		    float para_a1 = para_a2*sin( 
				    phi_angle[aindx]*angle2arc );
    		    float theta1_arc = atan(para_b/para_a1);
    		    theta1_angl[ja] = theta1_arc*arc2angle;
		    if ( theta1_angl[ja]<0 )
			theta1_angl[ja] = 180+theta1_angl[ja];
		}

		float aver_dip = null_val;
		if ( null_count<2 )
		{
		    aver_dip = theta1_angl[0]*null_flag[0] +
			       theta1_angl[1]*null_flag[1];
    		    aver_dip = aver_dip/(2-null_count);
		}
		dip_pca.set( jwidth, jhight, jt, aver_dip );
	    }
	}
    }
}


bool FaultOrientation::computeAzimuthPCA( const Array3D<bool>& conf_base,
	const Array3D<bool>& conf_upgr, int elem_leng, float null_val,
	Array3D<float>& azimuth_pca, TaskRunner* tr )
{
    AzimuthPcaCalculator apc( conf_base, conf_upgr, elem_leng, null_val,
	    azimuth_pca );
    return tr ? tr->execute( apc ) : apc.execute();
}


void FaultOrientation::computeComponentAngle( const Array2D<bool>& base_bina,
	const Array2D<bool>& upgr_bina_sect, int elem_leng, float null_val,
	Array2D<float>& azimuth_sect )
{
    azimuth_sect.setAll( null_val );    
    const int csz = base_bina.info().getSize(1);

    ConnComponents cc( base_bina );
    cc.compute();
    const int ncomponent = cc.nrComponents();
    
    for ( int idx=0; idx<ncomponent; idx++ )
    {
	const TypeSet<int>* comp = cc.getComponent( idx );
	if ( !comp )
	    continue;

	TypeSet<int> set_x, set_y;
	const int npoint = comp->size();
	for ( int idy=0; idy<npoint; idy++ )
	{
	    set_x += (*comp)[idy]/csz;
	    set_y += (*comp)[idy]%csz;
	}

	for ( int idy=0; idy<npoint; idy++ )
	{
	    const int current_width_index = set_x[idy];
	    const int current_hight_index = set_y[idy];
	    if ( !upgr_bina_sect.get(current_width_index,current_hight_index) )
		continue;
	    
	    TypeSet<int> point_set_x, point_set_y;
	    const int x0 = set_x[idy];
	    const int y0 = set_y[idy];
	    for ( int kp=0; kp<npoint; kp++ )
	    {
		const int x1 = set_x[kp];
		const int y1 = set_y[kp];
		const int d1 = x0-x1;
		const int d2 = y0-y1;
		const float dist = Math::Sqrt( float(d1*d1+d2*d2) );
		if ( dist<elem_leng )
		{
		    point_set_x += x1;
		    point_set_y += y1;
		}
	    }
	    if ( point_set_x.size()<2 )
		continue;

	    float angle = getAnglePCA( point_set_x, point_set_y, null_val );
	    azimuth_sect.set( current_width_index, current_hight_index, angle );
	}
    }
}


void FaultOrientation::stabilizeAngleSection( const Array2D<bool>& conf_sect,
	const Array2D<float>& angl_sect, int elem_leng_new, float uppr_perc,
	float lowr_perc, float angl_tole, float null_value, 
	Array2D<float>& angl_stab )
{
    const float* anglvals = angl_sect.getData();
    const int totalsz = (int) angl_sect.info().getTotalSz();
    float* angstabvals = angl_stab.getData();
    for ( int idx=0; idx<totalsz; idx++ )
	angstabvals[idx] = anglvals[idx]; 

    ConnComponents cc( conf_sect );
    cc.compute();
    const int ncomponent = cc.nrComponents();
    const int nhight = conf_sect.info().getSize(1);

    for ( int idx=0; idx<ncomponent; idx++ )
    {
	const TypeSet<int>& comp = *cc.getComponent( idx );
	const int npoint = comp.size();

	/*get all the elements for current component*/
	TypeSet<float> angl_vect;
	TypeSet<int> set_x, set_y, elem_x, elem_y, plus_points_flag;
	int a_index = 0;
	TypeSet<int> sort_indx;
	for ( int idy=0; idy<npoint; idy++ )
	{
	    const int kwidth = comp[idy]/nhight;
	    const int khight = comp[idy]%nhight;
	    set_x += kwidth;
	    set_y += khight;
	    if ( fabs(anglvals[comp[idy]]-null_value)>0.5 )
	    {
		sort_indx += a_index;
		a_index++;
		angl_vect += anglvals[comp[idy]];
		elem_x += kwidth;
		elem_y += khight;
		plus_points_flag += 1;
	    }
	    else
		plus_points_flag += 0;
	}

	if ( a_index<=2 )
	    continue;

	TypeSet<float> angl_vect_sort = angl_vect;
	sort_coupled( angl_vect_sort.arr(), sort_indx.arr(), a_index );
	int uppr_indx = (int)ceil( (a_index-1)*uppr_perc );
	uppr_indx = mMIN( a_index-1, uppr_indx );
  	int lowr_indx = (int)ceil( (a_index-1)*lowr_perc );
	lowr_indx = mMAX( 0, lowr_indx );

	float angl_aver = 0;
	for ( int jp=lowr_indx; jp<=uppr_indx; jp++ )
	    angl_aver += angl_vect_sort[jp];
	angl_aver /= (uppr_indx-lowr_indx+1);

	TypeSet<int> odd_set_x, odd_set_y;
	for ( int jp=0; jp<a_index; jp++ )
	{
	    if ( fabs(angl_vect_sort[jp]-angl_aver)>angl_tole )
	    {
		odd_set_x += elem_x[sort_indx[jp]];
		odd_set_y += elem_y[sort_indx[jp]];
	    }
	}

	const int nodd_last = odd_set_x.size();
	for ( int jp=0; jp<nodd_last; jp++ )
	{
	    TypeSet<int> point_set_x, point_set_y;
	    for ( int kp=0; kp<npoint; kp++ )
	    {
		const int term1 = odd_set_x[jp]-set_x[kp];
		const int term2 = odd_set_y[jp]-set_y[kp];
		const float dist = Math::Sqrt( float(term1*term1+term2*term2) );
		if ( dist<=elem_leng_new )
		{
    		    point_set_x += set_x[kp];
    		    point_set_y += set_y[kp];
		}
	    }

	    if ( point_set_x.size()<2 )
		continue;

	    const float angle = getAnglePCA( point_set_x, point_set_y, null_value );
	    angl_stab.set( odd_set_x[jp], odd_set_y[jp], angle );
	}
	
    	for ( int jp=0; jp<npoint; jp++ )
	{
    	    if ( plus_points_flag[jp]==1 )
		continue;

	    TypeSet<int> point_set_x, point_set_y;
    	    for ( int kp=0; kp<npoint; kp++ )
	    {
		const int term1 = set_x[jp]-set_x[kp];
		const int term2 = set_y[jp]-set_y[kp];
    		const float dist = Math::Sqrt( float(term1*term1+term2*term2) );
    		if ( dist<=elem_leng_new )
		{
    		    point_set_x += set_x[kp];
                    point_set_y += set_y[kp];
		}
		
	        if ( point_set_x.size()<2 )
		    continue;

		const float angl_plus = 
		    getAnglePCA( point_set_x, point_set_y, null_value );	
		if ( fabs(angl_plus-angl_aver) <= angl_tole )
		    angl_stab.set( set_x[jp], set_y[jp], angl_plus );
	    }
	}
    }
}


float FaultOrientation::getAnglePCA( const TypeSet<int>& point_set_x,
	const TypeSet<int>& point_set_y, float null_value )
{
    const int npoints = point_set_x.size();
    float x_mean = 0.0;
    float y_mean = 0.0;
    for ( int jp=0; jp<npoints; jp++ )
    {
	x_mean += point_set_x[jp];
	y_mean += point_set_y[jp];
    }
    x_mean = x_mean/npoints;
    y_mean = y_mean/npoints;

    float var_xx = 0.0;
    float var_yy = 0.0;
    float var_xy = 0.0;
    for ( int jp=0; jp<npoints; jp++ )
    {
	var_xx += (point_set_x[jp]-x_mean)*(point_set_x[jp]-x_mean);
	var_yy += (point_set_y[jp]-y_mean)*(point_set_y[jp]-y_mean);
	var_xy += (point_set_x[jp]-x_mean)*(point_set_y[jp]-y_mean);
    }

    var_xx /= (npoints-1);
    var_yy /= (npoints-1);
    var_xy /= (npoints-1);

    const float d0[] = { var_xx, var_xy };
    const float d1[] = { var_xy, var_yy };
    PCA pca(2);
    pca.addSample(d0);
    pca.addSample(d1);
    pca.calculate();

    TypeSet<float> eigenval;
    for ( int idy=0; idy<2; idy++ )
	eigenval += pca.getEigenValue(idy);

    float azimuth_dip = null_value;
    float eig_ratio = eigenval[0]/(eigenval[1]+eigenval[0]);
    if ( eig_ratio>0.5 )
    {
	TypeSet<float> eigenvec0(2,0);
	TypeSet<float> eigenvec(2,0);
	pca.getEigenVector( 0, eigenvec0 );
	pca.getEigenVector( 1, eigenvec );
	if ( mIsZero(eigenvec[0],1e-8) )
	    return 90;

	azimuth_dip = atan( eigenvec[1]/eigenvec[0] )*180/M_PIf;
	if ( azimuth_dip<0 )
	    azimuth_dip += 180;
    }
    
    return azimuth_dip;
}    


void FaultOrientation::stabilizeAzimuth( const Array3D<bool>& conf_bina,
	const Array3D<float>& azimuth_orig, int elem_leng, float null_val,
	Array3D<float>& azim_stab )
{
    const int isz = azimuth_orig.info().getSize(0);
    const int csz = azimuth_orig.info().getSize(1);
    const int elem_leng_new = elem_leng*4;
    const float uppr_perc = 0.85;
    const float lowr_perc = 0.15;
    const float angl_tole = 10;

    
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, base_bina_sect,
	    Array2DImpl<bool> (isz,csz) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, angl_orig_sect,
	    Array2DImpl<float> (isz,csz) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, azim_sect_stab,
	    Array2DImpl<float> (isz,csz) );

    const int zsz = azimuth_orig.info().getSize(2);
    for ( int idz=0; idz<zsz; idz++ )
    {
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
	    {
		base_bina_sect->set( idx, idy, conf_bina.get(idx,idy,idz) ); 
		angl_orig_sect->set( idx, idy, azimuth_orig.get(idx,idy,idz) ); 
	    }
	}

	stabilizeAngleSection( *base_bina_sect, *angl_orig_sect, 
		elem_leng_new, uppr_perc, lowr_perc, angl_tole, null_val, 
		*azim_sect_stab );

	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
	    {
		azim_stab.set( idx, idy, idz, azim_sect_stab->get(idx,idy) );
	    }
	}
    }
}


void FaultOrientation::setFaultConfidence( const Array3D<bool>& vein_bina_t0,
	const Array3D<bool>& vein_bina_0, const Array3D<bool>& vein_bina_45, 
	const Array3D<bool>& vein_bina_90, const Array3D<bool>& vein_bina_135, 
	Array3D<bool>& conf_low, Array3D<bool>& conf_med, 
	Array3D<bool>& conf_high )
{
    const int isz = vein_bina_t0.info().getSize(0);
    const int csz = vein_bina_t0.info().getSize(1);
    const int zsz = vein_bina_t0.info().getSize(2);

    for ( int idx=0; idx<isz; idx++ )
    {
	for ( int idy=0; idy<csz; idy++ )
	{
	    for ( int idz=0; idz<zsz; idz++ )
	    {
		const bool t0_flag = vein_bina_t0.get(idx,idy,idz);
		const int ncount = (int)vein_bina_0.get(idx,idy,idz) +
		    		   (int)vein_bina_45.get(idx,idy,idz) +
				   (int)vein_bina_90.get(idx,idy,idz) +
				   (int)vein_bina_135.get(idx,idy,idz);
		conf_low.set( idx, idy, idz, t0_flag && ncount>=1 );
		conf_med.set( idx, idy, idz, t0_flag && ncount>=2 ); 
		conf_high.set( idx, idy, idz, t0_flag && ncount>=3 ); 
	    }
	}
    }
}


void FaultOrientation::computeVerticalVeinSlice( const Array3D<float>& input, 
	Array3D<bool>& vein_bina_0, Array3D<bool>& vein_bina_45, 
	Array3D<bool>& vein_bina_90, Array3D<bool>& vein_bina_135 )
{
    const int isz = input.info().getSize(0);
    const int csz = input.info().getSize(1);
    const int zsz = input.info().getSize(2);
    const int wind_size = 4*sigma_;
    const bool is_t_slic = false;
    
    /*vein calculation along width direction(horizontal-), angle zone 1*/
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, attr_sect0,
	    Array2DImpl<float> (isz,zsz) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_bina_sect0,
	    Array2DImpl<bool> (isz,zsz) );
    for ( int idy=0; idy<csz; idy++ )
    {
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idz=0; idz<zsz; idz++ )
		attr_sect0->set( idx, idz, input.get(idx,idy,idz) );
	}
	    
	compute2DVeinBinary( *attr_sect0, threshold_, isfltabove_, 
		minfaultlength_, sigma_, percent_, is_t_slic, 
		*vein_bina_sect0, 0 );
	
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idz=0; idz<zsz; idz++ )
		vein_bina_0.set( idx, idy, idz, vein_bina_sect0->get(idx,idz) );
	}	
    }
    
    /*vein calculation along hight direction(vertical |), angle zone 3*/
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, attr_sect90,
	    Array2DImpl<float> (csz,zsz) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_bina_sect90,
	    Array2DImpl<bool> (csz,zsz) );
    for ( int idx=0; idx<isz; idx++ )
    {
	for ( int idy=0; idy<csz; idy++ )
	{
	    for ( int idz=0; idz<zsz; idz++ )
		attr_sect90->set( idy, idz, input.get(idx,idy,idz) );
	}
	    
	compute2DVeinBinary( *attr_sect90, threshold_, isfltabove_,
		minfaultlength_, sigma_, percent_, is_t_slic, 
		*vein_bina_sect90, 0 );
	
	for ( int idy=0; idy<csz; idy++ )
	{
	    for ( int idz=0; idz<zsz; idz++ )
		vein_bina_90.set(idx, idy, idz, vein_bina_sect90->get(idy,idz));
	}	
    }

    /*dip calculation along width direction(pi/4, /), angle zone 4 */
    const int nsection = isz + csz - 1;
    for ( int jsec=0; jsec<nsection; jsec++ )
    {
	int jhight_start, nwidth_sect;
	/*the section located in the upper left part*/
	if ( jsec<csz ) 
	{
	    /*get the width of current section*/
	    jhight_start = jsec;
	    nwidth_sect = 0;
	    int kwidth = 0;
	    for ( int khight=jhight_start; khight>=0; khight-- )
	    {
		if ( kwidth>=isz || khight<0 )
		    break;

		kwidth++;
    		nwidth_sect++;
	    }
	}
	else /*the section located in the lower right part*/
	{
	    /*get the width of current section*/
	    jhight_start = csz-1;
	    int jwidth_start = jsec-csz;
	    nwidth_sect = 0;
	    int khight = jhight_start;
	    for ( int kwidth=jwidth_start; kwidth<isz; kwidth++ )
	    {
		if ( khight<0 ) break;

		khight = khight-1;
		nwidth_sect = nwidth_sect+1;
	    }
	}

	/*check the section width*/
	if  ( nwidth_sect<wind_size )
	    continue;
	
	/*collect the attribue value of current section */

	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, attr_sect,
		Array2DImpl<float> (nwidth_sect,zsz) );
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_bina_sect,
		Array2DImpl<bool> (nwidth_sect,zsz) );
	TypeSet<int> width_index_map;
	TypeSet<int> hight_index_map;
	int kwidth = 0;
	int khight = jhight_start;
	for ( int kwidth_sect=0; kwidth_sect<nwidth_sect; kwidth_sect++ )
	{
	    width_index_map += kwidth;
	    hight_index_map += khight;
	    for ( int k=0; k<zsz; k++ )
		attr_sect->set( kwidth_sect, k, input.get(kwidth,khight,k) );
	    kwidth = kwidth+1;
	    khight = khight-1;
	}
     
	compute2DVeinBinary( *attr_sect, threshold_, isfltabove_,
		minfaultlength_, sigma_, percent_, is_t_slic, 
		*vein_bina_sect, 0 );
    	for ( int idx=0; idx<nwidth_sect; idx++ )
    	{
	    int iwidth = width_index_map[idx];
	    int ihight = hight_index_map[idx];
    	    for ( int idz=0; idz<zsz; idz++ )
    		vein_bina_45.set( iwidth, ihight, idz, 
			vein_bina_sect->get(idx,idz) );
    	}	
    }
    
    /*dip calculation along width direction(3pi/4, \), angle zone 2 */
    for ( int jsec=0; jsec<nsection; jsec++ )
    {
	int jhight_start, nwidth_sect;
	/*the section located in the left lower part*/
	if ( jsec<csz ) 
	{
	    /*get the width of current section*/
	    jhight_start = csz-jsec-1;
	    nwidth_sect = 0;
	    int kwidth = 0;
	    for ( int khight=jhight_start; khight<csz; khight++ )
	    {
		if ( kwidth>=isz )
		    break;

		kwidth++;
    		nwidth_sect++;
	    }
	}
	else /*the section located in the right upper part*/
	{
	    /*get the width of current section*/
	    jhight_start = 0;
	    int jwidth_start = jsec-csz;
	    nwidth_sect = 0;
	    int khight = jhight_start;
	    for ( int kwidth=jwidth_start; kwidth<isz; kwidth++ )
	    {
		if ( khight>=csz ) break;

		khight = khight+1;
		nwidth_sect = nwidth_sect+1;
	    }
	}

	/*check the section width*/
	if  ( nwidth_sect<wind_size )
	    continue;
	
	/*collect the attribue value of current section */

	mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, attr_sect,
		Array2DImpl<float> (nwidth_sect,zsz) );
	mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_bina_sect,
		Array2DImpl<bool> (nwidth_sect,zsz) );
	TypeSet<int> width_index_map;
	TypeSet<int> hight_index_map;
	int kwidth = 0;
	int khight = jhight_start;
	for ( int kwidth_sect=0; kwidth_sect<nwidth_sect; kwidth_sect++ )
	{
	    width_index_map += kwidth;
	    hight_index_map += khight;
	    for ( int k=0; k<zsz; k++ )
		attr_sect->set( kwidth_sect, k, input.get(kwidth,khight,k) );
	    kwidth = kwidth+1;
	    khight = khight+1;
	}
     
	compute2DVeinBinary( *attr_sect, threshold_, isfltabove_, 
		minfaultlength_, sigma_, percent_, is_t_slic, 
		*vein_bina_sect, 0 );
    	for ( int idx=0; idx<nwidth_sect; idx++ )
    	{
	    int iwidth = width_index_map[idx];
	    int ihight = hight_index_map[idx];
    	    for ( int idz=0; idz<zsz; idz++ )
    		vein_bina_135.set( iwidth, ihight, idz, 
			vein_bina_sect->get(idx,idz) );
    	}	
    }
}


bool FaultOrientation::computeVeinSlices( const Array3D<float>& input, 
	Array3D<bool>& output, TaskRunner* tr )
{
    VeinSliceCalculator vsc( input, threshold_, isfltabove_, minfaultlength_,
	    sigma_, percent_, output );
    return tr ? tr->execute(vsc) : vsc.execute();
}


bool FaultOrientation::compute2DVeinBinary( const Array2D<float>& input, 
	float threshold, bool isabove, int fault_min_length, int sigma, 
	float perc, bool is_t_slic, Array2D<bool>& vein_bina, TaskRunner* tr )
{
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, vein_score,
	    Array2DImpl<float> (input.info()) );
    if ( !vein_score ) return false;

    computeMaxCurvature( input, sigma, is_t_slic, *vein_score, tr );
    const float* vein_score_vector = vein_score->getData();

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, tmparr,
	    Array2DImpl<float> (input.info()) );
    if ( !tmparr ) return false;
    tmparr->copyFrom( *vein_score );
    float* vein_score_vector_sort = tmparr->getData();
    const int datasz = (int) input.info().getTotalSz();
    sort_array(vein_score_vector_sort,datasz);

    const od_int64 thresholdidx = (od_int64)(perc*datasz);
    const float vein_score_threshold = vein_score_vector_sort[thresholdidx];
    
    const float* inputarr = input.getData();
    bool* vbdata = vein_bina.getData();
    for ( int idx=0; idx<datasz; idx++ )
	vbdata[idx] = vein_score_vector[idx] > vein_score_threshold;    
    
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, input_bina,
	    Array2DImpl<bool> (input.info()) );
    if ( !input_bina ) return false;
    bool* inputdata = input_bina->getData();
    for ( int idx=0; idx<datasz; idx++ )
	inputdata[idx] = (mIsUdf(inputarr[idx])) ? false : 
	    ( (isabove && inputarr[idx]>=threshold) ||
	      (!isabove && inputarr[idx]<=threshold) );   

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, input_bina_thin,
	    Array2DImpl<bool> (input.info()) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, input_comp_thin,
	    Array2DImpl<bool> (input.info()) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<int> >, input_comp,
	    Array2DImpl<int> (input.info()) );
    thinning( *input_bina, *input_bina_thin );
    
    input_comp_thin->setAll( 0 );
    ConnComponents cc( *input_bina );
    cc.compute();
    input_comp->copyFrom( *cc.getLabel() );
    const int isz = input.info().getSize(0);	
    const int csz = input.info().getSize(1);	
    for ( int idx=0; idx<isz; idx++ )
    {
	for ( int idy=0; idy<csz; idy++ )
	{
	    const int marker = input_comp->get(idx,idy);
	    if ( marker>0 && input_bina_thin->get(idx,idy) )
		input_comp_thin->set( idx, idy, marker );    
	}	
    }

    ConnComponents cc_thin( *input_comp_thin );
    cc_thin.compute();
    const int nrcomp_thin = cc_thin.nrComponents();
    int* inputcompdata = input_comp->getData();
    bool* inputcompdata_thin = input_comp_thin->getData();
    for ( int idx=0; idx<nrcomp_thin; idx++ )
    {
	const TypeSet<int>* comp = cc_thin.getComponent(idx);
	const int compsz = comp ? comp->size() : 0;
	if ( compsz>=fault_min_length )
	    continue;

	for ( int idy=0; idy<compsz; idy++ )
	{
	    inputcompdata[(*comp)[idy]] = 0;	
	    inputcompdata_thin[(*comp)[idy]] = 0;	
	}
    }
    
    for ( int idx=0; idx<datasz; idx++ )
	inputdata[idx] = inputcompdata[idx] ? 1 : 0;

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_bina_merge,
	    Array2DImpl<bool> (input.info()) );
    vein_bina_merge->copyFrom( vein_bina );
    bool* mergedata = vein_bina_merge->getData();
    for ( int idx=0; idx<datasz; idx++ )
    {
	if ( inputdata[idx]==0 || vbdata[idx]==1 )
	    continue;

	mergedata[idx] = 1;
    }

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_bina_thin,
	    Array2DImpl<bool> (input.info()) );
    thinning( *vein_bina_merge, *vein_bina_thin );
    
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<int> >, vein_comp,
	    Array2DImpl<int> (input.info()) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_comp_thin,
	    Array2DImpl<bool> (input.info()) );
    ConnComponents cc1( *vein_bina_merge );
    cc1.compute();
    vein_comp->copyFrom( *cc1.getLabel() );
    vein_comp_thin->setAll( 0 );
    for ( int x=0; x<isz; x++ )
    {
	for ( int y=0; y<csz; y++ )
	{
    	    if ( vein_bina_thin->get(x,y) && vein_comp->get(x,y) )
		vein_comp_thin->set( x, y, vein_comp->get(x,y) );
	}
    }

    ConnComponents cc2( *vein_comp_thin );
    cc2.compute();
    const int nrcomp_vthin = cc2.nrComponents();
    int* vcompdata = vein_comp->getData();
    for ( int idx=0; idx<nrcomp_vthin; idx++ )
    {
	const TypeSet<int>* comp = cc2.getComponent(idx);
	const int compsz = comp ? comp->size() : 0;
	if ( compsz>=fault_min_length )
	    continue;

	for ( int idy=0; idy<compsz; idy++ )
	    vcompdata[(*comp)[idy]] = 0;	
    }

    for ( int idx=0; idx<datasz; idx++ )
	vbdata[idx] = vcompdata[idx] ? 1 : 0;
    return true;
}


void FaultOrientation::thinning( const Array2D<bool>& orig, Array2D<bool>& res )
{
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, tmp,
	    Array2DImpl<bool> (res.info()) );
    if ( !tmp ) return;

    for ( int idx=0; idx<mNrThinning; idx++ )
    {
	thinStep( orig, *tmp, true );
	thinStep( *tmp, res, false );
    }
}


void FaultOrientation::thinStep( const Array2D<bool>& input, 
	Array2D<bool>& output, bool firststep )
{
    const int lastidx = input.info().getSize(0)-1;
    const int lastidy = input.info().getSize(1)-1;
    for ( int idx=0; idx<=lastidx; idx++ )
    {
	for ( int idy=0; idy<=lastidy; idy++ )
	{
	    bool ison = input.get(idx,idy);
	    output.set( idx, idy, ison );
	    if ( !ison )
		continue;

	    bool bw00 = !idx || !idy ? false : input.get(idx-1,idy-1);
	    bool bw01 = !idx ? false : input.get(idx-1,idy);
	    bool bw02 = !idx || idy==lastidy ? false : input.get(idx-1,idy+1);
	    bool bw10 = !idy ? false : input.get(idx,idy-1);
	    bool bw12 = idy==lastidy ? false : input.get(idx,idy+1);
	    bool bw20 = idx==lastidx || !idy ? false : input.get(idx+1,idy-1);
	    bool bw21 = idx==lastidx ? false : input.get(idx+1,idy);
	    bool bw22 = 
		idx==lastidx || idy==lastidy ? false : input.get(idx+1,idy+1);

	    const bool b1 = !bw12 && (bw02 || bw01);
	    const bool b2 = !bw01 && (bw00 || bw10);
	    const bool b3 = !bw10 && (bw20 || bw21);
	    const bool b4 = !bw21 && (bw22 || bw12);
	    const char b = (char)b1 + (char)b2 + (char)b3 + (char)b4;
	    if ( b!=1 ) 
		continue;

	    double N1 = double( bw12 | bw02 ) + double( bw01 | bw00 ) + 
			double( bw10 | bw20 ) + double( bw21 | bw22 );
	    double N2 = double( bw02 | bw01 ) + double( bw00 | bw10 ) + 
			double( bw20 | bw21 ) + double( bw22 | bw12 );
	    double NCondition2 = mMIN(N1,N2);
	    if ( NCondition2<2 || NCondition2>3 )
		continue;
	    
	    int NCondition3 = firststep ? ((bw02 | bw01 | (!bw22)) & bw12)
					: ((bw20 | bw21 | (!bw00)) & bw10);
	    if ( !NCondition3 )
		output.set( idx, idy, 0 );
	}
    }
}


bool FaultOrientation::computeMaxCurvature( const Array2D<float>& input,
	       int sigma, bool is_t_slice, Array2D<float>& res, TaskRunner* tr )
{
    const int inputsz0 = input.info().getSize(0);
    const int xmaxidx = inputsz0 - 1; 
    const int inputsz1 = input.info().getSize(1);
    const int ymaxidx = inputsz1 - 1; 
    const int winsize = 4*sigma;
    const int sidesize = 2*winsize+1;
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, xtmp,
	    Array2DImpl<float>(sidesize,sidesize) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, ytmp,
	    Array2DImpl<float>(sidesize,sidesize) );
    if ( !xtmp || !ytmp ) 
	return false;

    for ( int idx=0; idx<sidesize; idx++ )
    {
	for ( int idy=0; idy<sidesize; idy++ )
	{
	    xtmp->set( idx, idy, mCast(float,idy-winsize) );
	    ytmp->set( idx, idy, mCast(float,idx-winsize) );
	}
    }

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, h,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, hx,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, hy,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, hxx,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, hxy,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, hyy,
	    Array2DImpl<float> (sidesize,sidesize) );
    if ( !h || !hx || !hy || !hxx || !hxy | !hyy )
	return false;

    const int sigma2 = sigma*sigma;
    const int sigma4 = sigma2*sigma2;
    const float coef = 1.0f/(2*M_PIf*sigma2);
    for ( int idx=0; idx<sidesize; idx++ )
    {
	for ( int idy=0; idy<sidesize; idy++ )
	{
	    float x = xtmp->get(idx,idy);
	    float x2 = x*x;
	    float y = ytmp->get(idx,idy);
	    float val = coef*exp(-(x2+y*y)/(2*sigma2));

	    h->set( idx, idy, val );
	    hx->set( idx, idy, val*(-1*x/sigma2) );
	    hxx->set( idx, idy, val*(x*x-sigma2)/sigma4 );
	    hxy->set( idx, idy, val*x*y/sigma4 );
	}
    }

    for ( int idx=0; idx<sidesize; idx++ )
    {
	for ( int idy=0; idy<sidesize; idy++ )
	{
	    hy->set( idx, idy, hx->get(idy,idx) );
	    hyy->set( idx, idy, hxx->get(idy,idx) );
	}
    }

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, ftmp,
	    Array2DImpl<float> (inputsz0+sidesize-1,inputsz1+sidesize-1) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, fx,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, fy,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, fxx,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, fxy,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, fyy,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    if ( !fx || !fy || !fxx || !fxy | !fyy )
	return false;
    
    Convolver2D<float> conv2;
    conv2.setX( input, true );
    conv2.setNormalize( false );
    conv2.setCorrelate( false );
    conv2.setZ( *ftmp );

    conv2.setY( *hx, false );
    bool isdone = tr ? tr->execute(conv2) : conv2.execute();
    if ( !isdone )
	return false;
    for (int i=0; i<inputsz0; i++ )
    {
	for (int j=0; j<inputsz1; j++ )
	    fx->set(i,j,ftmp->get(i+winsize,j+winsize) );
    }

    conv2.setY( *hy, false );
    isdone = tr ? tr->execute(conv2) : conv2.execute();
    if ( !isdone )
	return false;
    for (int i=0; i<inputsz0; i++ )
    {
	for (int j=0; j<inputsz1; j++ )
	    fy->set(i,j,ftmp->get(i+winsize,j+winsize) );
    }

    conv2.setY( *hxx, false );
    isdone = tr ? tr->execute(conv2) : conv2.execute();
    if ( !isdone )
	return false;
    for (int i=0; i<inputsz0; i++ )
    {
	for (int j=0; j<inputsz1; j++ )
	    fxx->set(i,j,ftmp->get(i+winsize,j+winsize) );
    }

    conv2.setY( *hxy, false );
    isdone = tr ? tr->execute(conv2) : conv2.execute();
    if ( !isdone )
	return false;
    for (int i=0; i<inputsz0; i++ )
    {
	for (int j=0; j<inputsz1; j++ )
	    fxy->set(i,j,ftmp->get(i+winsize,j+winsize) );
    }

    conv2.setY( *hyy, false );
    isdone = tr ? tr->execute(conv2) : conv2.execute();
    if ( !isdone )
	return false;
    for (int i=0; i<inputsz0; i++ )
    {
	for (int j=0; j<inputsz1; j++ )
	    fyy->set(i,j,ftmp->get(i+winsize,j+winsize) );
    }

    const int nrangles = 8;
    TypeSet<float> angle_set, angle_set_cos, angle_set_cos2, angle_set_sin,
	angle_set_sin2;
    for ( int idx=0; idx<nrangles; idx++ )
    {
	const float angle = M_PIf*idx/nrangles;
	const float cosangle = cos(angle);
	const float sinangle = sin(angle);
	angle_set += angle;
	angle_set_cos += cosangle;
	angle_set_cos2 += cosangle * cosangle;
	angle_set_sin += sinangle;
	angle_set_sin2 += sinangle * sinangle;
    }

    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, k,
	    Array3DImpl<float> (inputsz0,inputsz1,nrangles) );
    if ( !k )
	return false;

    for ( int idz=0; idz<nrangles; idz++ )
    {
	for ( int idx=0; idx<inputsz0; idx++ )
	{
	    for ( int idy=0; idy<inputsz1; idy++ )
	    {
		float dir1 = fx->get(idx,idy)*angle_set_cos[idz]+
		    fy->get(idx,idy)*angle_set_sin[idz];

		float dir2 = fxx->get(idx,idy)*angle_set_cos2[idz] +
		    fxy->get(idx,idy)*2*angle_set_cos[idz]*angle_set_sin[idz] +
    		    fyy->get(idx,idy)*angle_set_sin2[idz];

		float demomenator = Math::PowerOf( 1.0f+dir1*dir1, 1.5f );
		k->set( idx, idy, idz, dir2/demomenator ); 
	    }
	}
    }
    
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, vt,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    if ( !vt ) return false;
    vt->setAll(0);

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<int> >, xstep,
	    Array2DImpl<int> (nrangles,2) );
    if ( !xstep ) return false;

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<int> >, ystep,
	    Array2DImpl<int> (nrangles,2) );
    if ( !ystep ) return false;

    xstep->set(0,0,-2);
    xstep->set(0,1,2);
    xstep->set(1,0,-2);
    xstep->set(1,1,2);
    xstep->set(2,0,-2);
    xstep->set(2,1,2);
    xstep->set(3,0,-1);
    xstep->set(3,1,1);
    xstep->set(4,0,0);
    xstep->set(4,1,0);
    xstep->set(5,0,-1);
    xstep->set(5,1,1);
    xstep->set(6,0,-2);
    xstep->set(6,1,2);
    xstep->set(7,0,-2);
    xstep->set(7,1,2);

    ystep->set(0,0,0);
    ystep->set(0,1,0);
    ystep->set(1,0,-1);
    ystep->set(1,1,1);
    ystep->set(2,0,-2);
    ystep->set(2,1,2);
    ystep->set(3,0,-2);
    ystep->set(3,1,2);
    ystep->set(4,0,-2);
    ystep->set(4,1,2);
    ystep->set(5,0,2);
    ystep->set(5,1,-2);
    ystep->set(6,0,2);
    ystep->set(6,1,-2);
    ystep->set(7,0,1);
    ystep->set(7,1,-1);

    TypeSet<float> wr_step;
    wr_step += Math::Sqrt(float(2*2+0*0));
    wr_step += Math::Sqrt(float(2*2+1*1));
    wr_step += Math::Sqrt(float(2*2+2*2));
    wr_step += Math::Sqrt(float(1*1+2*2));
    wr_step += Math::Sqrt(float(0*0+2*2));
    wr_step += Math::Sqrt(float(1*1+2*2));
    wr_step += Math::Sqrt(float(2*2+2*2));
    wr_step += Math::Sqrt(float(2*2+1*1));

    for ( int ai=0; ai<nrangles; ai++ )
    {
	const int y_step_left = xstep->get(ai,0);
	const int x_step_left = ystep->get(ai,0);
	const int y_step_right = xstep->get(ai,1);
	const int x_step_right = ystep->get(ai,1);
	if ( ai==0 )
	{
	    if ( !is_t_slice )
		continue;

	    for ( int idx=0; idx<inputsz0; idx++ )
	    {
		for ( int idy=0; idy<inputsz1; idy++ )
		{
		    if ( k->get(idx,idy,ai)<=0 )
			continue;

		    int ystart=idy, ystop=idy; 
		    float wr = wr_step[ai];
			
		    /*detect the start position*/
		    int y_temp_start = mMAX(idy+y_step_left,0);
		    for ( int yi=y_temp_start; yi>=0; yi += y_step_left )
		    {
			const float curk = k->get(idx,yi,ai);
			if ( !yi || curk<=0 )
			{
			    ystart = curk<=0 ? yi-y_step_left : 0;
			    break;
			}
			wr += wr_step[ai];
		    }
		    
		    /*detect the stop position*/
		    y_temp_start = mMIN(idy+y_step_right,ymaxidx);
		    for ( int yi=y_temp_start; yi<inputsz1; yi += y_step_right )
		    {
			const float curk = k->get(idx,yi,ai);
			if ( yi==ymaxidx || curk<=0 )
			{
			    ystop = curk<=0 ? yi-y_step_right : ymaxidx;
			    break;
			}
			wr += wr_step[ai];
		    }

		    if ( ystart<0 ) ystart = 0;
		    if ( ystop<0 ) ystop = 0;

		    int ymax = ystart;
		    float kmax = k->get(idx,ymax,ai);
		    int loopsz = (ystop-ystart)/y_step_right+1;
		    if ( loopsz<0 ) loopsz = -loopsz;
		    for ( int i=0; i<loopsz; i++ )
		    {
			int yi = ystart+i*y_step_right;  
			const float curk = k->get(idx,yi,ai);
			if ( curk>kmax )
			{
			    kmax = curk;
			    ymax = yi;
			}
		    }
			
		    float score = k->get(idx,ymax,ai)*wr;
		    vt->set(idx,ymax,vt->get(idx,ymax)+score);
		}
	    }
	}
	else if ( ai<4 )
	{
	    for ( int idx=0; idx<inputsz0; idx++ )
	    {
		for ( int idy=0; idy<inputsz1; idy++ )
		{
		    if ( k->get(idx,idy,ai)<=0 )
			continue;

		    int xstart=idx, ystart=idy, ystop=idy; 
		    float wr = wr_step[ai];

		    /*detect the start position*/
		    int xi = mMAX(idx+x_step_left,0);
		    int y_temp_start = mMAX(idy+y_step_left,0);
		    for ( int yi=y_temp_start; yi>=0; yi += y_step_left )
		    {
			float curk = k->get(xi,yi,ai);
			if ( !xi || !yi || curk<=0 )
			{
			    if ( curk<=0 )
			    {
				xstart = xi-x_step_left;
				ystart = yi-y_step_left;
			    }
			    else if ( !xi )
			    {
				xstart = 0;
				ystart = yi;
			    }
			    else
			    {
				xstart = xi;
				ystart = 0;
			    }
			    break;
			}
			xi += x_step_left;
			if ( xi<0 || xi>xmaxidx )
			    break;
			wr += wr_step[ai];
		    }
		    
		    /*detect the stop position*/
		    xi = mMIN(idx+x_step_right,xmaxidx);
		    int y_temp_stop = mMIN(idy+y_step_right,ymaxidx);
		    for ( int yi=y_temp_stop; yi<inputsz1; yi += y_step_right )
		    {
    			float curk = k->get(xi,yi,ai);
			if ( xi==xmaxidx || yi==ymaxidx || curk<=0 )
			{
			    ystop = curk<=0 ? yi-y_step_right 
					    : (yi==ymaxidx ? ymaxidx : yi);
			    break;
			}
			xi += x_step_right;
			if ( xi<0 ||xi>xmaxidx )
			    break;

			wr += wr_step[ai];
		    }

		    if ( ystart>=ymaxidx )
			ystart = ymaxidx;

		    int xmax=xstart,ymax=ystart;
		    float kmax = k->get(xmax,ymax,ai);
		    xi = xstart;
		    int loopsz = (ystop-ystart)/y_step_right+1;
		    if ( loopsz<0 ) loopsz = -loopsz;
		    for ( int i=0; i<loopsz; i++ )
		    {
			int yi = ystart+i*y_step_right;  
    			float curk = k->get(xi,yi,ai);
			if ( curk>kmax )
			{
			    kmax = curk;
			    xmax = xi;
			    ymax = yi;
			}

			xi += x_step_right;
			if ( xi>xmaxidx )
			    break;
		    }
			
		    float score = k->get(xmax,ymax,ai)*wr;
		    vt->set(xmax,ymax,vt->get(xmax,ymax)+score);
		}
	    }
	}
	else if ( ai==4 )
	{
	    for ( int idx=0; idx<inputsz0; idx++ )
	    {
		for ( int idy=0; idy<inputsz1; idy++ )
		{
		    if ( k->get(idx,idy,ai)<=0 )
			continue;

		    int xstart=idx, xstop=idx; 
		    float wr = wr_step[ai];

		    /*detect the start position*/
		    int x_start_temp = mMAX(idx+x_step_left,0);
		    for ( int xi=x_start_temp; xi>=0; xi += x_step_left )
		    {
			const float curk = k->get(xi,idy,ai);
			if ( !xi || curk<=0 )
			{
			    xstart = curk<=0 ? xi-x_step_left : 0;
			    break;
			}

			wr += wr_step[ai];
		    }
		    
		    /*detect the stop position*/
		    int x_end_temp = mMIN(idx+x_step_right,xmaxidx);
		    for ( int xi=x_end_temp; xi<=xmaxidx; xi += x_step_right )
		    {
    			const float curk = k->get(xi,idy,ai);
			if ( xi==xmaxidx || curk<=0 )
			{
			    xstop = curk<=0 ? xi-x_step_right : xmaxidx;
			    break;
			}

			wr += wr_step[ai];
		    }

		    int xmax = xstart;
		    float kmax = k->get(xmax,idy,ai);
		    int loopsz = (xstop-xstart)/x_step_right+1;
		    if ( loopsz<0 ) loopsz = -loopsz;
		    for ( int i=0; i<loopsz; i++ )
		    {
			int xi = xstart+i*x_step_right;  
			const float curk = k->get(xi,idy,ai);
			if ( curk>kmax )
			{
			    kmax = curk;
			    xmax = xi;
			}
		    }
			
		    float score = k->get(xmax,idy,ai)*wr;
		    vt->set(xmax,idy,vt->get(xmax,idy)+score);
		}
	    }
	}
	else
	{
	    for ( int idx=0; idx<inputsz0; idx++ )
	    {
		for ( int idy=0; idy<inputsz1; idy++ )
		{
		    if ( k->get(idx,idy,ai)<=0 )
			continue;

		    int xstart=idx, xstop=idx, ystart=idy; 
		    float wr = wr_step[ai];

		    /*detect the start position*/
		    int yi = mMAX(idy+y_step_left,0);
		    int x_temp_start = mMIN(idx+x_step_left,xmaxidx);
		    for ( int xi=x_temp_start; xi<=xmaxidx; xi += x_step_left )
		    {
    			float curk = k->get(xi,yi,ai);
			if ( !yi || xi==xmaxidx || curk<=0 )
			{
			    if ( curk<=0 )
			    {
				xstart = xi-x_step_left;
				ystart = yi-y_step_left;
			    }
			    else if ( !yi )
			    {
				ystart = 0;
				xstart = xi;
			    }
			    else
			    {
				ystart = yi;
				xstart = xmaxidx;
			    }
			    break;
			}

			yi += y_step_left;
			if ( yi<0 )
			    break;

			wr += wr_step[ai];
		    }
		    
		    /*detect the stop position*/
		    yi = mMIN(idy+y_step_right,ymaxidx);
		    int x_temp_stop = mMAX(idx+x_step_right,0);
		    for ( int xi=x_temp_stop; xi>=0; xi += x_step_right )
		    {
    			float curk = k->get(xi,yi,ai);
			if ( yi==ymaxidx || !xi || curk<=0 )
			{
			    xstop = curk<0 ? xi-x_step_right : (!xi ? 0 : xi);
			    break;
			}

			yi += y_step_right;
			if ( yi>ymaxidx )
			    break;

			wr += wr_step[ai];
		    }

		    if ( ystart>=ymaxidx )
			ystart = ymaxidx;
		    int xmax=xstart,ymax=ystart;
		    float kmax = k->get(xmax,ymax,ai);
		    yi = ystart;
		    int loopsz = (xstop-xstart)/x_step_right+1;
		    if ( loopsz<0 ) loopsz = -loopsz;
		    for ( int i=0; i<loopsz; i++ )
		    {
			int xi = xstart+i*x_step_right;  
    			float curk = k->get(xi,yi,ai);
			if ( curk>kmax )
			{
			    kmax = curk;
			    xmax = xi;
			    ymax = yi;
			}

			yi += y_step_right;
			if ( yi>ymaxidx )
			    break;
		    }
			
		    float score = k->get(xmax,ymax,ai)*wr;
		    vt->set(xmax,ymax,vt->get(xmax,ymax)+score);
		}
	    }
	}
    }

    for ( int idx=0; idx<=xmaxidx; idx++ )
    {
	const bool xedge = idx<2 || idx>=xmaxidx-1;
	for ( int idy=0; idy<=ymaxidx; idy++ )
    	{
	    const bool yedge = idy<2 || idy>=ymaxidx-1;
	    float maxval, temp;

	    if ( !xedge && !yedge )
	    {
		maxval = mMIN( mMAX(vt->get(idx,idy+1),vt->get(idx,idy+2)),
			       mMAX(vt->get(idx,idy-1),vt->get(idx,idy-2)));

		temp = mMIN( mMAX(vt->get(idx-1,idy-1),vt->get(idx-2,idy-2)),
			     mMAX(vt->get(idx+1,idy+1),vt->get(idx+2,idy+2)));
		if ( maxval<temp ) maxval = temp;
		
		temp = mMIN( mMAX(vt->get(idx+1,idy-1),vt->get(idx+2,idy-2)),
			     mMAX(vt->get(idx-1,idy+1),vt->get(idx-2,idy+2)));
		if ( maxval<temp ) maxval = temp;

	       temp = mMIN( mMAX(vt->get(idx+1,idy),vt->get(idx+2,idy)),
			    mMAX(vt->get(idx-1,idy),vt->get(idx-2,idy)));
	       if ( maxval<temp ) maxval = temp;
	    }
	    else
	    {
		maxval = temp = vt->get( idx, idy );

		if ( idy<2 )
		{
		    if ( idy<ymaxidx-1 )
			maxval = mMAX( vt->get(idx,idy+1), vt->get(idx,idy+2) );
		}
		else
		{
		    if ( idy<ymaxidx-1 )
		    {
			maxval = mMIN( mMAX(vt->get(idx,idy+1),
				    	    vt->get(idx,idy+2)),
				       mMAX(vt->get(idx,idy-1),
					    vt->get(idx,idy-2)));
		    }
		    else
		    {
			maxval = mMAX(vt->get(idx,idy-1),vt->get(idx,idy-2));
		    }
		}

		if ( idx<2 || idy<2 )
		{
		    if ( idx<xmaxidx-1 && idy<ymaxidx-1 )
    			temp = mMAX(vt->get(idx+1,idy+1),vt->get(idx+2,idy+2));
		}
		else 
		{
		    temp = mMAX( vt->get(idx-1,idy-1), vt->get(idx-2,idy-2) );
		}

		if ( maxval<temp ) maxval = temp;

		if ( idx<2 )
		{
		    if ( idy>1 )
    			temp = mMAX(vt->get(idx+1,idy-1),vt->get(idx+2,idy-2));
		}
		else if ( idx>=xmaxidx-1 )
		{
		    if ( idy<ymaxidx-1 )
    			temp = mMAX(vt->get(idx-1,idy+1),vt->get(idx-2,idy+2));
		}
		else
		{
		    if ( idy<2 )
		    {
			if ( idy<ymaxidx-1 )
    			    temp = mMAX( vt->get(idx-1,idy+1),
				    	 vt->get(idx-2,idy+2) );
		    }
		    else 
		    {
			if ( idy>=ymaxidx-1 )
	    		    temp = mMAX( vt->get(idx+1,idy-1),
				         vt->get(idx+2,idy-2) );
			else
			{
			    temp = mMIN( mMAX(vt->get(idx+1,idy-1),
					      vt->get(idx+2,idy-2)),
				         mMAX(vt->get(idx-1,idy+1),
					      vt->get(idx-2,idy+2)));
			}
		    }
		}
		
		if ( maxval<temp ) maxval = temp;

		if ( idx<2 )
		{
		    if ( idx<xmaxidx-1 )
    			temp = mMAX(vt->get(idx+1,idy),vt->get(idx+2,idy));
		}
		else
		{
		    if ( idx>=xmaxidx-1 )
		    {
			temp = mMAX(vt->get(idx-1,idy),vt->get(idx-2,idy));
		    }
		    else
		    {
			temp = mMIN( mMAX(vt->get(idx+1,idy),
				    	  vt->get(idx+2,idy)),
    				     mMAX(vt->get(idx-1,idy),
       					 vt->get(idx-2,idy)));
		    }
		}

	       if ( maxval<temp ) maxval = temp;
	    }

	   res.set( idx, idy, maxval );
    	}
    }


    return true;
}


/*
void FaultOrientation::prepareSkeleton( unsigned char *ip, unsigned char *jp, 
    unsigned long width, unsigned long hight )
{
    for( unsigned long i=0; i<hight; i++ )
    {
	for( unsigned long j=0; j<width; j++)
	{
	    if(ip[i*width+j]>0)
		jp[i*width+j]=1;
	    else
		jp[i*width+j]=0;
	}
    }
}*/

void FaultOrientation::skeletonHilditch( Array2D<char>& input )
{
    const int width = input.info().getSize(0);
    const int hight = input.info().getSize(1);
    const int totalsz = width*hight;

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<char> >, icopy,
	    Array2DImpl<char> (input.info()) );
    if ( !icopy ) return;

    char* tmp = icopy->getData();
    char* data = input.getData();
    for( int i=0; i<totalsz; i++ )
    {
	if( data[i]!=0 )
	{
	    data[i]=1;
	    tmp[i]=1;
	}
    }

    bool shori;
    do
    {
	shori = false;
	/*search all input,if pixel value=-1(gived by last circle),
	  delete the pixel*/
	for( int i=0; i<totalsz; i++ )
	{
	    if( data[i]<0 ) 
		data[i] = 0;
	    tmp[i] = data[i];
	}

	for( int i=1; i<width-1; i++)
	{
	    for( int j=1; j<hight-1; j++)
	    {
		const int pidx = i*hight+j;
		if( data[pidx]!=1 ) //not foreground pixel
		    continue;
		
		//pixel position in input
		int p11 = (i-1)*hight+j-1;//low left
		int p12 = p11 + 1;//left
		int p13 = p12 + 1;//high left
		int p21 = i*hight+j-1;//below
		int p22 = p21 + 1;//center
		int p23 = p22 + 1;//above
		int p31 = (i+1)*hight+j-1;//low right
		int p32 = p31 + 1;//right
		int p33 = p32 + 1;//high right
		
		//4-neighbourhood is all foreground
		if( (tmp[p12] && tmp[p21] && tmp[p23] && tmp[p32])!=0 )
    		    continue;

		//sum of 8-neighbourhood
		const int nrn = tmp[p11] + tmp[p12] + tmp[p13] + tmp[p21] + 
		    tmp[p23] + tmp[p31] + tmp[p32] + tmp[p33];
		//if the sum of 8-neighbourhood <=1,
		////that is to say it is the noise or the end of contour
		////delete the center pixel

		if(nrn <= 1)
		{
    		    data[p22] = 2;
		    continue;
		}

		char n[10];
		n[4] = data[p11];//low left
		n[3] = data[p12];//left
		n[2] = data[p13];//high left
		n[5] = data[p21];//below
		n[1] = data[p23];//above
		n[6] = data[p31];//low right
		n[7] = data[p32];//right
		n[8] = data[p33];//high right
		n[9] = n[1];   //above
		
		int count = 0;
		for( int k=1; k<8; k=k+2 )
		{
    		    if( (!n[k]) && (n[k+1] || n[k+2]) )
			count++;
		}
		if( count!=1 )
		{
    		    data[p22] = 2;//deleted
    		    continue;
		}
		//left pixel=-1
		if( data[p12] == -1 )
		{
    		    data[p12] = 0;
    		    n[3] = 0;
    		    count = 0;
    		    for( int k=1; k<8; k=k+2)
    		    {
			if( (!n[k])&&(n[k+1]||n[k+2]) )
	    		    count++;
    		    }
    		    if( count != 1 )
    		    {
			data[p12] = -1;
			continue;
    		    }
    		    data[p12] = -1;
    		    n[3] = -1;
		}
		//below pixel!=-1
		if( data[p21]!=-1 )
		{
    		    data[p22] = -1;
    		    shori = true;
    		    continue;
		}

		data[p21] = 0;
		n[5] = 0;
		count = 0;
		for( int k=1; k<8; k=k+2 )
		{
    		    if( (!n[k]) && (n[k+1] || n[k+2]) )
			count++;
		}

		if( count == 1 )
		{
    		    data[p21] = -1;
    		    data[p22] = -1;
    		    shori =true;
		}
		else
    		    data[p21] = -1;
	    }
	}	
    }while( shori );
}    
