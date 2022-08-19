/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
