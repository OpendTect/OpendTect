/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril / J.C. Glas
 * DATE     : 2-12-2005
-*/

static const char* rcsID = "$Id: seis_cut_poly.cc,v 1.2 2005-12-23 11:30:38 cvsjaap Exp $";

#include "prog.h"
#include "batchprog.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "progressmeter.h"
#include "separstr.h"
#include "survinfo.h"
#include "ptrman.h"
#include <math.h>


static void addBid( const char* str, TypeSet<BinID>& bids )
{
    if ( !str || !*str ) return;

    FileMultiString fms( str );
    if ( fms.size() < 2 ) return;

    bids += BinID( atoi(fms[0]), atoi(fms[1]) );
}


static bool doCoincide( const Coord& point1, const Coord& point2,
			double eps = mDefEps )
{
    return point1.sqDistance( point2 ) <= eps * eps;
}


static double sgnDistToLine( const Coord& point, 
			     const Coord& dirvec, const Coord& posvec )
{
    const double nolinedist = 0;

    const double dirveclen = dirvec.distance( Coord( 0, 0 ) );
    if ( mIsZero( dirveclen, mDefEps ) )
	return nolinedist;
    const double substpointinlineeqn =   dirvec.y * ( point.x - posvec.x )
				       - dirvec.x * ( point.y - posvec.y );
    return substpointinlineeqn / dirveclen;
}


static bool isRightOfLine( const Coord& point,
			   const Coord& dirvec, const Coord& posvec )
{
    return sgnDistToLine( point, dirvec, posvec ) > 0;
}


static bool isOnLine( const Coord& point,
		      const Coord& dirvec, const Coord& posvec,
		      double eps = mDefEps )
{
    const double signeddist = sgnDistToLine( point, dirvec, posvec );
    return signeddist * signeddist <= eps * eps;
}


static bool isOnHalfLine( const Coord& point,
			  const Coord& dirvec, const Coord& endvec, 
			  double eps = mDefEps )
{
    if ( doCoincide( point, endvec, eps ) )
	return true;
    if ( !isOnLine( point, dirvec, endvec, eps ) )
	return false;
    const Coord rot90dirvec( -dirvec.y, dirvec.x );
    return isRightOfLine( point, rot90dirvec, endvec );
}


static bool isOnSegment( const Coord& point,
			 const Coord& vtx1, const Coord& vtx2, 
			 double eps = mDefEps )
{
    return    isOnHalfLine( point, vtx2 - vtx1, vtx1, eps )
	   && isOnHalfLine( point, vtx1 - vtx2, vtx2, eps );
}


static bool isEdgeCrossing( const Coord& raydir, const Coord& raysrc,
			    const Coord& vtx1, const Coord& vtx2 )
{
    const bool vtx1right = isRightOfLine( vtx1, raydir, raysrc );
    const bool vtx2right = isRightOfLine( vtx2, raydir, raysrc );

    if ( vtx1right && !vtx2right )
	return isRightOfLine( raysrc, vtx2 - vtx1, vtx1 );
    if ( !vtx1right && vtx2right )
	return isRightOfLine( raysrc, vtx1 - vtx2, vtx2 );
    return false;
}


//! Draws (horizontal) ray from a given point to the far (right)
//! and checks for odd number of edge crossings with polygon.

static bool isInside( const TypeSet<Coord>& poly, const Coord& point,
		      double eps = mDefEps, bool onborderisinside = true )
{
    const Coord arbitrarydir( 1, 0 );

    bool nrcrossingsodd = false;
    for ( int idx=0; idx<poly.size(); idx++ )
    {
	const Coord& vtxcurr = poly[ idx ];
	const Coord& vtxnext = poly[ idx+1 < poly.size() ? idx+1 : 0 ] ;

	if ( isOnSegment( point, vtxcurr, vtxnext, eps ) )
	    return onborderisinside;
	if ( isEdgeCrossing( arbitrarydir, point, vtxcurr, vtxnext ) )
	    nrcrossingsodd = !nrcrossingsodd;
    }
    return nrcrossingsodd;
}


bool BatchProgram::go( std::ostream& strm )
{ 
    PtrMan<IOObj> inioobj = getIOObjFromPars( "Input Seismics", false,
				    SeisTrcTranslatorGroup::ioContext(), true );
    PtrMan<IOObj> outioobj = getIOObjFromPars( "Output Seismics", false,
				    SeisTrcTranslatorGroup::ioContext(), true );
    if ( !outioobj || !inioobj )
	return false;

    PtrMan<IOPar> edgesubpar = pars().subselect( "Edge" );
    PtrMan<IOPar> exclsubpar = pars().subselect( "Exclude" );
    if ( !edgesubpar || !exclsubpar )
	{ strm << "No positions defined" << std::endl; return false; }

    TypeSet<BinID> edgebids, exclbids;
    for ( int idx=0; ; idx++ )
    {
	BufferString idxstr; idxstr += idx;
	const char* edgestr = edgesubpar->find( idxstr.buf() );
	const char* exclstr = exclsubpar->find( idxstr.buf() );
	if ( !edgestr && !exclstr )
	    { if ( !idx ) continue; else break; }

	addBid( edgestr, edgebids );
	addBid( exclstr, exclbids );
    }
    if ( edgebids.size() < 3 && exclbids.size() < 1 )
	{ strm << "Less than 3 valid positions defined\n"; return false; }

    TypeSet<Coord> edgecoords;
    for ( int idx=0; idx<edgebids.size(); idx++ )
	edgecoords += SI().transform( edgebids[idx] );

    SeisTrcReader rdr( inioobj );
    SeisTrcWriter wrr( outioobj );

    SeisTrc trc;

    ProgressMeter* pm = new ProgressMeter( strm );
    int nrexcl = 0;
    while ( rdr.get(trc) )
    {
	if ( !isInside( edgecoords, trc.info().coord )
	  || exclbids.indexOf(trc.info().binid) >= 0 )
	    { nrexcl++; continue; }

	if ( wrr.put(trc) )
	    ++(*pm);
	else
	{
	    strm << "\nCannot write: " << wrr.errMsg() << std::endl;
	    return false;
	}
    }

    delete pm;
    strm << "\nExcluded " << nrexcl << " traces" << std::endl;
    return true;
}
