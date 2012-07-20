/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * fault dip calculation based the pca analysis to the fault attributes
 * AUTHOR   : Bo Zhang/Yuancheng Liu
 * DATE     : July 2012
-*/

static const char* rcsID mUnusedVar = "$Id: fingervein.cc,v 1.6 2012-07-20 20:05:32 cvsyuancheng Exp $";

#include "fingervein.h"

#include "arrayndimpl.h"
#include "conncomponents.h"
#include "convolve2d.h"
#include "executor.h"
#include "math2.h"
#include "task.h"

#include "statruncalc.h"
#include "stattype.h"
#include  <iostream>

#define mSigma		3
#define mMinFaultLength	15
#define mThresholdPercent 0.93	
#define mNrThinning 100	

FingerVein::FingerVein( const Array2D<float>& input, float threshold, 
	bool isabove, bool istimeslice, Array2D<bool>& output )
    : input_(input)
    , output_(output)
    , threshold_(threshold)  
    , isabove_(isabove)  
    , istimeslice_(istimeslice)  
{ 
}


bool FingerVein::compute( bool domerge, TaskRunner* tr )
{
    mDeclareAndTryAlloc( Array2DImpl<float>*, score,
	    Array2DImpl<float> (input_.info()) );
    if ( !score ) return false;

    if ( !computeMaxCurvature(*score,mSigma,tr) )
	return false;

    const od_int64 datasz = input_.info().getTotalSz();
    mDeclareAndTryAlloc( Array2DImpl<float>*, tmparr,
	    Array2DImpl<float> (input_.info()) );
    if ( !tmparr ) return false;
    tmparr->copyFrom( *score );
    float* arr = tmparr->getData();
    sort_array(arr,datasz);
    const od_int64 thresholdidx = (od_int64)(mThresholdPercent*datasz);
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

    mDeclareAndTryAlloc( Array2DImpl<bool>*, score_binary,
	    Array2DImpl<bool> (input_.info()) );
    mDeclareAndTryAlloc( Array2DImpl<bool>*, input_hard_threshold,
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
	thinning( *input_hard_threshold );
	removeSmallComponents( *input_hard_threshold );
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

    thinning( output_ );
    removeSmallComponents( output_ );
    
    return true;
}


void FingerVein::removeSmallComponents( Array2D<bool>& data )
{
    ConnComponents cc( data );
    cc.compute();

    const int nrcomps = cc.nrComponents();
    bool* outputarr = data.getData();
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	const TypeSet<int>* comp = cc.getComponent( idx );
	if ( !comp ) continue;

	const int nrnodes = comp->size();
	if ( nrnodes<mMinFaultLength )
	{
	    for ( int idy=0; idy<nrnodes; idy++ )
    		outputarr[(*comp)[idy]] = 0;
	}
    }
}


void FingerVein::thinning( Array2D<bool>& res )
{
    mDeclareAndTryAlloc( Array2DImpl<bool>*, tmp,
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
    mDeclareAndTryAlloc( Array2DImpl<float>*, xtmp,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, ytmp,
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

    mDeclareAndTryAlloc( Array2DImpl<float>*, h,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, hx,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, hy,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, hxx,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, hxy,
	    Array2DImpl<float> (sidesize,sidesize) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, hyy,
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

    mDeclareAndTryAlloc( Array2DImpl<float>*, ftmp,
	    Array2DImpl<float> (inputsz0+sidesize-1,inputsz1+sidesize-1) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, fx,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, fy,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, fxx,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, fxy,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    mDeclareAndTryAlloc( Array2DImpl<float>*, fyy,
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

    mDeclareAndTryAlloc( Array3DImpl<float>*, k,
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
    
    mDeclareAndTryAlloc( Array2DImpl<float>*, vt,
	    Array2DImpl<float> (inputsz0,inputsz1) );
    if ( !vt ) return false;
    vt->setAll(0);

    mDeclareAndTryAlloc( Array2DImpl<int>*, xstep,
	    Array2DImpl<int> (nrangles,2) );
    if ( !xstep ) return false;

    mDeclareAndTryAlloc( Array2DImpl<int>*, ystep,
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

		    int xstart=idx, xstop=idx, ystart=idy, ystop=idy; 
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
			    if ( curk<=0 )
			    {
				xstop = xi-x_step_right;
				ystop = yi-y_step_right;
			    }
			    else if ( yi==ymaxidx )
			    {
				xstop = xi;
				ystop = ymaxidx;
			    }
			    else 
			    {
				xstop = xmaxidx;
				ystop = yi;
			    }
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

		    int xstart=idx, xstop=idx, ystart=idy, ystop=idy; 
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
			    if ( curk<0 )
			    {
				xstop = xi-x_step_right;
				ystop = yi-y_step_right;
			    }
			    else if ( !xi )
			    {
				xstop = 0;
				ystop = yi;
			    }
			    else 
			    {
				xstop = xi;
				ystop = ymaxidx;
			    }
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




