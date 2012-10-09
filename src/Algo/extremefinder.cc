/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id$";

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
    , limits( 0 )
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
	if ( limits ) *limits = *linterval;
	else limits = new Interval<float>(*linterval);
    }
    else if ( limits )
    {
	delete limits;
	limits = 0;
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
	    if ( limits && !limits->includes(current.start,true) )
	    { isok = false; return; }

	    current.stop -= halfwidth;

	    stopfuncval = centerfuncval;
	    centerfuncval = startfuncval;
	    mGetFuncVal( current.start, startfuncval, );
	}
	else
	{
	    current.stop += halfwidth;
	    if ( limits && !limits->includes(current.stop,true) )
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
    delete limits;
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
#define CGOLD 0.3819660
#define GOLD 1.618034
#define GLIMIT 100.0


ExtremeFinder1D::ExtremeFinder1D( const FloatMathFunction& func_, bool max_,
		  int itermax_, float tol_, const Interval<float>& sinterval,
		  const Interval<float>* linterval )
    : max( max_ )
    , func( func_ )
    , itermax( itermax_ )
    , tol( tol_ )
    , limits( 0 )
{
    reStart( sinterval, linterval );
}


#define mInitReturn \
    a = mMIN(ax,cx); \
    b = mMAX(ax,cx); \
    x=w=v=bx; \
    fw=fv=fx= max ? -func.getValue( x ) : func.getValue( x ); \
    return

void ExtremeFinder1D::reStart( const Interval<float>& sinterval,
       			       const Interval<float>* linterval )
{
    if ( linterval )
    {
	if ( limits ) *limits = *linterval;
	else limits = new Interval<float>(*linterval);
    }
    else if ( limits )
    {
	delete limits;
	limits = 0;
    }

    ax = sinterval.start;
    bx = sinterval.center();
    cx = sinterval.stop;
    iter = 0;
    e = 0;

    float fa = max ? -func.getValue( ax ) : func.getValue( ax );
    float fb = max ? -func.getValue( bx ) : func.getValue( bx );

    if ( fb>fa )
    {
	float ddummy;
	mSWAP( ax, bx, ddummy );
	float fdummy;
	mSWAP(fb,fa,fdummy);
    }

    cx=bx+GOLD*(bx-ax);
    float fc = max ? -func.getValue( cx ) : func.getValue( cx );
    while ( fb>fc )
    {
	const float r =(bx-ax)*(fb-fc);
	const float s =(bx-cx)*(fb-fa);
	float q = (bx)-((bx-cx)*s-(bx-ax)*r)/
				    (2.0*SIGN(mMAX(fabs(s-r),TINY),s-r));
	const float ulim = bx+GLIMIT*(cx-bx);
	float fq;
	if ( (bx-q)*(q-cx) > 0.0 )
	{
	    fq = max ? -func.getValue( q ) : func.getValue( q );
	    if ( fq<fc )
	    {
		ax = bx;
		bx = q;
		fa=fb;
		fb=fq;
		mInitReturn;
	    }
	    else if ( fq>fb )
	    {
		cx = q;
		fc = fq;
		mInitReturn;
	    }

	    q = cx+GOLD*(cx-bx);
	    fq = max ? -func.getValue( q ) : func.getValue( q );
	}
	else if ( (cx-q)*(q-ulim) > 0.0 )
	{
	    fq = max ? -func.getValue( q ) : func.getValue( q );
	    if ( fq<fc)
	    {
		SHIFT(bx,cx,q,cx+GOLD*(cx-bx));
		SHIFT(fb,fc,fq,max ? -func.getValue(q):func.getValue(q) );
	    }
	}
	else if ( (q-ulim)*(ulim-cx)>=0.0 )
	{
	    q = ulim;
	    fq = max ? -func.getValue( q ) : func.getValue( q );
	}
	else
	{
	    q = cx+GOLD*(cx-bx);
	    fq = max ? -func.getValue( q ) : func.getValue( q );
	}

	SHIFT(ax,bx,cx,q);
	SHIFT(fa,fb,fc,fq);
    }

    mInitReturn;
}


ExtremeFinder1D::~ExtremeFinder1D()
{
    delete limits;
}


float ExtremeFinder1D::extremePos() const
{
    return x;
}


float ExtremeFinder1D::extremeVal() const
{
    return max ? -fx : fx;
}


int ExtremeFinder1D::nrIter() const
{
    return iter;
}


int ExtremeFinder1D::nextStep()
{
    while ( true )
    {
	const float xm = 0.5*(a+b);
	const float tol1 = tol*fabs(x)*mDefEps;   
	const float tol2 = 2*tol1;
	if ( fabs(x-xm)<= (tol2-0.5*(b-a)))
	    return 0;

	if ( fabs(e)>tol1 )
	{
	    const float r = (x-w)*(fx-fv);
	    float q = (x-v)*(fx-fw);
	    float p = (x-v)*q-(x-w)*r;
	    q = 2*(q-r);
	    if ( q>0 ) p = -p;
	    q = fabs( q );
	    const float etemp = e;
	    e = d;

	    if ( fabs(p)>fabs(0.5*q*etemp) || p<=q*(a-x) || p>=q*(b-x))
	    {
		e = x>=xm ? a-x : b-x;
		d = CGOLD*e;
	    }
	    else
	    {
		d = p/q;
		u = x+d;
		if ( u-a < tol2 || b-u<tol2 )
		    d = SIGN(tol1, xm-x );
	    }
	}
	else
	{
	    e = x>=xm ? a-x : b-x;
	    d = CGOLD*e;
	}

	u = fabs(d)>=tol1 ? x+d : x+SIGN(tol1,d);
	const float fu = max ? -func.getValue( u ) : func.getValue( u );
	if ( fu<fx )
	{
	    if ( u>=x )
		a=x;
	    else
		b=x;
	    SHIFT(v,w,x,u);
	    SHIFT(fv,fw,fx,fu);
	    //We have a new estimate - return
	    return limits && limits->includes(x,true) ? 1 : 0;
	}
	else
	{
	    if ( u<x )
		a=u;
	    else
		b=u;

	    if ( fu<=fw || w==x )
	    {
		v = w;
		w = u;
		fv = fw;
		fw = fu;
	    }
	    else if ( fu<=fv || v==x || v ==w )
	    {
		v = u;
		fv = fu;
	    }
	}

	if ( iter++>itermax )
	    return -1;
    }
}

   
ExtremeFinderND::ExtremeFinderND( const FloatMathFunctionND& func_, bool max_,
				  int itermax_ )
    : p( new float[func.getNrDim()] )
    , iter( 0 )
    , ftol( 1e-3 )
    , pt( 0 )
    , max( max_ )
    , func( func_ )
    , itermax( itermax_ )
{
    for ( int idx=0; idx<n; idx++ )
    {
	float* x = new float[n];
	for ( int idy=0; idy<n; idy++ )
	{
	    x[idy] = idy==idx ? 1 : 0;
	}

	xi += x;
    }
}


ExtremeFinderND::~ExtremeFinderND()
{
    delete [] p;
    delete [] pt;
    for ( int idx=0; idx<n; idx++ )
    {
	delete [] xi[idx];
    }
}


int ExtremeFinderND::nextStep()
{
    if ( !iter )
    {
	fret = max ? -func.getValue( p ) : func.getValue( p );
	pt = new float[n];
	for ( int idx=0; idx<n; idx++ )
	{
	    pt[idx]=p[idx];
	}
    }

    const float fp = fret;
    int ibig = 0;
    float del = 0.0;

    for ( int dir=0; dir<n; dir++ )
    {
	ArrPtrMan<float> xit = new float [n];
	for ( int idy=0; idy<n; idy++ )
	    xit[idy] = xi[dir][idy];

	const float fptt=fret;
	fret = linExtreme(xit);
	if ( fptt-fret> del )
	{
	    del = fptt-fret;
	    ibig = dir;
	}
    }

    if ( 2.0*(fp-fret) <= ftol*(fabs(fp)+fabs(fret))+TINY)
	return 0;

    if ( iter==itermax )
	return -1;

    ArrPtrMan<float> xit = new float [n];
    ArrPtrMan<float> ptt = new float [n];
    for ( int idx=0; idx<n; idx++ )
    {
	ptt[idx] = 2.0*p[idx]-pt[idx];
	xit[idx] = p[idx]-pt[idx];
	pt[idx] = p[idx];
    }

    float fptt = fret = max ? -func.getValue( p ) : func.getValue( p );
    if ( fptt<fp )
    {
	const float t =
	    2.0*(fp-2.0*fret+fptt)*Math::Sqrt(fp-fret-del)-del*Math::Sqrt(fp-fptt);
	if ( t<0 )
	{
	    fret = linExtreme( xit );
	    for ( int idx=0; idx<n; idx++ )
	    {
		xi[ibig][idx]=xi[n-1][idx];
		xi[n-1][idx] = xit[idx];
	    }
	}
    }

    iter++;
    return 1;
}


int ExtremeFinderND::nrIter() const
{
    return iter;
}


float ExtremeFinderND::linExtreme( float* vec )
{
    AlongVectorFunction<float,float> vecfunc( func, p, vec );
    ExtremeFinder1D finder( vecfunc, max, 100, 1e-4, Interval<float>(0,1), 0 );

    int res = 1;
    while ( res==1 ) res = finder.doStep();

    const float lambdamin = finder.extremePos();

    for ( int idx=0; idx<n; idx++ )
    {
	vec[idx] *= lambdamin;
	p[idx] += vec[idx];
    }

    return finder.extremeVal();
}
