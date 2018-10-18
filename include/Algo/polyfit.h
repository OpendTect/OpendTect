#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2005
________________________________________________________________________

-*/


#include "algomod.h"
#include "typeset.h"


/*!\brief fits an Nth order polynomial to an X,Y data series. Guaranteed return
  is a TypeSet v of size N+1, that you can actually use as in:

  y = v[0] + v[1] * x + v[2] * x^2 + ... + v[N] * x^N

  Note that computations are (necessarily) always done using double precision.

  Partly taken from Manas Sharma's post on bragitoff.com
 */


template <class fT>
TypeSet<fT> polyFit( const fT* x, const fT* y, int nrpts, int order )
{
    TypeSet<fT> ret( order+1, (fT)0 );
    if ( order > nrpts-1 )
    {
	if ( nrpts < 1 )
	    return ret;
	order = nrpts - 1;
	if ( order < 1 )
	    { ret[0] = *y; return ret; }
    }

    const auto nrxpows = 2*order + 1;
    TypeSet<double> xpows( nrxpows, 0 );
    for ( auto ixpow=0; ixpow<nrxpows; ixpow++ )
	for ( auto ipt=0; ipt<nrpts; ipt++ )
	    xpows[ixpow] += std::pow( (double)x[ipt], ixpow );

    TypeSet< TypeSet<double> > normmat( order+1, TypeSet<double>(order+2,0) );
    for ( auto iorder=0; iorder<=order; iorder++ )
	for ( auto jorder=0; jorder<=order; jorder++ )
	    normmat[iorder][jorder] = xpows[iorder+jorder];

    TypeSet<double> yxpows( order+1, 0 );
    for ( auto iorder=0; iorder<=order; iorder++ )
	for ( auto ipt=0; ipt<nrpts; ipt++ )
	    yxpows[iorder] += std::pow( (double)x[ipt], iorder ) * y[ipt];

    for ( auto iorder=0; iorder<=order; iorder++ )
	normmat[iorder][order+1] = yxpows[iorder];

    const auto nreq = order + 1;
    for ( auto ieq=0; ieq<nreq; ieq++ )
	for ( auto jeq=ieq+1;jeq<nreq;jeq++)
	    if ( normmat[ieq][ieq] < normmat[jeq][ieq] )
		for ( auto idx=0; idx<=nreq; idx++ )
		    std::swap( normmat[ieq][idx], normmat[jeq][idx] );

    for ( auto iorder=0; iorder<order; iorder++ )
    {
	for ( auto ieq=iorder+1; ieq<nreq; ieq++ )
	{
	    const auto normrel = normmat[ieq][iorder] / normmat[iorder][iorder];
	    for ( auto idx=0; idx<=nreq; idx++ )
		normmat[ieq][idx] -= normrel * normmat[iorder][idx];
	}
    }

    TypeSet<double> out( ret.size(), 0 );
    const auto nrcoeffs = order + 1;
    for ( auto icoeff=nrcoeffs-1; icoeff!=-1; icoeff-- )
    {
	out[icoeff] = normmat[icoeff][nrcoeffs];
	for ( auto ieq=0; ieq<nreq; ieq++ )
	    if ( icoeff != ieq )
		out[icoeff] -= normmat[icoeff][ieq] * out[ieq];
	out[icoeff] /= normmat[icoeff][icoeff];
    }

    copy( ret, out );
    return ret;
}
