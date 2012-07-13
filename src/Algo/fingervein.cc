/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * fault dip calculation based the pca analysis to the fault attributes
 * AUTHOR   : Bo Zhang/Yuancheng Liu
 * DATE     : July 2012
-*/

static const char* rcsID mUnusedVar = "$Id: fingervein.cc,v 1.1 2012-07-13 20:13:57 cvsyuancheng Exp $";

#include "fingervein.h"

#include "arrayndimpl.h"
#include "convolve2d.h"
#include "executor.h"
#include "math.h"
#include "task.h"

#include "statruncalc.h"
#include "stattype.h"


FingerVein::FingerVein( const Array2D<float>& input, float threshold, 
	bool isabove, bool istimeslice, Array2D<bool>& output )
    : input_(input)
    , output_(output)
    , threshold_(threshold)  
    , isabove_(isabove)  
    , istimeslice_(istimeslice)  
{ 
}


bool FingerVein::compute( TaskRunner* tr )
{
    mDeclareAndTryAlloc( Array2DImpl<float>*, vein_score,
	    Array2DImpl<float> (input_.info()) );
    if ( !vein_score ) return false;

    const od_int64 datasz = input_.info().getTotalSz();
    int sigma = 3; //set
    if ( !computeMaxCurvature(*vein_score,sigma,tr) )
	return false;

    mDeclareAndTryAlloc( Array2DImpl<float>*, tmparr,
	    Array2DImpl<float> (input_.info()) );
    if ( !tmparr ) return false;
    tmparr->copyFrom( *vein_score );
    float* arr = tmparr->getData();
    Stats::CalcSetup scs;
    scs.require(Stats::Median);
    Stats::RunCalc<float> rc( scs );
    for ( od_int64 idx=0; idx<datasz; idx++ )
    {
	if ( !mIsUdf(arr[idx]) && arr[idx]>0 )
	    rc.addValue( arr[idx]);
    }
    const float md_vein = rc.median(); //use for added condition, not now

    sort_array(arr,datasz);
    const od_int64 tmpthresholdidx = (od_int64)(0.93*datasz);
    const float vein_score_threshold = arr[tmpthresholdidx];

    mDeclareAndTryAlloc( Array2DImpl<bool>*, vein_binarise,
	    Array2DImpl<bool> (input_.info()) );
    if ( !vein_binarise ) return false;

    const float* scoredata = vein_score->getData();
    bool* binarisearr = vein_binarise->getData();
    for ( od_int64 idx=0; idx<datasz; idx++ )
	binarisearr[idx] = scoredata[idx]>vein_score_threshold;

    mDeclareAndTryAlloc( Array2DImpl<bool>*, input_hard_threshold,
	    Array2DImpl<bool> (input_.info()) );
    if ( !input_hard_threshold ) return false;

    const float* inputarr = input_.getData();
    bool* inputbinaryarr = input_hard_threshold->getData();
    for ( od_int64 idx=0; idx<datasz; idx++ )
	inputbinaryarr[idx] = isabove_ ? inputarr[idx]>threshold_ 
	    			       : inputarr[idx]<threshold_;
    thinning( *input_hard_threshold );
    bool* thinedinputarr = input_hard_threshold->getData();

    /*TODO: apply connected componet method;*/

    bool* mergedata = output_.getData();
    for ( od_int64 idx=0; idx<datasz; idx++ )
    {
	if ( !binarisearr[idx] && thinedinputarr[idx] ) 
	    //&& scoredata[idx]>vein_md )
    	    mergedata[idx] = 1;
	else
	    mergedata[idx] = binarisearr[idx] ? 1 : 0;
    }

    thinning( output_ );
    //thinning( *vein_binarise );
    
    //again, delete the small fault for vein_binarise use CC
    //again, delete the small fault for vein_binarise_merge use CC

    return true;
}


void FingerVein::thinning( Array2D<bool>& res )
{
    mDeclareAndTryAlloc( Array2DImpl<bool>*, tmp,
	    Array2DImpl<bool> (res.info()) );
    if ( !tmp ) return;

    const int nriterates = 100;
    for ( int idx=0; idx<nriterates; idx++ )
    {
	condition( res, *tmp, true );
	condition( *tmp, res, false );
    }
}


void FingerVein::condition( const Array2D<bool>& input, Array2D<bool>& output,
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

	    const bool b1 = !bw12 && (bw01 || bw01);
	    const bool b2 = !bw01 && (bw00 || bw10);
	    const bool b3 = !bw10 && (bw20 || bw21);
	    const bool b4 = !bw21 && (bw22 || bw12);
	    const char b = b1 + b2 + b3 + b4;
	    if ( b!=1 ) 
		continue;

	    double N1 = double( bw12 | bw02 ) + double( bw01 | bw00 ) + 
			double( bw10 | bw20 ) + double( bw21 | bw22 );
	    double N2 = double( bw02 | bw01 ) + double( bw00 | bw10 ) + 
			double( bw20 | bw21 ) + double( bw22 | bw12 );
	    double NCondition2 = mMIN(N1,N2);
	    if ( NCondition2<2 || NCondition2>3 )
		continue;
	    
	    int NCondition3 = firststep ? ((bw12 | bw01 | (~bw22)) & bw12)
					: ((bw20 | bw21 | (~bw11)) & bw10);
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

    const int halfsidesz = (sidesize+1)/2;
    for ( int idx=0; idx<inputsz0; idx++ )
    {
	int startx = mMAX(idx-halfsidesz+1,0);
	int endx = mMIN(idx+halfsidesz,inputsz0);
	for ( int idy=0; idy<inputsz1; idy++ )
	{
	    int starty = mMAX(idy-halfsidesz+1,0);
    	    int endy = mMIN(idx+halfsidesz,inputsz1);

	    int k = mMAX(halfsidesz-1-idy,0);
	    float sumx=0, sumy=0, sumxx=0, sumxy=0, sumyy=0;
	    for ( int j=starty; j<endy; j++ )
	    {
		int l = mMAX(halfsidesz-1-idx,0);
		for ( int i=startx; i<endx; i++ )
		{
		    const float orival = input_.get(i,j);
		    sumx += hx->get(l,k)*orival;
		    sumy += hy->get(l,k)*orival;
		    sumxx += hxx->get(l,k)*orival;
		    sumxy += hxy->get(l,k)*orival;
		    sumyy += hyy->get(l,k)*orival;
		}
	    }
	    fx->set(idx,idy,sumx);
	    fy->set(idx,idy,sumy);
	    fxx->set(idx,idy,sumxx);
	    fxy->set(idx,idy,sumxy);
	    fyy->set(idx,idy,sumyy);
	}
    }

    /*
    Convolver2D<float> conv2;
    conv2.setX( input_, true );
    conv2.setNormalize( false );
    conv2.setCorrelate( false );

    conv2.setY( *hx, false );
    conv2.setZ( *fx );
    bool isdone = tr ? tr->execute(conv2) : conv2.execute();
    if ( !isdone )
	return false;

    conv2.setY( *hy, false );
    conv2.setZ( *fy );
    isdone = tr ? tr->execute(conv2) : conv2.execute();
    if ( !isdone )
	return false;

    conv2.setY( *hxx, false );
    conv2.setZ( *fxx );
    isdone = tr ? tr->execute(conv2) : conv2.execute();
    if ( !isdone )
	return false;

    conv2.setY( *hxy, false );
    conv2.setZ( *fxy );
    isdone = tr ? tr->execute(conv2) : conv2.execute();
    if ( !isdone )
	return false;

    conv2.setY( *hyy, false );
    conv2.setZ( *fyy );
    isdone = tr ? tr->execute(conv2) : conv2.execute();
    if ( !isdone )
	return false;*/

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

		float dir2 = fxx->get(idx,idy)*angle_set_cos2[idz];
		dir2 += 
		    fxy->get(idx,idy)*2*angle_set_cos[idz]*angle_set_sin[idz];
		dir2 += fyy->get(idx,idy)*angle_set_sin2[idz];

		float demomenator = pow( 1+dir1*dir1, 1.5 );
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
    wr_step += sqrt(2*2+0*0);
    wr_step += sqrt(2*2+1*1);
    wr_step += sqrt(2*2+2*2);
    wr_step += sqrt(1*1+2*2);
    wr_step += sqrt(0*0+2*2);
    wr_step += sqrt(1*1+2*2);
    wr_step += sqrt(2*2+2*2);
    wr_step += sqrt(2*2+1*1);

    for ( int ai=0; ai<nrangles; ai++ )
    {
    	for ( int idx=0; idx<inputsz0; idx++ )
    	{
    	    for ( int idy=0; idy<inputsz1; idy++ )
    	    {
		if ( k->get(idx,idy,ai)<=0 )
		    continue;

		int xstart=idx, xstop=idx, ystart=idy, ystop=idy; 
		float wr = wr_step[ai];
    		    
		const int x_step_left = xstep->get(ai,0);
    		const int y_step_left = ystep->get(ai,0);
		const int x_step_right = xstep->get(ai,1);
    		const int y_step_right = ystep->get(ai,1);

		/*detect the start position*/
		if ( ai==0 ) //horizontal case
		{
    		    int x_temp_start = mMAX(idx+x_step_left,0);
		    for ( int xi=x_temp_start; xi>=0; xi = xi+x_step_left )
		    {
			if ( !xi || k->get(xi,idy,ai)<=0 )
			{
			    if ( k->get(xi,idy,ai)<=0 )
				xstart = xi-x_step_left;
			    else
				xstart = 1;
			    break;
			}
			wr += wr_step[ai];
		    }
		}
		else if ( ai>0 && ai<4 )
		{
    		    int xi = mMAX(idx+x_step_left,0);
    		    int y_temp_start = mMAX(idy+y_step_left,0);
		    for ( int yi=y_temp_start; yi>=0; yi += y_step_left )
		    {
			if ( !xi || !yi || k->get(xi,yi,ai)<=0 )
			{
			    if ( k->get(xi,yi,ai)<=0 )
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
		}
		else if ( ai==4 )
		{
		    int y_start_temp = mMAX(idy+y_step_left,0);
		    for ( int yi=y_start_temp; yi>=0; yi += y_step_left )
		    {
			if ( !yi || k->get(idx,yi,ai)<=0 )
			{
			    ystart = k->get(idx,yi,ai)<=0 ? yi-y_step_left : 0;
			    break;
			}
			wr += wr_step[ai];
		    }
		}
		else
		{
    		    int xi = mMAX(idx+x_step_left,0);
    		    int y_temp_start = mMIN(idy+y_step_left,ymaxidx);
		    for ( int yi=y_temp_start; yi<=ymaxidx; yi += y_step_left )
		    {
			if ( !xi || yi==ymaxidx || k->get(xi,yi,ai)<=0 )
			{
			    if ( k->get(xi,yi,ai)<=0 )
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
				ystart = ymaxidx;
			    }
			    break;
			}
			xi += x_step_left;
			if ( xi<0 || xi>xmaxidx )
			    break;
			wr += wr_step[ai];
		    }
		}
		
		/*detect the stop position*/
		if ( ai==0 ) //horizontal case
		{
		    int x_temp_start = mMIN(idx+x_step_right,xmaxidx);
		    for ( int xi=x_temp_start; xi<inputsz0; xi +=x_step_right )
		    {
			if ( xi==xmaxidx || k->get(xi,idy,ai)<0 )
			{
			    xstop = k->get(xi,idy,ai)<0 ? xi-x_step_right
							: xmaxidx;
			    break;
			}
			wr += wr_step[ai];
		    }
		}
		else if ( ai>0 && ai<4 )
		{
		    int xi = mMIN(idx+x_step_right,xmaxidx);
		    int y_temp_stop = mMIN(idy+y_step_right,ymaxidx);
		    for ( int yi=y_temp_stop; yi<inputsz1; yi +=y_step_right )
		    {
			if ( xi==xmaxidx || yi==ymaxidx || k->get(xi,yi,ai)<0 )
			{
			    if ( k->get(xi,yi,ai)<0 )
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
		}
		else if ( ai==4 )
		{
		    int y_end_temp = mMIN(idy+y_step_right,ymaxidx);
		    for ( int yi=y_end_temp; yi<=ymaxidx; yi +=y_step_right )
		    {
			if ( yi==ymaxidx || k->get(idx,yi,ai)<0 )
			{
			    ystop = k->get(idx,yi,ai)<0 ? yi-y_step_right 
							: ymaxidx;
			    break;
			}
			wr += wr_step[ai];
		    }
		}
		else
		{
		    int xi = mMIN(idx+x_step_right,xmaxidx);
		    int y_temp_stop = mMAX(idy+y_step_right,0);
		    for ( int yi=y_temp_stop; yi>=0; yi +=y_step_right )
		    {
			if ( xi==xmaxidx || !yi || k->get(xi,yi,ai)<0 )
			{
			    if ( k->get(xi,idy,ai)<0 )
			    {
				xstop = xi-x_step_right;
				ystop = yi-y_step_right;
			    }
			    else if ( !yi )
			    {
				xstop = xi;
				ystop = 0;
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
		}

		int xmax,ymax;
		if ( ai==0 )
		{
		    xmax = xstart;
		    ymax = idy;
		    float kmax = k->get(xmax,ymax,ai);
		    for ( int xi=xstart; xi<=xstop; xi +=x_step_right )
		    {
			if ( k->get(xi,ymax,ai)>kmax )
			{
			    kmax = k->get(xi,ymax,ai);
			    xmax = xi;
			}
		    }
		}
		else if ( ai==4 )
		{
		    xmax = idx;
		    ymax = ystart;
		    float kmax = k->get(xmax,ymax,ai);
		    for ( int yi=ystart; yi<=ystop; yi += y_step_right )
		    {
			if ( k->get(xmax,yi,ai)>kmax )
			{
			    kmax = k->get(xmax,yi,ai);
			    ymax = yi;
			}
		    }
		}
		else
		{
		    xmax = xstart;
		    ymax = ystart;
		    float kmax = k->get(xmax,ymax,ai);
		    int xi = xstart;
		    for ( int yi=ystart; yi<=ystop && yi>=0; yi +=y_step_right )
		    {
			if ( k->get(xi,yi,ai)>kmax )
			{
			    kmax = k->get(xi,yi,ai);
			    xmax = xi;
			    ymax = yi;
			}
			xi += x_step_right;
			if ( xi<0 || xi>xmaxidx )
			    break;
		    }
		}
		    
		float score = k->get(xmax,ymax,ai)*wr;
		vt->set(xmax,ymax,vt->get(xmax,ymax)+score);
		wr = 0;
    	    }
	}
    }

    for ( int idx=0; idx<=xmaxidx; idx++ )
    {
	for ( int idy=0; idy<=ymaxidx; idy++ )
    	{
	    if ( idx<2 || idy<2 || idx>=xmaxidx-1 || idy>=ymaxidx-1 )
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




