/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "singlebatchjobdispatch.h"

#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "hostdata.h"
#include "jobiomgr.h"
#include "jobrunner.h"
#include "oddirs.h"
#include "oscommand.h"


Batch::SingleJobDispatcher::SingleJobDispatcher()
{
}


uiString Batch::SingleJobDispatcher::description() const
{
    return tr("The job will be executed on one computer, in a single process.");
}


bool Batch::SingleJobDispatcher::init()
{
    if ( parfnm_.isEmpty() )
	getDefParFilename( jobspec_.prognm_, parfnm_ );

    FilePath fp( parfnm_ );
    fp.setExtension( 0 );
    BufferString logfnm( fp.fullPath() );
    logfnm.add( "_log.txt" );
    jobspec_.pars_.update( sKey::LogFile(), logfnm );

    return true;
}


bool Batch::SingleJobDispatcher::launch()
{
    if ( !writeParFile() )
	return false;

    const HostDataList hdl( false );
    const bool hasconfig = !hdl.isEmpty();
    const BufferString localhostnm( HostData::localHostName() );
    const HostData* localhost = hasconfig ? hdl.find( localhostnm.str() )
					  : new HostData( localhostnm.str() );
    const bool remote = !remotehost_.isEmpty();
    const HostData* machine = hdl.find( remote ? remotehost_.str()
			       : localhost ? localhost->getHostName() : 0 );
    if ( !localhost || ( remote && !machine ) )
	return false;

    const bool unixtorunix = remote && !localhost->isWindows() &&
			     machine && !machine->isWindows();

    if ( remote )
    {
	remoteexec_.set( unixtorunix ? hdl.loginCmd()
				     : OS::MachineCommand::odRemExecCmd() );
    }
    else
	remoteexec_.setEmpty();

    CommandString argstr( machine ? *machine : *localhost );
    FilePath progfp( jobspec_.prognm_ );
    FilePath ioparfp;
    BufferString logfile;
    if ( remote )
    {
	if ( unixtorunix )
	{
	    progfp.insert( GetExecPlfDir() );
	    progfp = machine->convPath( HostData::Appl, progfp, localhost );
	}

	BufferString procdir( GetProcFileName( getTempBaseNm() ) );
	procdir.add( "_" ).add( MMJob_getTempFileNr() );
	MMJob_getTempFileNr()++;

	if ( File::exists(procdir) && !File::isDirectory(procdir) )
	    File::remove(procdir);
	if ( !File::exists(procdir) )
	    File::createDir(procdir);

	BufferString basenm( machine->getHostName() );
#ifdef __win__
	basenm.replace( '.',  '_' );
#endif
	FilePath basefp( procdir );
	basefp.add( basenm );
	BufferString msg;
	if ( !JobIOMgr::mkIOParFile(basefp,*machine,jobspec_.pars_,ioparfp,msg))
	{
	    DBG::message(msg);
	    return false;
	}

	FilePath logfp( ioparfp ); logfp.setExtension( "log" );
	logfile = logfp.fullPath();
    }
    else
    {
	ioparfp.set( parfnm_ );
	jobspec_.pars_.get( sKey::LogFile(), logfile );
    }

    BufferString cmd( unixtorunix
			? progfp.fullPath( machine->pathStyle() ).str()
			: progfp.fileName().str() );

    argstr.addFilePath( ioparfp );
    jobspec_.clargs_.add( argstr.string() );
    cmd.addSpace().add( jobspec_.clargs_ );

    if ( !hasconfig )
	delete localhost;

    if ( DBG::isOn(DBG_MM) )
    {
	BufferString msg( "Executing: ", cmd );
	if ( machine && machine->getHostName() )
	    msg.add( " on host " ).add( machine->getHostName() );

	if ( !remoteexec_.isEmpty() )
	    msg.add( " using " ).add( remoteexec_.str() ).addNewLine();

	DBG::message(msg);
    }

    if ( ( remote && !machine->isWindows() ) || !remote )
	jobspec_.execpars_.monitorfnm( logfile );

    OS::MachineCommand mc( cmd );
    mc.setRemExec( remoteexec_.str() );
    if ( !remoteexec_.isEmpty() )
	mc.setHostName( machine->getHostName() );

    OS::CommandLauncher cl( mc );

    return cl.execute( jobspec_.execpars_ );
}
