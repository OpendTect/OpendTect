/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
-*/

static const char* rcsID = "$Id";

#include "seistrc.h"
#include "seiscbvs.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "iostrm.h"
#include "strmprov.h"
#include "progressmeter.h"
#include "survinfo.h"
#include "filegen.h"
#include "filepath.h"
#include "ptrman.h"
#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisfact.h"


int main( int argc, char** argv )
{
    if ( argc < 3 )
    {
	cerr << "Usage: " << argv[0] << " inpfile outpfile"<< endl
	     << "Format: CBVS." << endl << endl;
	exitProgram( 1 );
    }
    else if ( !File_exists(argv[1]) )
    {
        cerr << argv[1] << " does not exist" << endl;
	exitProgram( 1 );
    }

    BufferString fname( argv[1] );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
    {
	fp.insert( File_getCurrentDir() );
	fname = fp.fullPath();
    }
    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead(new StreamConn(fname,Conn::Read)) )
        { cerr << tri->errMsg() << endl; exitProgram(1); }

    const CBVSReadMgr& rdmgr = *tri->readMgr();
    const CBVSInfo::SurvGeom& geom = rdmgr.info().geom;

    fname = argv[2];
    fp.set( fname );
    if ( !fp.isAbsolute() )
    {
	fp.insert( File_getCurrentDir() );
	fname = fp.fullPath();
    }

    SeisTrc trc;
    PtrMan<CBVSSeisTrcTranslator> tro = CBVSSeisTrcTranslator::getInstance();
    int nrwr = 0;
    ProgressMeter pm( cerr );

    for ( int linenr = geom.start.crl; linenr <= geom.stop.crl;
	    linenr += geom.step.crl ) 
    {
	for ( int trcnr = geom.start.inl; trcnr <= geom.stop.inl;
		trcnr += geom.step.inl ) 
	{
	    pm.update( nrwr );
	    if ( !tri->goTo(BinID(trcnr,linenr)) )
		continue;

	    else if ( !tri->read(trc) )
		{ cerr << "Cannot read " << linenr << '/' << trcnr
		       << endl; exitProgram(1); }

	    Swap( trc.info().binid.inl, trc.info().binid.crl );
	    trc.info().coord = SI().transform( trc.info().binid );

	    if ( !nrwr
		    && !tro->initWrite(new StreamConn(fname,Conn::Write),trc) )
		{ cerr << "Cannot start write!" << endl; exitProgram( 1 ); }

	    if ( !tro->write(trc) )
		{ cerr << "Cannot write!" << endl; exitProgram( 1 ); }

	    nrwr++;
	}
    }

    exitProgram( nrwr ? 0 : 1 ); return 0;
}
