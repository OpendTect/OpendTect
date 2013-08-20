/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2013
 SVN:		$Id$
________________________________________________________________________

-*/

#include "mex.h"

#include "ctxtioobj.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "segybatchio.h"
#include "segyfiledata.h"
#include "segyfiledef.h"
#include "segydirectdef.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "threadwork.h"

extern "C" void od_Seis_initStdClasses();


#define mErrRet( msg ) \
{ mexErrMsgTxt( BufferString(msg,"\n") ); return; }


static MultiID getKey( const char* nm )
{
    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.deftransl = ctxt.toselect.allowtransls_ = "SEGYDirect";
    CtxtIOObj ctio( ctxt );
    IOM().to( ctio.ctxt.getSelKey() );
    ctio.setName( nm );
    IOM().getEntry( ctio );
    if ( ctio.ioobj )
	IOM().commitChanges( *ctio.ioobj );

    return ctio.ioobj ? ctio.ioobj->key() : MultiID::udf();
}


void mexFunction( int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[] )
{
    if ( nrhs < 2 )
	mErrRet( "Usage: writesegyindex segy_filename OpendTect_name" );

    if ( nlhs != 0 )
	mErrRet( "No output required." );

    char* progname = (char*)"writesegyindex";
    int argc = 1;
    char** argv = (char**)mxCalloc( argc, sizeof(char*) );
    argv[0] = progname;

    SetProgramArgs( argc, argv );
    od_Seis_initStdClasses();

    const BufferString fnm = mxArrayToString( prhs[0] );
    if ( !File::exists(fnm) )
	mErrRet( "Input SEG-Y file name does not exist" );

    const BufferString indexnm = mxArrayToString( prhs[1] );

    BufferString msg( "No dataroot specified. Using: ", GetBaseDataDir(), "\n");
    mexPrintf( msg );

    msg = BufferString( "No survey specified. Using: ", GetSurveyName(), "\n" );
    mexPrintf( msg );
    mexEvalString("pause(.001);");

    const MultiID mid = getKey( indexnm );
    IOPar pars;
    pars.set( sKey::FileName(), fnm );
    pars.set( SEGY::IO::sKeyTask(), SEGY::IO::sKeyIndex3DVol() );
    pars.setYN( SEGY::IO::sKeyIs2D(), false );
    pars.set( sKey::Output(), mid );

    SEGY::FileSpec spec( fnm );
    SEGY::FileIndexer indexer( mid, true, spec, false, pars );
    if ( !indexer.execute() )
	mErrRet( indexer.message() );

    msg.setEmpty();
    msg.add("Succesfully scanned SEG-Y file. Index file ").add( indexnm )
       .add(" is now available with id ").add( mid ).add(".\n");
    mexPrintf( msg );

    Threads::WorkManager::twm().shutdown();
}
