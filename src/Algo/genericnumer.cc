/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : 9-3-1999
-*/

#include "genericnumer.h"

#include "math.h"
#include "mathfuncsampler.h"
    
#define ITMAX 100
#define EPS 3.0e-8
   
bool findValue( const MathFunction<float>& func, float x1, float x2, float& res,
		   float targetval, float tol)
{ 
    int iter;
    float a=x1,b=x2,c,d,e,min1,min2;
    float fa=targetval - func.getValue(a);
    float fb=targetval - func.getValue(b);
    float fc,p,q,r,s,tol1,xm;
    void nrerror();

    if (fb*fa > 0.0) return false;
    fc=fb;
    for (iter=1;iter<=ITMAX;iter++) {
	    if (fb*fc > 0.0) {
		    c=a;
		    fc=fa;
		    e=d=b-a;
	    }
	    if (fabs(fc) < fabs(fb)) {
		    a=b;
		    b=c;
		    c=a;
		    fa=fb;
		    fb=fc;
		    fc=fa;
	    }
	    tol1=2.0*EPS*fabs(b)+0.5*tol;
	    xm=0.5*(c-b);
	    if (fabs(xm) <= tol1 || fb == 0.0) 
	    {
		res = b;
		return true;
	    }

	    if (fabs(e) >= tol1 && fabs(fa) > fabs(fb)) {
		    s=fb/fa;
		    if (a == c) {
			    p=2.0*xm*s;
			    q=1.0-s;
		    } else {
			    q=fa/fc;
			    r=fb/fc;
			    p=s*(2.0*xm*q*(q-r)-(b-a)*(r-1.0));
			    q=(q-1.0)*(r-1.0)*(s-1.0);
		    }
		    if (p > 0.0)  q = -q;
		    p=fabs(p);
		    min1=3.0*xm*q-fabs(tol1*q);
		    min2=fabs(e*q);
		    if (2.0*p < (min1 < min2 ? min1 : min2)) {
			    e=d;
			    d=p/q;
		    } else {
			    d=xm;
			    e=d;
		    }
	    } else {
		    d=xm;
		    e=d;
	    }
	    a=b;
	    fa=fb;
	    if (fabs(d) > tol1)
		    b += d;
	    else
		    b += (xm > 0.0 ? fabs(tol1) : -fabs(tol1));
	    fb=targetval - func.getValue(b);
    }
    return false;
}

#undef ITMAX
#undef EPS

float findValueInAperture( const MathFunction<float>& func, float startx, 
			   const TimeGate& aperture, float dx, float target,
			   float tol)
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

    float positivesol = mUndefValue;
    if ( positivefound )
	findValue( func, startx+dist-dx, startx+dist, positivesol, target, tol);

    float negativesol = -mUndefValue;
    if ( negativefound )
	findValue( func, startx-dist, startx-dist+dx, negativesol, target, tol);

    return (fabs(positivesol-startx) > fabs(negativesol-startx) ? negativesol 
				      : positivesol) - startx; 
}


float similarity( const MathFunction<float>& a, const MathFunction<float>& b, 
			 float a1, float b1, float dist, int sz, bool normalize)
{
    MathFunctionSampler<float> sampa(a);
    MathFunctionSampler<float> sampb(b);

    sampa.sd.start = a1;
    sampa.sd.step = dist;
    sampb.sd.start = b1;
    sampb.sd.step = dist;

    return similarity( sampa, sampb, sz, normalize, 0, 0 );
}


#define ITMAX 100
#define CGOLD 0.3819660
#define ZEPS 1.0e-10
#define SIGN(a,b) ((b) > 0.0 ? fabs(a) : -fabs(a))
#define SHFT(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);

float findExtreme( const MathFunction<float>& func, bool minimum, float x1,
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

    return mUndefValue;
}
    
#undef ITMAX
#undef CGOLD
#undef ZEPS
#undef SIGN
