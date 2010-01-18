/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2010
-*/
 
static const char* rcsID = "$Id: probdenfunc.cc,v 1.1 2010-01-18 16:13:15 cvsbert Exp $";


#include "sampledprobdenfunc.h"
#include "interpol1d.h"
#include "interpol2d.h"
#include "interpol3d.h"


SampledProbDenFunc1D::SampledProbDenFunc1D( const Array1D<float>& a1d )
    : sd_(0,1)
    , bins_(a1d)
{
}

// More constructors


float SampledProbDenFunc1D::value( float pos ) const
{
    const int sz = size( 0 );
    if ( sz < 1 ) return 0;

    const float fidx = sd_.getIndex( pos );
    if ( fidx < -0.5 || fidx > sz-0.5 )
	return 0;

    float v[4];
    const int idx = (int)fidx;
    v[0] = bins_.get( idx < 2 ? 0 : idx-1 );
    v[1] = bins_.get( idx );
    v[2] = idx < sz-1 ? bins_.get( idx + 1 ) : v[1];
    v[3] = idx < sz-2 ? bins_.get( idx + 2 ) : v[2];

    return Interpolate::PolyReg1D<float>(v).apply( fidx - idx );
}


// More classes
