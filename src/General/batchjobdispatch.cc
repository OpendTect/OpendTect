/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchjobdispatch.h"
#include "keystrs.h"
#include "filepath.h"
#include "strmprov.h"
#include "oscommand.h"
#include "oddirs.h"
#include "ascstream.h"
#include "ioman.h"


mImplFactory(Batch::JobDispatcher,Batch::JobDispatcher::factory)

Batch::JobSpec::JobSpec( Batch::JobSpec::ProcType pt )
{
    switch ( pt )
    {
#define mHandlePTCase(typ,pnm) \
	case typ: prognm_ = "od_process_" #pnm
	mHandlePTCase(Attrib,attrib);
	mHandlePTCase(AttribEM,attrib_em);
	mHandlePTCase(Grid2D,2dgrid);
	mHandlePTCase(PreStack,prestack);
	mHandlePTCase(SEGY,segyio);
	mHandlePTCase(T2D,time2depth);
	mHandlePTCase(VelConv,velocityconv);
	mHandlePTCase(Vol,volume);
    }
}


bool Batch::JobDispatcher::go( const Batch::JobSpec& js )
{
    BufferString reason;
    if ( !isSuitedFor(js,&reason) )
	{ errmsg_.set( "Cannot launch job:\n" ).add( reason ); return false; }

    jobspec_ = js;
    if ( !init() )
	return false;

    jobspec_.pars_.update( sKey::LogFile(), jobspec_.execpars_.logfname_ );
    jobspec_.pars_.set( sKey::Survey(), IOM().surveyName() );
    jobspec_.pars_.set( sKey::DataRoot(), GetBaseDataDir() );

    return launch();
}


Batch::SingleJobDispatcher::SingleJobDispatcher()
    : tostdio_(true)
{
}


const char* Batch::SingleJobDispatcher::description() const
{
    return "The job will be executed on one computer, as a whole.";
}


bool Batch::SingleJobDispatcher::init()
{
    const BufferString prognm = FilePath( jobspec_.prognm_ ).fileName();
    tostdio_ = false;
    BufferString& logfnm = jobspec_.execpars_.logfname_;
    bool alreadyhavelogfnm = !logfnm.isEmpty();
    if ( !jobspec_.execpars_.inprogresswindow_ )
    {
	StreamProvider sp( jobspec_.execpars_.logfname_ );
	FixedString spnm( sp.fullName() );
	if ( !alreadyhavelogfnm
	  || spnm==StreamProvider::sStdIO() || spnm==StreamProvider::sStdErr() )
	{
	    logfnm = StreamProvider::sStdIO();
	    tostdio_ = true;
	    alreadyhavelogfnm = false;
	}
    }

    FilePath parfp;
    if ( !alreadyhavelogfnm )
	parfp.set( GetProcFileName( prognm ) );
    else
    {
	parfp.set( logfnm );
	if ( !parfp.isAbsolute() )
	    parfp.setPath( GetProcFileName(0) );
	logfnm = parfp.fullPath();
    }
    parfp.setExtension( ".par" );
    parfnm_ = parfp.fullPath();

    if ( !alreadyhavelogfnm && !tostdio_ )
    {
	FilePath logfp( parfnm_ );
	logfp.setExtension( "_log.txt" );
	logfnm = logfp.fullPath();
    }

    return true;
}


bool Batch::SingleJobDispatcher::launch()
{
    od_ostream parstrm( parfnm_ );
    ascostream astrm( parstrm );
    astrm.putHeader( "Parameters" );
    jobspec_.pars_.putTo( astrm );
    parstrm.close();

    BufferString basiccmd( jobspec_.prognm_, " ", parfnm_ );
    if ( remotehost_.isEmpty() )
    {
	if ( jobspec_.isodprog_ )
	    return ExecODProgram( jobspec_.prognm_, parfnm_ );

	return ExecOSCmd( basiccmd, tostdio_, true );
    }

    BufferString cmd;
    if ( jobspec_.isodprog_ )
    {
	cmd.set( GetExecScript( true ) ).add( remotehost_ ).add( " --rexec " )
	    .add( OSCommand::defaultRemExec() ).add( " --inbg " );
	if ( !__iswin__ && jobspec_.execpars_.prioritylevel_ )
	{
	    const float fnicelvl = jobspec_.execpars_.prioritylevel_ * 19;
	    cmd.add( "--nice " ).add( mNINT32(fnicelvl) ).add( " " );
	}
    }
    cmd.add( basiccmd );

    OSCommand oscomm( cmd, jobspec_.isodprog_ ? "" : remotehost_.buf() );
    return oscomm.execute( cmd );
}
