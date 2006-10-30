/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril / J.C. Glas
 * DATE     : 2-12-2005
-*/

static const char* rcsID = "$Id: seis_cut_poly.cc,v 1.4 2006-10-30 17:03:13 cvsbert Exp $";

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
#include "pickset.h"
#include "picksettr.h"
#include "ptrman.h"
#include <math.h>


static void addCoord( const char* str, TypeSet<Coord>& coords )
{
    if ( !str || !*str ) return;

    FileMultiString fms( str );
    if ( fms.size() < 2 ) return;

    BinID bid( atoi(fms[0]), atoi(fms[1]) );
    coords += SI().transform( bid );
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
		      bool incborder, double eps = mDefEps )
{
    const Coord arbitrarydir( 1, 0 );

    bool nrcrossingsodd = false;
    for ( int idx=0; idx<poly.size(); idx++ )
    {
	const Coord& vtxcurr = poly[ idx ];
	const Coord& vtxnext = poly[ idx+1 < poly.size() ? idx+1 : 0 ] ;

	if ( isOnSegment( point, vtxcurr, vtxnext, eps ) )
	    return incborder;
	if ( isEdgeCrossing( arbitrarydir, point, vtxcurr, vtxnext ) )
	    nrcrossingsodd = !nrcrossingsodd;
    }
    return nrcrossingsodd;
}


#define mErrRet(s) { strm << s << std::endl; return false; }

bool BatchProgram::go( std::ostream& strm )
{ 
    PtrMan<IOObj> inioobj = getIOObjFromPars( "Input Seismics", false,
				    SeisTrcTranslatorGroup::ioContext(), true );
    PtrMan<IOObj> outioobj = getIOObjFromPars( "Output Seismics", true,
				    SeisTrcTranslatorGroup::ioContext(), true );
    if ( !outioobj || !inioobj )
	mErrRet("Need input and output seismics")

    const char* vrtcspsid = pars().find( "Vertices PickSet.ID" );

    TypeSet<Coord> edgecoords;
    if ( vrtcspsid )
    {
	PtrMan<IOObj> ioobj = IOM().get( MultiID(vrtcspsid) );
	if ( !ioobj )
	    mErrRet("Cannot find pickset ID in object manager")
	Translator* tr = ioobj->getTranslator();
	mDynamicCastGet(PickSetTranslator*,pstr,tr)
	if ( !pstr )
	    mErrRet("Invalid object ID (probably not a Pick Set entry")
	PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
	if ( !conn || conn->bad() )
	    mErrRet("Cannot open Pick Set")
	PtrMan<Pick::Set> pickset = new Pick::Set;
	const char* errmsg = pstr->read( *pickset, *conn );
	if ( errmsg && *errmsg )
	    mErrRet("Cannot read Pick Set")

	for ( int idx=0; idx<pickset->size(); idx++ )
	    edgecoords += (*pickset)[idx].pos;
    }

    PtrMan<IOPar> edgesubpar = pars().subselect( "Edge" );
    if ( edgesubpar && edgesubpar->size() )
    {
	for ( int idx=0; ; idx++ )
	{
	    BufferString idxstr; idxstr += idx;
	    const char* edgestr = edgesubpar->find( idxstr.buf() );
	    if ( !edgestr )
		{ if ( !idx ) continue; else break; }
	    addCoord( edgestr, edgecoords );
	}
    }

    if ( edgecoords.size() < 3 )
	mErrRet("Less than 3 valid positions defined")


    SeisTrcReader rdr( inioobj );
    SeisTrcWriter wrr( outioobj );

    SeisTrc trc;

    ProgressMeter* pm = new ProgressMeter( strm );
    int nrexcl = 0;
    bool needinside = true; pars().getYN( "Select inside", needinside );
    bool incborder = true; pars().getYN( "Border is inside", incborder );
    while ( rdr.get(trc) )
    {
	const bool inside = isInside( edgecoords, trc.info().coord, incborder );
	if ( (needinside && !inside) || (!needinside && inside) )
	    { nrexcl++; continue; }

	if ( wrr.put(trc) )
	    ++(*pm);
	else
	{
	    BufferString msg( "\nCannot write: " ); msg += wrr.errMsg();
	    mErrRet( msg );
	}
    }

    delete pm;
    strm << "\nExcluded " << nrexcl << " traces" << std::endl;
    return true;
}
