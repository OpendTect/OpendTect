/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	R. K. Singh
 Date:		March 2008
 RCS:		$Id: od_madexec.cc,v 1.6 2008-04-28 06:36:02 cvsraman Exp $
________________________________________________________________________

-*/

#include "madstream.h"
#include "batchprog.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "progressmeter.h"
#include "seistype.h"
#include "strmprov.h"


static const char* sKeyRSFEndOfHeader = "\014\014\004";
static const char* sKeyMadagascar = "Madagascar";
static const char* sKeyInput = "Input";
static const char* sKeyOutput = "Output";
static const char* sKeyProc = "Proc";
static const char* sKeyWrite = "Write";



const char* getProcString( IOPar& pars, BufferString& errmsg )
{
    static BufferString ret( "@" );
    IOPar* readpar = pars.subselect(sKeyInput);
    if ( !readpar || !readpar->size() )
    { errmsg = "No Input parameters"; return 0; }

    BufferStringSet procs;
    pars.get( sKeyProc, procs );
    const bool isprocessing = procs.size() && !procs.get(0).isEmpty();
    for ( int pidx=0; pidx<procs.size(); pidx++ )
    {
	BufferString proc = procs.get( pidx );
	if ( pidx ) ret += " | ";

	ret += proc;
    }

    IOPar* outpar = pars.subselect( sKeyOutput );
    if ( !outpar || !outpar->size() )
    { errmsg = "No Output parameters"; return 0; }

    BufferString outptyp = outpar->find( sKey::Type );
    if ( outptyp == sKey::None ) return StreamProvider::sStdIO;
    else if ( outptyp == sKeyMadagascar )
    {
	if ( isprocessing )
	{
	    ret += " out=stdout";
	    ret += " > ";
	    ret += outpar->find( sKey::FileName );
	}
	else
	    ret = outpar->find( sKey::FileName );

	return ret.buf();
    }
    
    Seis::GeomType gt = Seis::geomTypeOf( outptyp );
    if ( gt == Seis::Vol || gt == Seis::Line )
    {
	const bool dowrite = true;
	pars.setYN( sKeyWrite, dowrite );
	pars.set( "Log file", StreamProvider::sStdErr );
	BufferString fname = FilePath::getTempName( "par" );
	pars.write( fname, sKey::Pars );
	if ( isprocessing ) ret += " | ";

	BufferString comm = GetExecScript( false );
	comm += " ";
	comm += "odmadexec";
	ret += comm; ret += " "; ret += fname;
	return ret.buf();
    }
    else
    {
	errmsg = "Output Type ";
	errmsg += outptyp;
	errmsg += " not supported";
	return 0;
    }
}


#undef mErrRet
#define mErrRet(s) { strm << "Error: " << s << std::endl; return false; }

bool processMad( IOPar& pars, std::ostream& strm )
{
    ODMad::MadStream mstrm( pars );
    if ( !mstrm.isOK() )
	mErrRet( mstrm.errMsg() );

    BufferString errmsg;
    const char* comm = getProcString( pars, errmsg );
    if ( !comm )
	mErrRet( errmsg );

    StreamData sd;
    if ( !strcmp(comm,StreamProvider::sStdIO) )
	sd.ostrm = &std::cout;
    else
	sd = StreamProvider( comm ).makeOStream();

    if ( !mstrm.putHeader(*sd.ostrm) )
	mErrRet( "Cannot create RSF Header" );

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

    delete [] trc;
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
	ODMad::MadStream mstrm( pars() );
	if ( !mstrm.isOK() )
	    mErrRet( mstrm.errMsg() );

	if ( !mstrm.writeTraces() )
	    mErrRet( mstrm.errMsg() );

	StreamProvider sppar( argv_[1] );
	sppar.remove ( false );
	return true;
    }
    else
	return processMad( pars(), strm );
}

#undef mErrRet
