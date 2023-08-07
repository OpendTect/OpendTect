/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "singlebatchjobdispatch.h"

#include "batchprogtracker.h"
#include "clientservicebase.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "hostdata.h"
#include "jobiomgr.h"
#include "jobrunner.h"
#include "keystrs.h"
#include "netserver.h"
#include "oddirs.h"
#include "od_ostream.h"
#include "oscommand.h"


Batch::SingleJobDispatcher::SingleJobDispatcher()
{
}


Batch::SingleJobDispatcher::~SingleJobDispatcher()
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
    fp.setExtension( nullptr );
    BufferString logfnm( fp.fullPath() );
    logfnm.add( "_log.txt" );
    if ( File::isInUse(logfnm) )
    {
	errmsg_ = tr("File %1 is already in use, "
	    "please close the process and try again").arg( logfnm );
	return false;
    }

    jobspec_.pars_.update( sKey::LogFile(), logfnm );

    return true;
}


bool Batch::SingleJobDispatcher::launch( Batch::ID* batchid )
{
    if ( !writeParFile() )
	return false;

    const HostDataList hdl( false );
    const BufferString localhostnm( GetLocalHostName() );
    const HostData* localhost = hdl.find( localhostnm.str() );
    PtrMan<HostData> localhostdestroyer;
    if ( !localhost )
    {
	localhostdestroyer = new HostData( localhostnm );
	localhost = localhostdestroyer.ptr();
    }

    const bool execlocal = remotehost_.isEmpty();
    const HostData* exechost = localhost;
    remoteexec_.setEmpty();
    if ( !execlocal )
    {
	exechost = hdl.find( remotehost_ );
	if ( !exechost )
	{
	    errmsg_ = tr("Wrong host information");
	    return false;
	}

	const bool unix2unix = !localhost->isWindows()
			      && !exechost->isWindows();
	remoteexec_.set( unix2unix ? hdl.loginCmd()
				   : OS::MachineCommand::odRemExecCmd() );
    }

    FilePath ioparfp;
    BufferString logfile;
    if ( execlocal )
    {
	ioparfp.set( parfnm_ );
	jobspec_.pars_.get( sKey::LogFile(), logfile );
    }
    else
    {
	BufferString procdir( GetProcFileName( getTempBaseNm() ) );
	procdir.add( "_" ).add( MMJob_getTempFileNr() );
	MMJob_getTempFileNr()++;

	if ( File::exists(procdir) && !File::isDirectory(procdir) )
	    File::remove(procdir);
	if ( !File::exists(procdir) )
	    File::createDir(procdir);

	BufferString basenm( exechost->getHostName() );
#ifdef __win__
	basenm.replace( '.',  '_' );
#endif
	FilePath basefp( procdir );
	basefp.add( basenm );
	BufferString msg;
	if ( !JobIOMgr::mkIOParFile(basefp,*exechost,jobspec_.pars_,
				    ioparfp,msg) )
	    { DBG::message(msg); return false; }

	FilePath logfp( basefp );
	logfp.setExtension( "log" );
	logfile = logfp.fullPath();
    }

    // Build machine command
    auto pathstyle = FilePath::Local;
    OS::MachineCommand mc( jobspec_.prognm_ );
    if ( !execlocal )
    {
	mc.setRemExec( remoteexec_ )
	  .setHostName( exechost->connAddress() )
	  .setHostIsWindows( exechost->isWindows() );
	pathstyle = exechost->pathStyle();
    }

    mc.addArg( ioparfp.fullPath(pathstyle) );
    mc.addArgs( jobspec_.clargs_ );
    if ( batchid )
	JobDispatcher::addIDTo( *batchid, mc );

    jobspec_.execpars_.monitorfnm( logfile );

    if ( DBG::isOn(DBG_MM) )
    {
	BufferString msg( "Executing: ", mc.toString(&jobspec_.execpars_) );
	msg.add( " on host " ).add( exechost->connAddress() );

	if ( !execlocal )
	    msg.add( " using " ).add( remoteexec_.str() ).addNewLine();

	DBG::message(msg);
    }

    if ( jobspec_.execpars_.launchtype_ >= OS::Batch )
	ServiceClientMgr::addApplicationAuthority( mc );

    FilePath fp( parfnm_ );
    fp.setExtension( "lock" );
    const BufferString lockflfp = fp.fullPath();
    od_ostream parstrm( lockflfp );
    parstrm.close();

    if ( mc.execute(jobspec_.execpars_) )
	BPT(); //Here could be place to forward service name and process name
    else
    {
	File::remove( lockflfp );
	return false;
    }

    return true;
}
