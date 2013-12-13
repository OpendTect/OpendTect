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
	mErrRet( "Usage: writesegyindex segy_filename OpendTect_name "
		 "[parameter-file]" );

    if ( nlhs != 0 )
	mErrRet( "No output required." );

    char* progname = (char*)"writesegyindex";
    int argc = 1;
    char** argv = (char**)mxCalloc( argc, sizeof(char*) );
    argv[0] = progname;

    SetProgramArgs( argc, argv );
    od_Seis_initStdClasses();

    const BufferString segyfnm = mxArrayToString( prhs[0] );
    if ( !File::exists(segyfnm) )
	mErrRet( "Input SEG-Y file name does not exist" );

    const BufferString indexnm = mxArrayToString( prhs[1] );

    IOPar par;
    if ( nrhs==3 )
    {
	const BufferString parfnm = mxArrayToString( prhs[2] );
	if ( !File::exists(parfnm) )
	    mErrRet( "Input parameter file name does not exist" );

	if ( !par.read(parfnm,0) )
	    mErrRet( "Cannot read input parameter file" );
    }

    BufferString res;
    if ( par.get(sKey::DataRoot(),res) )
    {
	if ( !File::exists(res) || !File::isDirectory(res) )
	    mErrRet( "Given dataroot does not exist or is not a directory" );

	SetEnvVar( "DTECT_DATA", res );
    }
    else
    {
	const BufferString msg( "No dataroot specified. Using: ",
		GetBaseDataDir() );
	mexPrintf( msg );
    }

    if ( par.get(sKey::Survey(),res) )
    {
	FilePath fp( GetBaseDataDir() ); fp.add( res );
	const BufferString surveyfp = fp.fullPath();
	if ( !File::exists(surveyfp) || !File::isDirectory(surveyfp) )
	    mErrRet( "Given survey does not exist" );

	IOMan::setSurvey( res );
    }
    else
    {
	const BufferString msg( "No survey specified. Using: ",
				GetSurveyName() );
	mexPrintf( msg );
    }
    mexEvalString("pause(.001);");

    const MultiID mid = getKey( indexnm );
    IOPar segypars;
    segypars.set( sKey::FileName(), segyfnm );
    segypars.set( SEGY::IO::sKeyTask(), SEGY::IO::sKeyIndex3DVol() );
    segypars.setYN( SEGY::IO::sKeyIs2D(), false );
    segypars.set( sKey::Output(), mid );

    SEGY::FileSpec spec( segyfnm );
    SEGY::FileIndexer indexer( mid, true, spec, false, segypars );
    if ( !indexer.execute() )
	mErrRet( indexer.message() );

    BufferString msg;
    msg.add("Succesfully scanned SEG-Y file. Index file ").add( indexnm )
       .add(" is now available with id ").add( mid ).add(".\n");
    mexPrintf( msg );

    Threads::WorkManager::twm().shutdown();
}
