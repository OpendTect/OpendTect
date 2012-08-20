/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * fault dip calculation based the pca analysis to the fault attributes
 * AUTHOR   : Bo Zhang/Yuancheng Liu
 * DATE     : July 2012
-*/

static const char* rcsID mUnusedVar = "$Id: fingervein.cc,v 1.15 2012-08-20 18:55:39 cvsyuancheng Exp $";

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

#define mNrThinning 20	

class veinSliceCalculator : public ParallelTask
{
public:
veinSliceCalculator( const Array3D<float>& input, FaultAngle& fa,
	int minfltlength, int sigma, float percent, Array3D<bool>& output )
    : input_(input)
    , fa_(fa)  
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

    for ( int idz=start; idz<=stop && shouldContinue(); idz++, addToNrDone(1) )
    {
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
		attr->set( idx, idy, input_.get(idx,idy,idz) );
	}

	fa_.vein_usage( *attr, minfaultlength_, sigma_, percent_, is_t_slic,
		*vein_bina,0 );
	
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
		output_.set( idx, idy, idz, vein_bina->get(idx,idy) );
	}	
    }

    return true;
}

FaultAngle&		fa_;
const Array3D<float>&	input_;
Array3D<bool>&		output_;
int 			sigma_; 
float			percent_;
int			minfaultlength_;
};

class azimuthPcaCalculator : public ParallelTask
{
public:
azimuthPcaCalculator( FaultAngle& fa, const Array3D<bool>& conf_base,
	const Array3D<bool>& conf_upgr, int elem_leng, float null_val,
	Array3D<float>& azimuth_pca )
    : fa_(fa)  
    , conf_base_(conf_base)
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

    for ( int idz=start; idz<=stop && shouldContinue(); idz++,addToNrDone(1) )
    {
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
	    {
		base_bina_sect->set( idx, idy, conf_base_.get(idx,idy,idz) );
		upgr_bina_sect->set( idx, idy, conf_upgr_.get(idx,idy,idz) );
	    }
	}

	fa_.get_component_angle( *base_bina_sect, *upgr_bina_sect, 
		elem_leng_, null_val_, *azimuth_sect );
	
	for ( int idx=0; idx<isz; idx++ )
	{
	    for ( int idy=0; idy<csz; idy++ )
	    {
		azimuth_pca_.set( idx, idy, idz, azimuth_sect->get(idx,idy) );
	    }
	}
    }
    return true;
}

FaultAngle&		fa_;
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

    if ( !computeMaxCurvature(*score,sigma,tr) )
	return false;

    const od_int64 datasz = input_.info().getTotalSz();
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


void FingerVein::thinning( Array2D<bool>& res )
{
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, tmp,
	    Array2DImpl<bool> (res.info()) );
    if ( !tmp ) return;

    for ( int idx=0; idx<mNrThinning; idx++ )
    {
	thinStep( res, *tmp, true );
	thinStep( *tmp, res, false );
    }
}


void FingerVein::thinStep( const Array2D<bool>& input, Array2D<bool>& output,
       bool firststep )
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
	    bool bw11 = input.get(idx,idy);
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
	    
	    int NCondition3 = firststep ? ((bw12 | bw01 | (!bw22)) & bw12)
					: ((bw20 | bw21 | (!bw11)) & bw10);
	    if ( !NCondition3 )
		output.set( idx, idy, 0 );
	}
    }
}


bool FingerVein::computeMaxCurvature( Array2D<float>& res, int sigma, 
	TaskRunner* tr )
{
    const int inputsz0 = input_.info().getSize(0);
    const int xmaxidx = inputsz0 - 1; 
    const int inputsz1 = input_.info().getSize(1);
    const int ymaxidx = inputsz1 - 1; 
    const int winsize = 4*sigma;
    const int sidesize = 2*winsize+1;
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, xtmp,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, ytmp,
	    Array2DImpl<float> (sidesize,sidesize) );
    if ( !xtmp || !ytmp ) 
	return false;

    for ( int idx=0; idx<sidesize; idx++ )
    {
	for ( int idy=0; idy<sidesize; idy++ )
	{
	    xtmp->set( idx, idy, idy-winsize );
	    ytmp->set( idx, idy, idx-winsize );
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

    const float sigma2 = sigma*sigma;
    const float sigma4 = sigma2*sigma2;
    const float coef = 1.0/(2*M_PI*sigma2);
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
    conv2.setX( input_, true );
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
	const float angle = M_PI*idx/nrangles;
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

		float demomenator = Math::PowerOf( 1.0+dir1*dir1, 1.5 );
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
    wr_step += sqrt(float(2*2+0*0));
    wr_step += sqrt(float(2*2+1*1));
    wr_step += sqrt(float(2*2+2*2));
    wr_step += sqrt(float(1*1+2*2));
    wr_step += sqrt(float(0*0+2*2));
    wr_step += sqrt(float(1*1+2*2));
    wr_step += sqrt(float(2*2+2*2));
    wr_step += sqrt(float(2*2+1*1));

    for ( int ai=0; ai<nrangles; ai++ )
    {
	const int y_step_left = xstep->get(ai,0);
	const int x_step_left = ystep->get(ai,0);
	const int y_step_right = xstep->get(ai,1);
	const int x_step_right = ystep->get(ai,1);
	if ( ai==0 )
	{
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
				if ( ystart>ymaxidx )
				    ystart = ymaxidx;
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
	    if ( xedge || idy<2 || idy>=ymaxidx-1 )
	    {
		res.set( idx, idy, 0 );
		continue;
	    }

	    float maxval = mMIN( mMAX(vt->get(idx,idy+1),vt->get(idx,idy+2)),
				 mMAX(vt->get(idx,idy-1),vt->get(idx,idy-2)));

	    float temp = mMIN( mMAX(vt->get(idx-1,idy-1),vt->get(idx-2,idy-2)),
			       mMAX(vt->get(idx+1,idy+1),vt->get(idx+2,idy+2)));
	    if ( maxval<temp ) maxval = temp;
	    
	    temp = mMIN( mMAX(vt->get(idx+1,idy-1),vt->get(idx+2,idy-2)),
			 mMAX(vt->get(idx-1,idy+1),vt->get(idx-2,idy+2)));
	    if ( maxval<temp ) maxval = temp;

	    if ( !istimeslice_ )
	    { 
	       temp = mMIN( mMAX(vt->get(idx+1,idy),vt->get(idx+2,idy)),
			    mMAX(vt->get(idx-1,idy),vt->get(idx-2,idy)));
	       if ( maxval<temp ) maxval = temp;
	    }
	    res.set( idx, idy, maxval );
    	}
    }

    return true;
}

#define mPercent 0.94
#define mNull_val -9
#define mElem_leng 10
#define mSigma 3


FaultAngle::FaultAngle( const Array3D<float>& input )
    : input_(input)
    , threshold_(0)
    , isfltabove_(false)  
    , minfaultlength_(15)  
{
    azimuth_stable_ = new Array3DImpl<float>( input.info() );
    dip_stable_ = new Array3DImpl<float>( input.info() );
    conf_low_ = new Array3DImpl<bool>( input.info() );
    conf_med_ = new Array3DImpl<bool>( input.info() );
    conf_high_ = new Array3DImpl<bool>( input.info() );
    conf_low_->setAll( false );
    conf_med_->setAll( false );
    conf_high_->setAll( false );
}


FaultAngle::~FaultAngle()
{
    delete azimuth_stable_;
    delete dip_stable_;	
    delete conf_low_;
    delete conf_med_;
    delete conf_high_;
}


void FaultAngle::setThreshold( float threshold, bool isabove )
{
    threshold_ = threshold;
    isfltabove_ = isabove;
}


void FaultAngle::setMinFaultLength( int minlenght )
{ minfaultlength_ = minlenght; }


bool FaultAngle::compute2D( TaskRunner* tr )
{
    const int isz = input_.info().getSize(0);
    const int csz = input_.info().getSize(1);
    const int zsz = input_.info().getSize(2);
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
	    float val = isinl ? input_.get(0,idx,idy) : (is_t_slic ? 
		    input_.get(idx,idy,0) : input_.get(idx,0,idy) );
	    attr_sect->set( idx, idy, val );
	}
    }
    vein_usage( *attr_sect, minfaultlength_, mSigma, mPercent, 
	    is_t_slic, *vein_bina, tr );
    
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, azimuth_pca,
	    Array2DImpl<float> (xsz,ysz) );
    azimuth_pca->setAll( mNull_val );
    get_component_angle( *vein_bina, *vein_bina, mElem_leng, mNull_val, 
	    *azimuth_pca );
	
    const int elem_leng_new = (mElem_leng)*4;
    const float uppr_perc = 0.85;
    const float lowr_perc = 0.15;
    const float angl_tole = 10;
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, azimuth_pca_stab,
	    Array2DImpl<float> (xsz,ysz) );
    angle_section_stabilise( *vein_bina, *azimuth_pca, elem_leng_new, 
	    uppr_perc, lowr_perc, angl_tole, mNull_val, *azimuth_pca_stab );
    if ( isinl )
    {
	for ( int i=0; i<xsz; i++ )
	{
	    for ( int j=0; j<ysz; j++ )
	    {
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
		azimuth_stable_->set( i, 0, j, azimuth_pca_stab->get(i,j) );
		conf_low_->set( i, 0, j, vein_bina->get(i,j) );
		conf_med_->set( i, 0, j, vein_bina->get(i,j) );
		conf_high_->set( i ,0, j, vein_bina->get(i,j) );
	    }
	}
    }

    return true;
}


bool FaultAngle::compute( TaskRunner* tr )
{
    if ( input_.info().getSize(0)==1 || input_.info().getSize(1)==1 || 
	 input_.info().getSize(2)==1 )
	return compute2D( tr );

    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, vein_bina_t0,
	    Array3DImpl<bool> (input_.info()) );
    vein_t0_slice( input_, mSigma, mPercent, *vein_bina_t0, tr );

    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, vein_bina_0,
	    Array3DImpl<bool> (input_.info()) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, vein_bina_45,
	    Array3DImpl<bool> (input_.info()) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, vein_bina_90,
	    Array3DImpl<bool> (input_.info()) );
    mDeclareAndTryAlloc( PtrMan<Array3DImpl<bool> >, vein_bina_135,
	    Array3DImpl<bool> (input_.info()) );
    vein_vertical_slice( input_, mSigma, mPercent,  
	   *vein_bina_0, *vein_bina_45, *vein_bina_90, *vein_bina_135 );

    get_fault_confidence( *vein_bina_t0, *vein_bina_0, *vein_bina_45, 
	    *vein_bina_90, *vein_bina_135, *conf_low_, *conf_med_, *conf_high_);

    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, azimuth_pca,
	    Array3DImpl<float> (input_.info()) );
    azimuth_pca_vein( *conf_low_, *conf_med_, mElem_leng, mNull_val, 
	    *azimuth_pca, tr );
    azimuth_stabilise( *conf_low_, *azimuth_pca, mElem_leng, mNull_val, 
	    *azimuth_stable_ );

    mDeclareAndTryAlloc( PtrMan<Array3DImpl<float> >, dip_pca,
	    Array3DImpl<float> (input_.info()) );
    const int wind_size = ceil(4*mSigma);
    dip_pca_vein( *conf_low_, *conf_med_, *azimuth_stable_, wind_size,
	    mElem_leng, mNull_val, *dip_pca );
    dip_stablise( *conf_low_, *azimuth_stable_, *dip_pca, wind_size,
	    mElem_leng, mNull_val, *dip_stable_ );

    return true;
}


void FaultAngle::dip_stablise( const Array3D<bool>& conf_bina,
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

	angle_section_stabilise( *base_bina_sect, *dip_sect, elem_leng_new,
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
	
	angle_section_stabilise( *base_bina_sect, *dip_sect, elem_leng_new,
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
	angle_section_stabilise( *base_bina_sect, *dip_sect, elem_leng_new,
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
	angle_section_stabilise( *base_bina_sect, *dip_sect, elem_leng_new,
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
	arc_set += idx*M_PI/4;
	angle_set += idx*45;
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

void FaultAngle::dip_pca_vein( const Array3D<bool>& conf_base,
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
	get_component_angle( *base_bina_sect, *upgr_bina_sect, 
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
	get_component_angle( *base_bina_sect, *upgr_bina_sect, 
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
	get_component_angle( *base_bina_sect, *upgr_bina_sect, 
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
	get_component_angle( *base_bina_sect, *upgr_bina_sect, 
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
	arc_set += idx*M_PI/4;
	angle_set += idx*45;
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


bool FaultAngle::azimuth_pca_vein( const Array3D<bool>& conf_base,
	const Array3D<bool>& conf_upgr, int elem_leng, float null_val,
	Array3D<float>& azimuth_pca, TaskRunner* tr )
{
    azimuthPcaCalculator apc( *this, conf_base, conf_upgr, elem_leng, null_val, 
	    azimuth_pca );
    return tr ? tr->execute( apc ) : apc.execute();
}


void FaultAngle::get_component_angle( const Array2D<bool>& base_bina_sect,
	const Array2D<bool>& upgr_bina_sect, int elem_leng, float null_val,
	Array2D<float>& azimuth_sect )
{
    azimuth_sect.setAll( null_val );    
    const int csz = base_bina_sect.info().getSize(1);

    ConnComponents cc( base_bina_sect );
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
		const float d1 = x0-x1;
		const float d2 = y0-y1;
		const float dist = Math::Sqrt(d1*d1+d2*d2);
		if ( dist<elem_leng )
		{
		    point_set_x += x1;
		    point_set_y += y1;
		}
	    }
	    if ( point_set_x.size()<2 )
		continue;

	    float angle = angle_pca( point_set_x, point_set_y, null_val );
	    azimuth_sect.set( current_width_index, current_hight_index, angle );
	}
    }
}


void FaultAngle::angle_section_stabilise( const Array2D<bool>& conf_sect,
	const Array2D<float>& angl_sect, int elem_leng_new, float uppr_perc,
	float lowr_perc, float angl_tole, float null_value, 
	Array2D<float>& angl_stab )
{
    const float* anglvals = angl_sect.getData();
    const int totalsz = angl_sect.info().getTotalSz();
    float* angstabvals = angl_stab.getData();
    for ( int idx=0; idx<totalsz; idx++ )
	angstabvals[idx] = anglvals[idx]; return;

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
	int uppr_indx = ceil( (a_index-1)*uppr_perc );
	uppr_indx = mMIN( a_index-1, uppr_indx );
  	int lowr_indx = ceil( (a_index-1)*lowr_perc );
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
		float term1 = odd_set_x[jp]-set_x[kp];
		float term2 = odd_set_y[jp]-set_y[kp];
		float dist = Math::Sqrt( term1*term1+term2*term2 );
		if ( dist<=elem_leng_new )
		{
    		    point_set_x += set_x[kp];
    		    point_set_y += set_y[kp];
		}
	    }

	    if ( point_set_x.size()<2 )
		continue;

	    float angle = angle_pca( point_set_x, point_set_y, null_value );
	    angl_stab.set( odd_set_x[jp], odd_set_y[jp], angle );
	}
	
    	for ( int jp=0; jp<npoint; jp++ )
	{
    	    if ( plus_points_flag[jp]==1 )
		continue;

	    TypeSet<int> point_set_x, point_set_y;
    	    for ( int kp=0; kp<npoint; kp++ )
	    {
		float term1 = set_x[jp]-set_x[kp];
		float term2 = set_y[jp]-set_y[kp];
    		float dist = Math::Sqrt( term1*term1+term2*term2 );
    		if ( dist<=elem_leng_new )
		{
    		    point_set_x += set_x[kp];
                    point_set_y += set_y[kp];
		}
		
	        if ( point_set_x.size()<2 )
		    continue;

		float angl_plus = 
		    angle_pca( point_set_x, point_set_y, null_value );	
		if ( fabs(angl_plus-angl_aver) <= angl_tole )
		    angl_stab.set( set_x[jp], set_y[jp], angl_plus );
	    }
	}
    }
}


float FaultAngle::angle_pca( const TypeSet<int>& point_set_x,
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

	azimuth_dip = atan( eigenvec[1]/eigenvec[0] )*180/M_PI;
	if ( azimuth_dip<0 )
	    azimuth_dip += 180;
    }
    
    return azimuth_dip;
}    


void FaultAngle::azimuth_stabilise( const Array3D<bool>& conf_bina,
	const Array3D<float>& azimuth_orig, int elem_leng, float null_val,
	Array3D<float>& azim_stab )
{
    const int isz = input_.info().getSize(0);
    const int csz = input_.info().getSize(1);
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

    const int zsz = input_.info().getSize(2);
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

	angle_section_stabilise( *base_bina_sect, *angl_orig_sect, 
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


void FaultAngle::get_fault_confidence( const Array3D<bool>& vein_bina_t0,
	const Array3D<bool>& vein_bina_0, const Array3D<bool>& vein_bina_45, 
	const Array3D<bool>& vein_bina_90, const Array3D<bool>& vein_bina_135, 
	Array3D<bool>& conf_low, Array3D<bool>& conf_med, 
	Array3D<bool>& conf_high )
{
    const int isz = input_.info().getSize(0);
    const int csz = input_.info().getSize(1);
    const int zsz = input_.info().getSize(2);

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


void FaultAngle::vein_vertical_slice( const Array3D<float>& input, 
	int sigma, float percent,
	Array3D<bool>& vein_bina_0, Array3D<bool>& vein_bina_45, 
	Array3D<bool>& vein_bina_90, Array3D<bool>& vein_bina_135 )
{
    const int isz = input.info().getSize(0);
    const int csz = input.info().getSize(1);
    const int zsz = input.info().getSize(2);
    const int wind_size = ceil(4*sigma);
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
	    
	//FingerVein fv( *attr_sect0, 0, false, is_t_slic, *vein_bina_sect0 );
	//fv.compute( false, false, 1, 1, sigma, percent, 0 );
	vein_usage( *attr_sect0, minfaultlength_, sigma, percent, is_t_slic, 
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
	    
	//FingerVein fv( *attr_sect90, 0, false, is_t_slic, *vein_bina_sect90 );
	//fv.compute( false, false, 1, 1, sigma, percent, 0 );
	vein_usage( *attr_sect90, minfaultlength_, sigma, percent, is_t_slic, 
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
     
	//FingerVein fv( *attr_sect, 0, false, is_t_slic, *vein_bina_sect );
    	//fv.compute( false, false, 1, 1, sigma, percent, 0 );    
	vein_usage( *attr_sect, minfaultlength_, sigma, percent, is_t_slic, 
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
     
	//FingerVein fv( *attr_sect, 0, false, is_t_slic, *vein_bina_sect );
    	//fv.compute( false, false, 1, 1, sigma, percent, 0 );    
	vein_usage( *attr_sect, minfaultlength_, sigma, percent, is_t_slic, 
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




bool FaultAngle::vein_t0_slice( const Array3D<float>& input, int sigma,
	float percent, Array3D<bool>& output, TaskRunner* tr )
{
    veinSliceCalculator vsc(input,*this,minfaultlength_,sigma,percent,output);
    return tr ? tr->execute(vsc) : vsc.execute();
}


bool FaultAngle::vein_usage( const Array2D<float>& img, int fault_min_length,
			     int sigma, float perc, bool is_t_slic, 
			     Array2D<bool>& vein_bina, TaskRunner* tr )
{
    const int img_w = img.info().getSize(0);	
    const int img_h = img.info().getSize(1);	
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, vein_score,
	    Array2DImpl<float> (img.info()) );
    if ( !vein_score ) return false;

    vein_max_curvature( img, sigma, is_t_slic, *vein_score, tr );
    const float* vein_score_vector = vein_score->getData();

    const od_int64 datasz = img.info().getTotalSz();
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, tmparr,
	    Array2DImpl<float> (img.info()) );
    if ( !tmparr ) return false;
    tmparr->copyFrom( *vein_score );
    float* vein_score_vector_sort = tmparr->getData();
    sort_array(vein_score_vector_sort,datasz);

    const od_int64 thresholdidx = (od_int64)(perc*datasz);
    const float vein_score_threshold = 
	    vein_score_vector_sort[0]<vein_score_vector_sort[datasz-1] ? 
	    vein_score_vector_sort[thresholdidx] : 
	    vein_score_vector_sort[datasz-1-thresholdidx] ;
    
    const float* inputarr = img.getData();
    bool* vbdata = vein_bina.getData();
    for ( int idx=0; idx<datasz; idx++ )
	vbdata[idx] = vein_score_vector[idx] > vein_score_threshold;    
    
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, img_bina,
	    Array2DImpl<bool> (img.info()) );
    if ( !img_bina ) return false;
    bool* imgdata = img_bina->getData();
    for ( int idx=0; idx<datasz; idx++ )
	imgdata[idx] = (mIsUdf(inputarr[idx])) ? false : 
	    ( (isfltabove_ && inputarr[idx]>=threshold_) ||
	      (!isfltabove_ && inputarr[idx]<=threshold_) );   

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, img_bina_thin,
	    Array2DImpl<bool> (img.info()) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, img_comp_thin,
	    Array2DImpl<bool> (img.info()) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<int> >, img_comp,
	    Array2DImpl<int> (img.info()) );
    thinning( *img_bina, *img_bina_thin );
    
    img_comp_thin->setAll( 0 );
    ConnComponents cc( *img_bina );
    cc.compute();
    img_comp->copyFrom( *cc.getLabel() );
    for ( int idx=0; idx<img_w; idx++ )
    {
	for ( int idy=0; idy<img_h; idy++ )
	{
	    const int marker = img_comp->get(idx,idy);
	    if ( marker>0 && img_bina_thin->get(idx,idy) )
		img_comp_thin->set( idx, idy, marker );    
	}	
    }

    ConnComponents cc_thin( *img_comp_thin );
    cc_thin.compute();
    const int nrcomp_thin = cc_thin.nrComponents();
    int* imgcompdata = img_comp->getData();
    bool* imgcompdata_thin = img_comp_thin->getData();
    for ( int idx=0; idx<nrcomp_thin; idx++ )
    {
	const TypeSet<int>* comp = cc_thin.getComponent(idx);
	const int compsz = comp ? comp->size() : 0;
	if ( compsz>=fault_min_length )
	    continue;

	for ( int idy=0; idy<compsz; idy++ )
	{
	    imgcompdata[(*comp)[idy]] = 0;	
	    imgcompdata_thin[(*comp)[idy]] = 0;	
	}
    }
    
    for ( int idx=0; idx<datasz; idx++ )
	imgdata[idx] = imgcompdata[idx] ? 1 : 0;

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_bina_merge,
	    Array2DImpl<bool> (img.info()) );
    vein_bina_merge->copyFrom( vein_bina );
    bool* mergedata = vein_bina_merge->getData();
    for ( int idx=0; idx<datasz; idx++ )
    {
	if ( imgdata[idx]==0 || vbdata[idx]==1 )
	    continue;

	mergedata[idx] = 1;
    }

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_bina_thin,
	    Array2DImpl<bool> (img.info()) );
    thinning( *vein_bina_merge, *vein_bina_thin );
    
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<int> >, vein_comp,
	    Array2DImpl<int> (img.info()) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<bool> >, vein_comp_thin,
	    Array2DImpl<bool> (img.info()) );
    ConnComponents cc1( *vein_bina_merge );
    cc1.compute();
    vein_comp->copyFrom( *cc1.getLabel() );
    vein_comp_thin->setAll( 0 );
    for ( int x=0; x<img_w; x++ )
    {
	for ( int y=0; y<img_h; y++ )
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


void FaultAngle::thinning( const Array2D<bool>& orig, Array2D<bool>& res )
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


void FaultAngle::thinStep( const Array2D<bool>& input, Array2D<bool>& output,
       bool firststep )
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
	    bool bw11 = input.get(idx,idy);
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
	    
	    int NCondition3 = firststep ? ((bw12 | bw01 | (!bw22)) & bw12)
					: ((bw20 | bw21 | (!bw11)) & bw10);
	    if ( !NCondition3 )
		output.set( idx, idy, 0 );
	}
    }
}


bool FaultAngle::vein_max_curvature( const Array2D<float>& input,
	       int sigma, bool is_t_slice, Array2D<float>& res, TaskRunner* tr )
{
    const int inputsz0 = input.info().getSize(0);
    const int xmaxidx = inputsz0 - 1; 
    const int inputsz1 = input.info().getSize(1);
    const int ymaxidx = inputsz1 - 1; 
    const int winsize = 4*sigma;
    const int sidesize = 2*winsize+1;
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, xtmp,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, ytmp,
	    Array2DImpl<float> (sidesize,sidesize) );
    if ( !xtmp || !ytmp ) 
	return false;

    for ( int idx=0; idx<sidesize; idx++ )
    {
	for ( int idy=0; idy<sidesize; idy++ )
	{
	    xtmp->set( idx, idy, idy-winsize );
	    ytmp->set( idx, idy, idx-winsize );
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

    const float sigma2 = sigma*sigma;
    const float sigma4 = sigma2*sigma2;
    const float coef = 1.0/(2*M_PI*sigma2);
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
	const float angle = M_PI*idx/nrangles;
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

		float demomenator = Math::PowerOf( 1.0+dir1*dir1, 1.5 );
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
    wr_step += sqrt(float(2*2+0*0));
    wr_step += sqrt(float(2*2+1*1));
    wr_step += sqrt(float(2*2+2*2));
    wr_step += sqrt(float(1*1+2*2));
    wr_step += sqrt(float(0*0+2*2));
    wr_step += sqrt(float(1*1+2*2));
    wr_step += sqrt(float(2*2+2*2));
    wr_step += sqrt(float(2*2+1*1));

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
	    if ( xedge || idy<2 || idy>=ymaxidx-1 )
	    {
		res.set( idx, idy, 0 );
		continue;
	    }

	    float maxval = mMIN( mMAX(vt->get(idx,idy+1),vt->get(idx,idy+2)),
				 mMAX(vt->get(idx,idy-1),vt->get(idx,idy-2)));

	    float temp = mMIN( mMAX(vt->get(idx-1,idy-1),vt->get(idx-2,idy-2)),
			       mMAX(vt->get(idx+1,idy+1),vt->get(idx+2,idy+2)));
	    if ( maxval<temp ) maxval = temp;
	    
	    temp = mMIN( mMAX(vt->get(idx+1,idy-1),vt->get(idx+2,idy-2)),
			 mMAX(vt->get(idx-1,idy+1),vt->get(idx-2,idy+2)));
	    if ( maxval<temp ) maxval = temp;

	   temp = mMIN( mMAX(vt->get(idx+1,idy),vt->get(idx+2,idy)),
			mMAX(vt->get(idx-1,idy),vt->get(idx-2,idy)));
	   if ( maxval<temp ) maxval = temp;

	   res.set( idx, idy, maxval );
    	}
    }

    return true;
}
