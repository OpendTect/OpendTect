/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2013
 SVN:		$Id$
________________________________________________________________________

-*/


#include "mex.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "ranges.h"
#include "seisparallelreader.h"
#include "seistrctr.h"
#include "survinfo.h"

extern "C" void od_Seis_initStdClasses();

static const char* sInpSeisKey = "Input Seismics";

#define mErrRet( msg ) \
{ mexErrMsgTxt( BufferString(msg,"\n") ); return; }


void mexFunction( int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[] )
{
    if ( nrhs != 1 )
	mErrRet( "Usage: readcbvs parameter-file" );

    if ( nlhs != 1 )
	mErrRet( "One output required." );

    char* progname = (char*)"readcbvs";
    int argc = 1;
    char** argv = (char**)mxCalloc( argc, sizeof(char*) );
    argv[0] = progname;

    SetProgramArgs( argc, argv );
    od_Seis_initStdClasses();

    const BufferString fnm = mxArrayToString( prhs[0] );
    if ( !File::exists(fnm) )
	mErrRet( "Input parameter file name does not exist" );

    IOPar par;
    if ( !par.read(fnm,0) )
	mErrRet( "Cannot read input parameter file" );

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

    BufferString errmsg;
    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    PtrMan<IOObj> ioobj =
	IOM().getFromPar( par, sInpSeisKey, ctxt, false, errmsg );
    if ( !ioobj )
    {
	if ( errmsg.isEmpty() )
	    errmsg = "Cannot find input data information in parameter file";
	mErrRet( errmsg );
    }

    CubeSampling cs;
    cs.usePar( par );

    BufferString csinfo;
    cs.hrg.toString( csinfo );
    mexPrintf( csinfo.buf() ); mexPrintf( "\n" );

    Seis::ParallelReader rdr( *ioobj, cs );
    rdr.execute();

    mexPrintf( "Nr of traces: %d\n", cs.hrg.totalNr() );

    const int nrdim = 3;
    mwSize dims[nrdim];
    for ( int idx=0; idx<nrdim; idx++ )
    {
    }

    plhs[0] = mxCreateNumericArray( nrdim, dims, mxDOUBLE_CLASS, mxREAL );
    double* arrptr = mxGetPr( plhs[0] );
}
