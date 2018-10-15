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


/*!\brief fits an Nth order polynomial to X, Y data series.

  Partly taken from Manas Sharma's post on bragitoff.com
 */


template <class fT>
TypeSet<fT> polyFit( const fT* x, const fT* y, int nrpts, int reqorder )
{
    int order = reqorder;
    if ( order > nrpts-1 )
    {
	if ( nrpts < 1 )
	    return TypeSet<fT>( reqorder, 0 );
	order = nrpts - 1;
	if ( order < 1 )
	{
	    TypeSet<fT> ret( reqorder, 0 );
	    ret[0] = *y;
	    return ret;
	}
    }

    const int nrsigma = 2*order + 1;
    fT sigma[nrsigma];
    for ( int isigma=0; isigma<nrsigma; isigma++ )
    {
	sigma[isigma] = (fT)0;
	for ( int ipt=0; ipt<nrpts; ipt++ )
	    sigma[isigma] += std::pow( x[ipt], isigma );
    }

    fT normmat[order+1][order+2];
    for ( int iorder=0; iorder<=order; iorder++ )
	for ( int jorder=0; jorder<=order; jorder++ )
	    normmat[iorder][jorder] = sigma[iorder+jorder];

    fT sigmaxn[ order+1 ];
    for ( int iorder=0; iorder<=order; iorder++ )
    {
	sigmaxn[iorder] = (fT)0;
	for ( int ipt=0; ipt<nrpts; ipt++ )
	    sigmaxn[iorder] += std::pow( x[ipt], iorder ) * y[ipt];
    }

    for ( int iorder=0; iorder<=order; iorder++ )
	normmat[iorder][order+1] = sigmaxn[iorder];

    const int nreq = order + 1;
    for ( int ieq=0; ieq<nreq; ieq++ )
    {
	for ( int jeq=ieq+1;jeq<nreq;jeq++)
	{
	    if ( std::fabs(normmat[ieq][ieq]) >= std::fabs(normmat[jeq][ieq]) )
		continue;

	    for ( int idx=0; idx<=nreq; idx++ )
		std::swap( normmat[ieq][idx], normmat[jeq][idx] );
	}
    }

    for ( int iorder=0; iorder<order; iorder++ )
    {
	for ( int ieq=iorder+1; ieq<nreq; ieq++ )
	{
	    const fT normrel = normmat[ieq][iorder] / normmat[iorder][iorder];
	    for ( int idx=0; idx<=nreq; idx++ )
		normmat[ieq][idx] -= normrel * normmat[iorder][idx];
	}
    }

    const auto retnrcoeffs = reqorder + 1;
    const auto nrcoeffs = order + 1;
    TypeSet<fT> coeffs( retnrcoeffs, (fT)0 );
    for ( int icoeff=nrcoeffs-1; icoeff!=-1; icoeff-- )
    {
	coeffs[icoeff] = normmat[icoeff][nrcoeffs];
	for ( int ieq=0; ieq<nreq; ieq++ )
	    if ( icoeff != ieq )
		coeffs[icoeff] -= normmat[icoeff][ieq] * coeffs[ieq];
	coeffs[icoeff] /= normmat[icoeff][icoeff];
    }

    return coeffs;
}
