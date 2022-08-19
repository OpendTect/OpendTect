/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mex.h"

#define nullptr 0

#include "arrayndimpl.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "matlabarray.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "ranges.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisparallelreader.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "threadwork.h"

extern "C" void od_Seis_initStdClasses();

static const char* sInpSeisKey = "Input Seismics";

#define mErrRet( msg ) \
{ mexErrMsgTxt( BufferString(msg,"\n") ); return; }


void mexFunction( int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[] )
{
    if ( nrhs != 1 )
	mErrRet( "Usage: readcbvs parameter-file" );

    if ( nlhs == 0 )
	mErrRet( "At least one output required." );

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

    SeisIOObjInfo seisinfo( *ioobj );
    const int nrcomponents = seisinfo.nrComponents();

    if ( nlhs != nrcomponents )
    {
	errmsg.set( "Input cube has " ).add( nrcomponents ).add( " components" )
	      .add( " but " ).add( nlhs ).add( " outputs are provided.\n" );
	errmsg.add( "Please provide " ).add( nrcomponents ).add( " outputs." );
	mErrRet( errmsg );
    }

    TrcKeyZSampling tkzs;
    tkzs.usePar( par );

    mexPrintf( "Reading: %s\n", ioobj->name().buf() );
    BufferString tkzsinfo;
    tkzs.hsamp_.toString( tkzsinfo );
    tkzsinfo.add( "\nZ range: " );
    tkzsinfo.add( tkzs.zsamp_.start ).add( " - " ).add( tkzs.zsamp_.stop )
	  .add( " [" ).add( tkzs.zsamp_.step ).add( "]" );
    mexPrintf( tkzsinfo.buf() ); mexPrintf( "\n" );

    mexPrintf( "Nr of components: %d\n", nrcomponents );
    mexPrintf( "Nr of traces: %d\n", tkzs.hsamp_.totalNr() );
    mexPrintf( "Nr of samples: %d\n", tkzs.nrZ() );

    Seis::SequentialReader rdr( *ioobj, &tkzs );
    if ( !rdr.execute() )
    {
	errmsg = "Error reading input.";
	const uiString uimsg = rdr.uiMessage();
	if ( !uimsg.isEmpty() )
	    errmsg.add("\n").add( uimsg.getFullString() );
	mErrRet( errmsg );
    }

    RegularSeisDataPack* dp = rdr.getDataPack();
    if ( dp->isEmpty() )
	mErrRet( "No data read" );

    for ( int idx=0; idx<dp->nrComponents(); idx++ )
    {
	const Array3D<float>& arr3d = dp->data( idx );
	ArrayNDCopier copier( arr3d );
	copier.init( false );
	copier.execute();

	if ( idx < nlhs )
	    plhs[idx] = copier.getMxArray();
    }

    Threads::WorkManager::twm().shutdown();
}
