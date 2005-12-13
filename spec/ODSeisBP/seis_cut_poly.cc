/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril / J.C. Glas
 * DATE     : 2-12-2005
-*/

static const char* rcsID = "$Id: seis_cut_poly.cc,v 1.1 2005-12-13 14:40:48 cvsbert Exp $";

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


static bool isInside( const TypeSet<Coord>& poly, const Coord& point )
{   
    // count crossings with polygon when drawing a horizontal line
    // from the given point to the right and check for odd or even

    const bool onborderisinside = true;
   
    bool nrcrossingsodd = false;
    
    for ( int vtx0=0; vtx0<poly.size(); vtx0++ )
    {
	int vtx1 = vtx0 + 1; 
	if ( vtx1 == poly.size() ) 
	    vtx1 = 0;		    // list of vertices is circular
        
	// is point aligned with horizontal edge and between its bounds?
	
	if ( point.y == poly[vtx0].y && point.y == poly[vtx1].y )
	    if ( point.x <= poly[vtx0].x && point.x >= poly[vtx1].x ||
	         point.x <= poly[vtx1].x && point.x >= poly[vtx0].x    ) 
		return onborderisinside;

	// is point within horizontal band covered by slanting edge?

	if ( point.y < poly[vtx0].y && point.y >= poly[vtx1].y ||
	     point.y < poly[vtx1].y && point.y >= poly[vtx0].y    )
	{
	    // intersect edge with horizontal line through point

	    const double xcrossing = poly[vtx0].x + 
				     ( point.y - poly[vtx0].y ) * 
				     ( poly[vtx1].x - poly[vtx0].x ) / 
				     ( poly[vtx1].y - poly[vtx0].y )   ;
	    
	    // does point coincide with the slanting edge?

	    if ( point.x == xcrossing )
		return onborderisinside; 

	    // only count crossings at the right of point

	    if ( point.x < xcrossing )
		nrcrossingsodd = !nrcrossingsodd;
	}
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

    bool clockwise = true; pars().getYN( "Edge.Clockwise", clockwise );

    SeisTrcReader rdr( inioobj );
    SeisTrcWriter wrr( outioobj );

    SeisTrc trc;

    const bool haveedge = edgebids.size() > 2;
    const bool haveexcl = exclbids.size() > 0;

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
