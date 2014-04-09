/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2013
 SVN:		$Id$
________________________________________________________________________

-*/

#include "mex.h"

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

    CubeSampling cs;
    cs.usePar( par );

    mexPrintf( "Reading: %s\n", ioobj->name().buf() );
    BufferString csinfo;
    cs.hrg.toString( csinfo );
    csinfo.add( "\nZ range: " );
    csinfo.add( cs.zrg.start ).add( " - " ).add( cs.zrg.stop )
	  .add( " [" ).add( cs.zrg.step ).add( "]" );
    mexPrintf( csinfo.buf() ); mexPrintf( "\n" );

    mexPrintf( "Nr of components: %d\n", nrcomponents );
    mexPrintf( "Nr of traces: %d\n", cs.hrg.totalNr() );
    mexPrintf( "Nr of samples: %d\n", cs.nrZ() );


    TypeSet<int> comps;
    ObjectSet< Array3D<float> > arrays;
    for ( int idx=0; idx<nrcomponents; idx++ )
    {
	comps += idx;
	mDeclareAndTryAlloc( Array3D<float>*, arr3d,
		Array3DImpl<float>(cs.nrInl(),cs.nrCrl(),cs.nrZ()) );
	if ( !arr3d || arr3d->isEmpty() )
	{
	    delete arr3d;
	    mErrRet( "Cannot allocate enough memory to load input data" );
	}

	arr3d->setAll( mUdf(float) );
	arrays += arr3d;
    }

    Seis::ParallelReader rdr( *ioobj, comps, arrays, cs );
    if ( !rdr.execute() )
    {
	errmsg = "Error reading input.";
	if ( !rdr.errMsg().isEmpty() )
	    errmsg.add("\n").add( rdr.errMsg() );
	mErrRet( errmsg );
    }

    for ( int idx=0; idx<arrays.size(); idx++ )
    {
	const Array3D<float>* arr3d = arrays[idx];
	if ( !arr3d ) continue;

	ArrayNDCopier copier( *arr3d );
	copier.init( false );
	copier.execute();

	if ( idx < nlhs )
	    plhs[idx] = copier.getMxArray();
    }

    Threads::WorkManager::twm().shutdown();
}
