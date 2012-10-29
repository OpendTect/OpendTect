/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/
 
static const char* rcsID mUsedVar = "$Id$";


#include "mathfunc.h"
#include "linear.h"
#include "interpol1d.h"

LineParameters<float>* SecondOrderPoly::createDerivative() const
{
    return new LinePars( b, a*2 );
}


int PointBasedMathFunction::baseIdx( float x ) const
{
    const int sz = x_.size();
    if ( sz < 1 )		return -1;
    const float x0 = x_[0];
    if ( x < x0 )		return -1;
    if ( sz == 1 )		return x >= x0 ? 0 : -1;
    const float xlast = x_[sz-1];
    if ( x >= xlast )		return sz-1;
    if ( sz == 2 )		return 0;

    int ilo = 0; int ihi = sz - 1;
    while ( ihi - ilo > 1 )
    {
	int imid = (ihi+ilo) / 2;
	if ( x < x_[imid] )
	    ihi = imid;
	else
	    ilo = imid;
    }

    return ilo;
}


void PointBasedMathFunction::add( float x, float y )
{
    if ( mIsUdf(x) ) return;

    const int baseidx = baseIdx( x );
    x_ += x; y_ += y;

    const int sz = x_.size();
    if ( baseidx > sz - 3 )
	return;

    float prevx = x; float prevy = y;
    for ( int idx=baseidx+1; idx<sz; idx++ )
    {
	float tmpx = x_[idx]; float tmpy = y_[idx];
	x_[idx] = prevx; y_[idx] = prevy;
	prevx = tmpx; prevy = tmpy;
    }
}


void PointBasedMathFunction::remove( int idx )
{
    if ( idx<0 || idx>=size() )
	return;

    x_.removeSingle( idx );
    y_.removeSingle( idx );
}


float PointBasedMathFunction::outsideVal( float x ) const
{
    if ( extrapol_==None ) return mUdf(float);
    
    const int sz = x_.size();
    
    if ( extrapol_==EndVal || sz<2 )
    {
    	return x-x_[0] < x_[sz-1]-x ? y_[0] : y_[sz-1];
    }
    
    if ( x<x_[0] )
    {
	const float gradient = (y_[1]-y_[0])/(x_[1]-x_[0]);
	return y_[0]+(x-x_[0])*gradient;
    }
    
    const float gradient = (y_[sz-1]-y_[sz-2])/(x_[sz-1]-x_[sz-2]);
    return y_[sz-1] + (x-x_[sz-1])*gradient;
}


#define mInitFn() \
    const int sz = x_.size(); \
    if ( sz < 1 ) return mUdf(float)


float PointBasedMathFunction::snapVal( float x ) const
{
    mInitFn();
    const int baseidx = baseIdx( x );

    if ( baseidx < 0 )
	return y_[0];
    if ( baseidx > sz-2 )
	return y_[sz - 1];
    return x - x_[baseidx] < x_[baseidx+1] - x ? y_[baseidx] : y_[baseidx+1];
}


float PointBasedMathFunction::interpVal( float x ) const
{
    mInitFn();
    if ( x < x_[0] || x > x_[sz-1] )
	return outsideVal(x);
    else if ( sz < 2 )
	return y_[0];

    const int i0 = baseIdx( x );
    const float v0 = y_[i0];
    if ( i0 == sz-1 )
	return v0;

    const float x0 = x_[i0];
    const int i1 = i0 + 1; const float x1 = x_[i1]; const float v1 = y_[i1];
    const float dx = x1 - x0;
    if ( dx == 0 ) return v0;

    const float relx = (x - x0) / dx;
    if ( mIsUdf(v0) || mIsUdf(v1) )
	return relx < 0.5 ? v0 : v1;

    // OK - we have 2 nearest points and they:
    // - are not undef
    // - don't coincide

    if ( itype_ == Linear )
	return v1 * relx + v0 * (1-relx);

    const int im1 = i0 > 0 ? i0 - 1 : i0;
    const float xm1 = im1 == i0 ? x0 - dx : x_[im1];
    const float vm1 = mIsUdf(y_[im1]) ? v0 : y_[im1];

    const int i2 = i1 < sz-1 ? i1 + 1 : i1;
    const float x2 = i2 == i1 ? x1 + dx : x_[i2];
    const float v2 = mIsUdf(y_[i2]) ? v1 : y_[i2];

    return Interpolate::poly1D( xm1, vm1, x0, v0, x1, v1, x2, v2, x );
}
