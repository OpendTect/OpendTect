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
    return launch();
}


const char* Batch::SingleJobDispatcher::description() const
{
    return "The job will be executed on one computer, as a whole.";
}


bool Batch::SingleJobDispatcher::launch()
{
    jobspec_.pars_.update( sKey::LogFile(), logspec_ );
    BufferString parfnm;
    const BufferString prognm = FilePath( jobspec_.prognm_ ).fileName();
    const bool inwin = logspec_ == "window";
    bool tostdio = false;
    if ( !inwin )
    {
	StreamProvider sp( logspec_ );
	FixedString spnm( sp.fullName() );
	if ( logspec_.isEmpty() || spnm == StreamProvider::sStdIO()
				|| spnm == StreamProvider::sStdErr() )
	{
	    logspec_ = StreamProvider::sStdIO();
	    tostdio = true;
	}
    }

    if ( logspec_.isEmpty() || inwin || tostdio )
    {
	FilePath fp( GetProcFileName( prognm ) );
	fp.setExtension( ".par" );
    }
    else
    {
	FilePath fp( logspec_ );
	if ( !fp.isAbsolute() )
	    fp.setPath( GetProcFileName(0) );
	logspec_ = fp.fullPath();
	fp.setExtension( ".par" );
	parfnm = fp.fullPath();
    }

    jobspec_.pars_.update( sKey::LogFile(), logspec_ );
    jobspec_.pars_.set( sKey::Survey(), IOM().surveyName() );
    jobspec_.pars_.set( sKey::DataRoot(), GetBaseDataDir() );

    od_ostream parstrm( parfnm );
    ascostream astrm( parstrm );
    astrm.putHeader( "Parameters" );
    jobspec_.pars_.putTo( astrm );
    parstrm.close();

    BufferString basiccmd( jobspec_.prognm_, " ", parfnm );
    if ( remotehost_.isEmpty() )
    {
	if ( jobspec_.isodprog_ )
	    return ExecODProgram( jobspec_.prognm_, parfnm );

	return ExecOSCmd( basiccmd, tostdio, true );
    }

    BufferString cmd;
    if ( jobspec_.isodprog_ )
	cmd.set( GetExecScript( true ) ).add( remotehost_ ).add( " --rexec " )
	    .add( OSCommand::defaultRemExec() ).add( " --inbg --nice 19 " );
    cmd.add( basiccmd );
    OSCommand oscomm( cmd, jobspec_.isodprog_ ? "" : remotehost_.buf() );
    return oscomm.execute( cmd );
}
