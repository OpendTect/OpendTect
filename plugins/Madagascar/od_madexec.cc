/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	R. K. Singh
 Date:		March 2008
 RCS:		$Id: od_madexec.cc,v 1.1 2008-03-31 11:20:50 cvsraman Exp $
________________________________________________________________________

-*/

#include "madstream.h"
#include "batchprog.h"
#include "cubesampling.h"
#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "progressmeter.h"
#include "ptrman.h"
#include "strmprov.h"
#include <iostream>

#include "prog.h"


static const char* sKeyRSFEndOfHeader = "\014\014\004";
static const char* sKeyMadagascar = "Madagascar";
static const char* sKeyInput = "Input";
static const char* sKeyOutput = "Output";
static const char* sKeyProc = "Proc";
static const char* sKeyWrite = "Write";



const char* getProcString( IOPar& pars )
{
    BufferString* ret = new BufferString( "@" );
    IOPar* readpar = pars.subselect(sKeyInput);
    if ( !readpar || !readpar->size() ) return 0;

    BufferStringSet procs;
    pars.get( sKeyProc, procs );
    for ( int pidx=0; pidx<procs.size(); pidx++ )
    {
	BufferString proc = procs.get( pidx );
	if ( pidx ) *ret += " | ";

	*ret += proc;
    }

    IOPar* outpar = pars.subselect( sKeyOutput );
    if ( !outpar || !outpar->size() ) return 0;

    BufferString outtyp = outpar->find( sKey::Type );
    if ( outtyp == sKey::None ) return ret->buf();
    else if ( outtyp == sKeyMadagascar )
    {
	*ret += " out=stdout";
	*ret += " > ";
	*ret += outpar->find( sKey::FileName );
    }
    else
    {
	const bool dowrite = true;
	pars.setYN( sKeyWrite, dowrite );
	pars.set( "Log file", StreamProvider::sStdErr );
	BufferString fname = FilePath::getTempName( "par" );
	std::cerr << "temp File: " << fname << std::endl;
	pars.write( fname, 0 );
	*ret += " | ./odmadexec ";
	*ret += fname;
    }

    return ret->buf();
}


bool processMad( IOPar& pars, std::ostream& strm )
{
    const char* comm = getProcString( pars ); 
    StreamData sd = StreamProvider( comm ).makeOStream();
    ODMad::MadStream mstrm( pars, true );
    if ( !mstrm.putHeader(*sd.ostrm) ) return false;

    TextStreamProgressMeter pm( strm );
    pm.setName( "Madagascar Processing" );
    pm.setMessage( "Processing Traces..." );
    const int trcsize = mstrm.getNrSamples();
    float* trc = new float[trcsize];
    int tracecount = 0;
    while ( mstrm.getNextTrace(trc) )
    {
	for ( int idx=0; idx<trcsize; idx++ )
	{
	    const float val = trc[idx];
	    (*sd.ostrm).write( (const char*) &val, sizeof(val));
	}

	pm.setNrDone( ++tracecount );
    }

    pm.setFinished();
    sd.close();
    return true;
}


bool BatchProgram::go( std::ostream& strm )
{
    bool dowrite = false;
    pars().getYN( sKeyWrite, dowrite );
    if ( dowrite )
    {
	ODMad::MadStream mstrm( pars(), false );
	if ( !mstrm.writeTraces() ) return false;

	StreamProvider sp( argv_[1] );
	sp.remove ( false );
	return true;
    }
    else
	return processMad( pars(), strm );
}



