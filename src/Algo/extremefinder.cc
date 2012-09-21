/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2003
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "extremefinder.h"
#include "mathfunc.h"
#include "ptrman.h"
#include "ranges.h"
#include "undefval.h"

#include <math.h>

BisectionExtremeFinder1D::BisectionExtremeFinder1D(
		  const FloatMathFunction& func_, bool max_,
		  int itermax_, float tol_, const Interval<float>& sinterval,
		  const Interval<float>* linterval )
    : max( max_ )
    , func( func_ )
    , itermax( itermax_ )
    , tol( tol_ )
    , limits_( 0 )
{
    reStart( sinterval, linterval );
}


#define mGetFuncVal(x,variable, retval) \
funcval = func.getValue(x); \
if ( mIsUdf(funcval) ) { isok=false; return retval; } \
variable = max ? -funcval : funcval




void BisectionExtremeFinder1D::reStart( const Interval<float>& sinterval,
				        const Interval<float>* linterval )
{
    float funcval;
    if ( linterval )
    {
	if ( limits_ ) *limits_ = *linterval;
	else limits_ = new Interval<float>(*linterval);
    }
    else if ( limits_ )
    {
	delete limits_;
	limits_ = 0;
    }

//Bracket the interval
    current = sinterval;

    mGetFuncVal( current.start, startfuncval, );
    mGetFuncVal( current.stop, stopfuncval, );
    mGetFuncVal( current.center(), centerfuncval, );
    const float halfwidth = current.width()/2;
    while ( centerfuncval>startfuncval || centerfuncval>stopfuncval )
    {
	if ( startfuncval<stopfuncval )
	{
	    current.start -= halfwidth;
	    if ( limits_ && !limits_->includes(current.start,true) )
	    { isok = false; return; }

	    current.stop -= halfwidth;

	    stopfuncval = centerfuncval;
	    centerfuncval = startfuncval;
	    mGetFuncVal( current.start, startfuncval, );
	}
	else
	{
	    current.stop += halfwidth;
	    if ( limits_ && !limits_->includes(current.stop,true) )
	    { isok = false; return; }

	    current.start += halfwidth;

	    startfuncval = centerfuncval;
	    centerfuncval = stopfuncval;
	    mGetFuncVal( current.stop, stopfuncval, );
	}
    }

    isok = true;
    iter = 0;
}


BisectionExtremeFinder1D::~BisectionExtremeFinder1D()
{
    delete limits_;
}


float BisectionExtremeFinder1D::extremePos() const
{
    return current.center();
}


float BisectionExtremeFinder1D::extremeVal() const
{
    return max ? -centerfuncval : centerfuncval;
}


int BisectionExtremeFinder1D::nrIter() const
{
    return iter;
}


int BisectionExtremeFinder1D::nextStep()
{
    if ( !isok ) return -1;

    float funcval;
    float centerpos = current.center();
    while ( iter<itermax )
    {
	const float width = current.width();
	const float width_4 = width/4;
	if ( width<tol )
	    return 0;

	const float firstquartile = centerpos-width_4;
	const float lastquartile = centerpos+width_4;
	mGetFuncVal( firstquartile, float firstquartilefuncval, -1 );
	mGetFuncVal( lastquartile, float lastquartilefuncval, -1 );

	if ( firstquartilefuncval<lastquartilefuncval )
	{
	    current.stop = centerpos;
	    stopfuncval = centerfuncval;
	    centerpos = firstquartile;
	    centerfuncval = firstquartilefuncval;
	}
	else
	{
	    current.start = centerpos;
	    startfuncval = centerfuncval;
	    centerpos = lastquartile;
	    centerfuncval = lastquartilefuncval;
	}

        iter++;
    }

    return -1;
}

   
#define TINY 1.0e-25
#define SHIFT(a, b, c, d ) (a)=(b); (b)=(c); (c)=(d);
#define SIGN(a,b) ((b) > 0.0 ? fabs(a) : -fabs(a))
#define CGOLD 0.3819660f
#define GOLD 1.618034f
#define GLIMIT 100.0f


ExtremeFinder1D::ExtremeFinder1D( const FloatMathFunction& func, bool max,
		  int itermax, float tol, const Interval<float>& sinterval,
		  const Interval<float>* linterval )
    : max_( max )
    , func_( func )
    , itermax_( itermax )
    , tol_( tol )
    , limits_( 0 )
{
    reStart( sinterval, linterval );
}

#undef mGetFuncVal
#define mGetFuncVal( xpos ) \
( max_ ? -func_.getValue( xpos ) : func_.getValue( xpos ) )


#define mInitReturn \
    a_ = mMIN(ax_,cx_); \
    b_ = mMAX(ax_,cx_); \
    x_=w_=v_=bx_; \
    fw_=fv_=fx_= mGetFuncVal( x_ ); \
    return



void ExtremeFinder1D::reStart( const Interval<float>& sinterval,
       			       const Interval<float>* linterval )
{
    if ( linterval )
    {
	if ( limits_ ) *limits_ = *linterval;
	else limits_ = new Interval<float>(*linterval);
    }
    else if ( limits_ )
    {
	delete limits_;
	limits_ = 0;
    }

    ax_ = sinterval.start;
    bx_ = sinterval.center();
    cx_ = sinterval.stop;
    iter_ = 0;
    e_ = 0;

    float fa = mGetFuncVal( ax_ );
    float fb = mGetFuncVal( bx_ );
    if ( fb>fa )
    {
	float ddummy;
	mSWAP( ax_, bx_, ddummy );
	float fdummy;
	mSWAP(fb,fa,fdummy);
    }

    cx_=bx_+GOLD*(bx_-ax_);
    float fc = mGetFuncVal( cx_ );
    while ( fb>fc )
    {
	const float r =(bx_-ax_)*(fb-fc);
	const float s =(bx_-cx_)*(fb-fa);
	float q = (float) ( (bx_)-((bx_-cx_)*s-(bx_-ax_)*r)/
				    (2.0*SIGN(mMAX(fabs(s-r),TINY),s-r)) );
	const float ulim = bx_+GLIMIT*(cx_-bx_);
	float fq;
	if ( (bx_-q)*(q-cx_) > 0.0 )
	{
	    fq = mGetFuncVal( q );
	    if ( fq<fc )
	    {
		ax_ = bx_;
		bx_ = q;

		mInitReturn;
	    }
	    else if ( fq>fb )
	    {
		cx_ = q;
		mInitReturn;
	    }

	    q = cx_+GOLD*(cx_-bx_);
	    fq = mGetFuncVal( q );
	}
	else if ( (cx_-q)*(q-ulim) > 0.0 )
	{
	    fq = mGetFuncVal( q );
	    if ( fq<fc)
	    {
		SHIFT(bx_,cx_,q,cx_+GOLD*(cx_-bx_));
		SHIFT(fb,fc,fq, mGetFuncVal( q ) );
	    }
	}
	else if ( (q-ulim)*(ulim-cx_)>=0.0 )
	{
	    q = ulim;
	    fq = mGetFuncVal( q );
	}
	else
	{
	    q = cx_+GOLD*(cx_-bx_);
	    fq = mGetFuncVal( q );
	}

	SHIFT(ax_,bx_,cx_,q);
	SHIFT(fa,fb,fc,fq);
    }

    mInitReturn;
}


ExtremeFinder1D::~ExtremeFinder1D()
{
    delete limits_;
}


float ExtremeFinder1D::extremePos() const
{
    return x_;
}


float ExtremeFinder1D::extremeVal() const
{
    return max_ ? -fx_ : fx_;
}


int ExtremeFinder1D::nrIter() const
{
    return iter_;
}


int ExtremeFinder1D::nextStep()
{
    while ( true )
    {
	const float xm = 0.5f*(a_+b_);
	const float tol1 = (float)( tol_*fabs(x_)*mDefEps );   
	const float tol2 = 2*tol1;
	if ( fabs(x_-xm)<= (tol2-0.5*(b_-a_)))
	    return 0;

	if ( fabs(e_)>tol1 )
	{
	    const float r = (x_-w_)*(fx_-fv_);
	    float q = (x_-v_)*(fx_-fw_);
	    float p = (x_-v_)*q-(x_-w_)*r;
	    q = 2*(q-r);
	    if ( q>0 ) p = -p;
	    q = fabs( q );
	    const float etemp = e_;
	    e_ = d_;

	    if ( fabs(p)>fabs(0.5*q*etemp) || p<=q*(a_-x_) || p>=q*(b_-x_))
	    {
		e_ = x_>=xm ? a_-x_ : b_-x_;
		d_ = CGOLD*e_;
	    }
	    else
	    {
		d_ = p/q;
		u_ = x_+d_;
		if ( u_-a_ < tol2 || b_-u_<tol2 )
		    d_ = SIGN(tol1, xm-x_ );
	    }
	}
	else
	{
	    e_ = x_>=xm ? a_-x_ : b_-x_;
	    d_ = CGOLD*e_;
	}

	u_ = fabs(d_)>=tol1 ? x_+d_ : x_+SIGN(tol1,d_);
	const float fu = mGetFuncVal( u_ );
	if ( fu<fx_ )
	{
	    if ( u_>=x_ )
		a_=x_;
	    else
		b_=x_;
	    SHIFT(v_,w_,x_,u_);
	    SHIFT(fv_,fw_,fx_,fu);
	    //We have a new estimate - return
	    return limits_ && limits_->includes(x_,true) ? 1 : 0;
	}
	else
	{
	    if ( u_<x_ )
		a_=u_;
	    else
		b_=u_;

	    if ( fu<=fw_ || w_==x_ )
	    {
		v_ = w_;
		w_ = u_;
		fv_ = fw_;
		fw_ = fu;
	    }
	    else if ( fu<=fv_ || v_==x_ || v_==w_ )
	    {
		v_ = u_;
		fv_ = fu;
	    }
	}

	if ( iter_++>itermax_ )
	    return -1;
    }
}

   
ExtremeFinderND::ExtremeFinderND( const FloatMathFunctionND& func, bool max,
				  int itermax )
    : p_( new float[func.getNrDim()] )
    , iter_( 0 )
    , ftol_( 1e-3 )
    , pt_( 0 )
    , max_( max )
    , func_( func )
    , itermax_( itermax )
    , n_( func.getNrDim() )
{
    for ( int idx=0; idx<n_; idx++ )
    {
	float* x = new float[n_];
	for ( int idy=0; idy<n_; idy++ )
	{
	    x[idy] = idy==idx ? 1 : 0;
	}

	xi_ += x;
    }
}


ExtremeFinderND::~ExtremeFinderND()
{
    delete [] p_;
    delete [] pt_;
    for ( int idx=0; idx<n_; idx++ )
    {
	delete [] xi_[idx];
    }
}


int ExtremeFinderND::nextStep()
{
    if ( !iter_ )
    {
	fret_ = mGetFuncVal( p_ );
	pt_ = new float[n_];
	for ( int idx=0; idx<n_; idx++ )
	{
	    pt_[idx]=p_[idx];
	}
    }

    const float fp = fret_;
    int ibig = 0;
    float del = 0.0;

    for ( int dir=0; dir<n_; dir++ )
    {
	ArrPtrMan<float> xit = new float [n_];
	for ( int idy=0; idy<n_; idy++ )
	    xit[idy] = xi_[dir][idy];

	const float fptt=fret_;
	fret_ = linExtreme(xit);
	if ( fptt-fret_> del )
	{
	    del = fptt-fret_;
	    ibig = dir;
	}
    }

    if ( 2.0*(fp-fret_) <= ftol_*(fabs(fp)+fabs(fret_))+TINY)
	return 0;

    if ( iter_==itermax_ )
	return -1;

    ArrPtrMan<float> xit = new float [n_];
    ArrPtrMan<float> ptt = new float [n_];
    for ( int idx=0; idx<n_; idx++ )
    {
	ptt[idx] = 2.0f*p_[idx]-pt_[idx];
	xit[idx] = p_[idx]-pt_[idx];
	pt_[idx] = p_[idx];
    }

    float fptt = fret_ = mGetFuncVal( p_ );
    if ( fptt<fp )
    {
	const float t =
	    2.0f*(fp-2.0f*fret_+fptt)*Math::Sqrt(fp-fret_-del)-
		del*Math::Sqrt(fp-fptt);
	if ( t<0 )
	{
	    fret_ = linExtreme( xit );
	    for ( int idx=0; idx<n_; idx++ )
	    {
		xi_[ibig][idx]=xi_[n_-1][idx];
		xi_[n_-1][idx] = xit[idx];
	    }
	}
    }

    iter_++;
    return 1;
}


int ExtremeFinderND::nrIter() const
{
    return iter_;
}


float ExtremeFinderND::linExtreme( float* vec )
{
    AlongVectorFunction<float,float> vecfunc( func_, p_, vec );
    ExtremeFinder1D finder( vecfunc, max_, 100, 1e-4, Interval<float>(0,1), 0 );

    int res = 1;
    while ( res==1 ) res = finder.doStep();

    const float lambdamin = finder.extremePos();

    for ( int idx=0; idx<n_; idx++ )
    {
	vec[idx] *= lambdamin;
	p_[idx] += vec[idx];
    }

    return finder.extremeVal();
}
