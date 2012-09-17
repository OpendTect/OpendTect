/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 9-3-1999
-*/
static const char* rcsID = "$Id: genericnumer.cc,v 1.24 2011/12/06 07:58:48 cvsranojay Exp $";

#include "genericnumer.h"
#include "undefval.h"
#include "idxable.h"
    
#define ITMAX 100
#define EPS 3.0e-8


bool findValue( const FloatMathFunction& func, float x1, float x2, float& res,
		float targetval, float tol)
{
    float f1 = targetval - func.getValue(x1);
    float f2 = targetval - func.getValue(x2);

    if ( f1==0.0 ) { res = x1; return true; }
    if ( f2==0.0 ) { res = x2; return true; }

    if ( f2*f1>0.0 ) return false;

    float x3 = res;
    float f3 = mIsUdf(x3) ? f2 : targetval - func.getValue(x3);
    float e=0, d=0;
    for ( int idx=1; idx<=ITMAX; idx++ )
    {
	if ( f2*f3 > 0.0 )
	{
	    x3 = x1;
	    f3 = f1;
	    e = d = x2-x1;
	}

	if ( fabs(f3)<fabs(f2) )
	{
	    x1 = x2; f1 = f2;
	    x2 = x3; f2 = f3;
	    x3 = x1; f3 = f1;
	}

	const float tol1 = 2.0 * EPS * fabs(x2)+0.5*tol;
	const float xm = 0.5 * (x3-x2);

	if ( fabs(xm)<=tol1 || f2==0.0 ) 
	{
	    res = x2;
	    return true;
	}

	if ( fabs(e)>=tol1 && fabs(f1)>fabs(f2) )
	{
	    const float s = f2/f1;
	    float p, q;
	    if ( x1==x3 )
	    {
		p = 2.0*xm*s;
		q = 1.0-s;
	    }
	    else
	    {
		q = f1/f3;
		const float r = f2/f3;
		p = s*(2.0*xm*q*(q-r)-(x2-x1)*(r-1.0));
		q = (q-1.0)*(r-1.0)*(s-1.0);
	    }

	    if  ( p>0.0 ) q = -q;
	    p = fabs(p);
	    const float min1 = 3.0 * xm * q - fabs( tol1 * q );
	    const float min2 = fabs( e*q );
	    if ( 2.0 * p< ( min1<min2 ? min1 : min2 ) )
	    {
		e = d;
		d = p/q;
	    }
	    else
	    {
		d = xm;
		e = d;
	    }
	}
	else
	{
	    d = xm;
	    e = d;
	}

	x1 = x2; f1 = f2;

	if ( fabs(d)>tol1 )
	    x2 += d;
	else
	    x2 += ( xm>0.0 ? fabs(tol1) : -fabs(tol1) );

	f2 = targetval - func.getValue(x2);
    }

    return false;
}

#undef ITMAX
#undef EPS

float findValueInAperture( const FloatMathFunction& func, float startx, 
			   const Interval<float>& aperture, float dx,
			   float target, float tol )
{
    float aperturecenter = (aperture.start + aperture.stop) / 2;
    const float halfaperture = aperture.stop - aperturecenter;
    startx += (aperture.start + aperture.stop) / 2;

    bool centerispositive = target - func.getValue( startx ) > 0;
  
    float dist = dx; 
    bool negativefound = false;
    bool positivefound = false;
    while ( dist <= halfaperture )
    {
	bool currentispositive = target - func.getValue( startx+dist) > 0;

	if ( centerispositive != currentispositive )
	    positivefound = true;
 
	currentispositive = target - func.getValue( startx-dist) > 0;

	if ( centerispositive != currentispositive )
	    negativefound = true;

	if ( negativefound || positivefound )
	    break;

	dist += dx;
    }
  
    if ( !negativefound && !positivefound )
	return 0;

    float positivesol;  Values::setUdf(positivesol);
    if ( positivefound )
	findValue( func, startx+dist-dx, startx+dist, positivesol, target, tol);

    float negativesol; Values::setUdf(negativesol);
    if ( negativefound )
	findValue( func, startx-dist, startx-dist+dx, negativesol, target, tol);

    return (fabs(positivesol-startx) > fabs(negativesol-startx) ? negativesol 
				      : positivesol) - startx; 
}


float similarity( const FloatMathFunction& a, const FloatMathFunction& b, 
			 float a1, float b1, float dist, int sz, bool normalize)
{
    MathFunctionSampler<float,float> sampa(a);
    MathFunctionSampler<float,float> sampb(b);

    sampa.sd.start = a1;
    sampa.sd.step = dist;
    sampb.sd.start = b1;
    sampb.sd.step = dist;

    return similarity( sampa, sampb, sz, normalize, 0, 0 );
}


float semblance( const ObjectSet<float>& signals, const Interval<int>& samplegate )
{
    const int nrsignals = signals.size()-1;

    float numerator = 0;
    float denominator = 0;
    for ( int zidx=samplegate.start; zidx<=samplegate.stop ; zidx++ )
    {
	float sum = 0;
	for ( int signalidx=nrsignals-1; signalidx>=0; signalidx-- )
	{
	    const float val = signals[signalidx][zidx];
	    if ( mIsUdf(val) )
		continue;

	    sum += val;
	    denominator += val*val;
	}

	numerator += sum*sum;
    }

    return denominator
	? numerator / (nrsignals*denominator)
	: mUdf(float);
}


float semblance( const ObjectSet<float>& signals, int signalsize,
		 const TypeSet<float>& signalstarts,
		 const Interval<int>& samplegate )
{
    const int nrsignals = signals.size();
    const int nrsamples = samplegate.width()+1;

    mAllocVarLenArr( float, cache, nrsignals*nrsamples );
    int offset = 0;
    ObjectSet<float> semblanceinput;

    for ( int signalidx=0; signalidx<nrsignals; signalidx++ )
    {
	const float* signal = signals[signalidx];
	semblanceinput += cache+offset;
	for ( int zidx=samplegate.start; zidx<=samplegate.stop ; zidx++ )
	{
	    const float zpos = signalstarts[signalidx]+zidx;

	    cache[offset] =
		IdxAble::interpolateRegWithUdf(signal,signalsize,zpos,false);

	    offset++;
	}
    }
    
    return semblance( semblanceinput, Interval<int>( 0,  nrsamples-1 ) );
}


double LanczosKernel( int size, double x )
{
    if ( x<=-size || x>=size )
	return 0;

    if ( x==0.0)
	return 1;

    const double x_pi = x * M_PI;

    return size * sin( x_pi ) * sin( x_pi/size ) / ( x_pi*x_pi );
}


#define ITMAX 100
#define CGOLD 0.3819660
#define ZEPS 1.0e-10
#define SIGN(a,b) ((b) > 0.0 ? fabs(a) : -fabs(a))
#define SHFT(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);

float findExtreme( const FloatMathFunction& func, bool minimum, float x1,
		   float x3, float tol)   
{
    float x2 = (x1+x3)/2;

    int iter;
    float a,b,d,etemp,fu,fv,fw,fx,p,q,r,tol1,tol2,u,v,w,x,xm;
    float e=0.0;
    
    a=((x1 < x2) ? x1 : x2);
    b=((x1 > x2) ? x1 : x2);
    x=w=v=x3;
    fw=fv=fx= minimum ? func.getValue(x) : -func.getValue(x);
    for (iter=1;iter<=ITMAX;iter++) 
    {
	xm=0.5*(a+b);
	tol2=2.0*(tol1=tol*fabs(x)+ZEPS);
	if (fabs(x-xm) <= (tol2-0.5*(b-a))) 
	{
	    return x;
	}

	if (fabs(e) > tol1) 
	{
	    r=(x-w)*(fx-fv);
	    q=(x-v)*(fx-fw);
	    p=(x-v)*q-(x-w)*r;
	    q=2.0*(q-r);
	    if (q > 0.0) p = -p;
	    q=fabs(q);
	    etemp=e;
	    e=d;
	    if (fabs(p) >= fabs(0.5*q*etemp) || p <= q*(a-x) || p >= q*(b-x))
		d=CGOLD*(e=(x >= xm ? a-x : b-x));
	    else 
	    {
		d=p/q;
		u=x+d;
		if (u-a < tol2 || b-u < tol2)
			d=SIGN(tol1,xm-x);
	    }
	} 
	else 
	{
	    d=CGOLD*(e=(x >= xm ? a-x : b-x));
	}

	u=(fabs(d) >= tol1 ? x+d : x+SIGN(tol1,d));
	fu = minimum ? func.getValue(u) : -func.getValue(u);
	if (fu <= fx) 
	{
	    if (u >= x) a=x; else b=x;
	    SHFT(v,w,x,u)
	    SHFT(fv,fw,fx,fu)
    	}
	else
	{
	    if (u < x) a=u;
	    else b=u;
	    if (fu <= fw || w == x)
	    {
		v=w;
		w=u;
		fv=fw;
		fw=fu;
	    }
	    else if (fu <= fv || v == x || v == w)
	    {
		v=u;
		fv=fu;
	    }
	}
    }

    return mUdf(float);
}
    
#undef ITMAX
#undef CGOLD
#undef ZEPS
#undef SIGN

unsigned int greatestCommonDivisor( unsigned int val0, unsigned int val1 )
{
    unsigned int shift = 0;

    if ( !val0 ) return val1;
    if ( !val1 ) return val0;

    while ( (val0&1)==0  &&  (val1&1)==0 )
    {
	val0 >>= 1;   
	val1 >>= 1;   
	shift++;   
    }

    do
    {
	if ((val0 & 1) == 0)   
	    val0 >>= 1;        
	else if ((val1 & 1) == 0) 
	    val1 >>= 1;         
	else if (val0 >= val1)   
	    val0 = (val0-val1) >> 1;
	else                   
	    val1 = (val1-val0) >> 1;
    } while ( val0 );

    return val1 << shift; 
}
