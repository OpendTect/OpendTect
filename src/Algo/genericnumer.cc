/*+
 * AUTHOR   : K. Tingdahl
 * DATE     : 9-3-1999
-*/

#include "genericnumer.h"
#include "mathfunc.h"
#include <math.h>
    
#define ITMAX 100
#define EPS 6e-8
   
bool findValue( const MathFunction& func, float x1, float x2, float& res,
		float targetval, float tol)
{ 
    int iter;
    float a=x1,b=x2,c,d,e,min1,min2;
    float fa=targetval - func.getValue(a);
    float fb=targetval - func.getValue(b);
    float fc,p,q,r,s,tol1,xm;
    void nrerror();

    if ( fb*fa > 0 ) return false;
    fc = fb;
    for ( iter=1; iter<=ITMAX; iter++ )
    {
	if ( fb*fc > 0 )
	    { c = a; fc = fa; e = d = b - a; }
	if ( fabs(fc) < fabs(fb) )
	    { a = b; b = c; c = a; fa = fb; fb = fc; fc = fa; }
	tol1 = EPS * fabs(b) + 0.5*tol;
	xm = 0.5 * (c - b);
	if ( fabs(xm) <= tol1 || fb == 0 ) 
	{
	    res = b;
	    return true;
	}

	if ( fabs(e) < tol1 || fabs(fa) <= fabs(fb) )
	    { d = xm; e = d; }
	else
	{
	    s = fb / fa;
	    if ( a == c )
		{ p = 2 * xm * s; q = 1 - s; }
	    else
	    {
		q = fa / fc; r = fb / fc;
		p = s * (2*xm*q*(q-r) - (b-a)*(r-1));
		q = (q-1) * (r-1) * (s-1);
	    }
	    if ( p > 0 ) q = -q;
	    p = fabs(p);
	    min1 = 3 * xm * q - fabs( tol1 * q );
	    min2 = fabs( e * q );
	    if ( 2*p < (min1 < min2 ? min1 : min2) )
		{ e = d; d = p/q; }
	    else
		{ d = xm; e = d; }
	}
	a = b;
	fa = fb;
	if ( fabs(d) > tol1 )
	    b += d;
	else
	    b += (xm > 0.0 ? fabs(tol1) : -fabs(tol1));

	fb = targetval - func.getValue(b);
    }

    return false;
}


#undef ITMAX
#undef EPS


float findValueInAperture( const MathFunction& func, float startx, 
			   float aperture, float dx, float target, float tol )
{
    bool centerispositive = target - func.getValue( startx ) > 0;
    const float halfaperture = aperture / 2;
  
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


float similarity( const MathFunction& a, const MathFunction& b, 
			 float a1, float b1, float dist, int sz )
{
    float val1, val2;
    double sqdist = 0, sq1 = 0, sq2 = 0;

    for ( int idx=0; idx<sz; idx++ )
    {
	val1 = a.getValue(a1);
	val2 = b.getValue(b1);
	sq1 += val1 * val1;
	sq2 += val2 * val2;
	sqdist += (val1-val2) * (val1-val2);
	
	a1 += dist;
	b1 += dist;
    }

    if ( sq1 + sq2 < 1e-10 ) return 0;
    return 1 - (sqrt(sqdist) / (sqrt(sq1) + sqrt(sq2)));
}
