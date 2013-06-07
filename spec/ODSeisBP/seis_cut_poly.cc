/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril 
 * DATE     : 2-12-2005
-*/

static const char* rcsID = "$Id: seis_cut_poly.cc,v 1.12 2010/10/14 09:58:06 cvsbert Exp $";

#include "prog.h"
#include "batchprog.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "polygon.h"
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


static void addCoord( const char* str, ODPolygon<double>& poly )
{
    if ( !str || !*str ) return;

    FileMultiString fms( str );
    if ( fms.size() < 2 ) return;

    BinID bid( toInt(fms[0]), toInt(fms[1]) );
    poly.add( SI().transform( bid ) );
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

    ODPolygon<double> poly;
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
	    poly.add( (*pickset)[idx].pos );
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
	    addCoord( edgestr, poly );
	}
    }

    if ( poly.size() < 3 )
	mErrRet("Less than 3 valid positions defined")


    SeisTrcReader rdr( inioobj );
    SeisTrcWriter wrr( outioobj );

    SeisTrc trc;

    TextStreamProgressMeter pm( strm );
    int nrexcl = 0;
    bool needinside = true; pars().getYN( "Select inside", needinside );
    bool incborder = true; pars().getYN( "Border is inside", incborder );
    while ( rdr.get(trc) )
    {
	const bool inside = poly.isInside( trc.info().coord, incborder, 0.001 );
	if ( (needinside && !inside) || (!needinside && inside) )
	    { nrexcl++; continue; }

	if ( wrr.put(trc) )
	    ++pm;
	else
	{
	    BufferString msg( "\nCannot write: " ); msg += wrr.errMsg();
	    mErrRet( msg );
	}
    }

    pm.setFinished();
    strm << "\nExcluded " << nrexcl << " traces" << std::endl;
    return true;
}
