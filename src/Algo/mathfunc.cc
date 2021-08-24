/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/



#include "mathfunc.h"
#include "linear.h"
#include "valseriesinterpol.h"


LineParameters<float>* SecondOrderPoly::createDerivative() const
{
    return new LinePars( b, a*2 );
}


ValSeriesMathFunc::ValSeriesMathFunc( const ValueSeries<float>& vs, int sz )
    : vs_(vs)
    , sz_(sz)
{}


float ValSeriesMathFunc::getValue( float x ) const
{
    ValueSeriesInterpolator<float> interp( sz_-1 );
    if ( (-1<x && x<0) || (sz_<x && x<sz_+1) )
	interp.extrapol_ = true;

    return interp.value( vs_, x );
}
