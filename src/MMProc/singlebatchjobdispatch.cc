/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/

#include "singlebatchjobdispatch.h"

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

    File::Path fp( parfnm_ );
    fp.setExtension( 0 );
    BufferString logfnm( fp.fullPath() );
    logfnm.add( "_log.txt" );
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
	    return false;

	const bool unix2unix = !localhost->isWindows()
			      && !exechost->isWindows();
	remoteexec_.set( unix2unix ? hdl.loginCmd()
				   : OS::MachineCommand::odRemExecCmd() );
    }

    File::Path ioparfp;
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
	File::Path basefp( procdir );
	basefp.add( basenm );
	BufferString msg;
	if ( !JobIOMgr::mkIOParFile(basefp,*exechost,jobspec_.pars_,
				    ioparfp,msg) )
	    { DBG::message(msg); return false; }

	File::Path logfp( ioparfp );
	logfp.setExtension( "log" );
	logfile = logfp.fullPath();
    }

    // Build machine command
    auto pathstyle = File::Path::Local;
    OS::MachineCommand mc( jobspec_.prognm_ );
    if ( !execlocal )
    {
	mc.setHostName( exechost->getHostName() );
	mc.setHostIsWindows( exechost->isWindows() );
	pathstyle = exechost->pathStyle();
    }
    mc.addArg( ioparfp.fullPath(pathstyle) );
    mc.addArgs( jobspec_.clargs_ );
    if ( batchid )
	JobDispatcher::addIDTo( *batchid, mc );

    if ( execlocal || !exechost->isWindows() )
	jobspec_.execpars_.monitorfnm( logfile );

    mc.setRemExec( remoteexec_ );
    if ( !execlocal )
	mc.setHostName( exechost->getHostName() );

    if ( DBG::isOn(DBG_MM) )
    {
	BufferString msg( "Executing: ", mc.toString(&jobspec_.execpars_) );
	msg.add( " on host " ).add( exechost->getHostName() );

	if ( !execlocal )
	    msg.add( " using " ).add( remoteexec_.str() ).addNewLine();

	DBG::message(msg);
    }

/*    if ( jobspec_.execpars_.launchtype_ >= OS::Batch )
      Not (yet) registering all bathc programs */
    if ( jobspec_.execpars_.launchtype_ > OS::Batch )
	ServiceClientMgr::addApplicationAuthority( mc );

    return mc.execute( jobspec_.execpars_ );
}
