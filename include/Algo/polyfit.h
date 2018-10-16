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

    const int nrxpows = 2*order + 1;
    TypeSet<fT> xpows( nrxpows, (fT)0 );
    for ( int ixpow=0; ixpow<nrxpows; ixpow++ )
	for ( int ipt=0; ipt<nrpts; ipt++ )
	    xpows[ixpow] += std::pow( x[ipt], ixpow );

    TypeSet< TypeSet<fT> > normmat( order+1, TypeSet<fT>(order+2,(fT)0) );
    for ( int iorder=0; iorder<=order; iorder++ )
	for ( int jorder=0; jorder<=order; jorder++ )
	    normmat[iorder][jorder] = xpows[iorder+jorder];

    TypeSet<fT> yxpows( order+1, (fT)0 );
    for ( int iorder=0; iorder<=order; iorder++ )
	for ( int ipt=0; ipt<nrpts; ipt++ )
	    yxpows[iorder] += std::pow( x[ipt], iorder ) * y[ipt];

    for ( int iorder=0; iorder<=order; iorder++ )
	normmat[iorder][order+1] = yxpows[iorder];

    const int nreq = order + 1;
    for ( int ieq=0; ieq<nreq; ieq++ )
	for ( int jeq=ieq+1;jeq<nreq;jeq++)
	    if ( normmat[ieq][ieq] < normmat[jeq][ieq] )
		for ( int idx=0; idx<=nreq; idx++ )
		    std::swap( normmat[ieq][idx], normmat[jeq][idx] );

    for ( int iorder=0; iorder<order; iorder++ )
    {
	for ( int ieq=iorder+1; ieq<nreq; ieq++ )
	{
	    const fT normrel = normmat[ieq][iorder] / normmat[iorder][iorder];
	    for ( int idx=0; idx<=nreq; idx++ )
		normmat[ieq][idx] -= normrel * normmat[iorder][idx];
	}
    }

    const auto nrcoeffs = order + 1;
    for ( int icoeff=nrcoeffs-1; icoeff!=-1; icoeff-- )
    {
	ret[icoeff] = normmat[icoeff][nrcoeffs];
	for ( int ieq=0; ieq<nreq; ieq++ )
	    if ( icoeff != ieq )
		ret[icoeff] -= normmat[icoeff][ieq] * ret[ieq];
	ret[icoeff] /= normmat[icoeff][icoeff];
    }

    return ret;
}
