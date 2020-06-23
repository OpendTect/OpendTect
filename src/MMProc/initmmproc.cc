/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "singlebatchjobdispatch.h"
#include "clusterjobdispatch.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "hostdata.h"
#include "jobiomgr.h"
#include "mmbatchjobdispatch.h"
#include "oddirs.h"
#include "oscommand.h"
#include "procdescdata.h"


Batch::SingleJobDispatcherRemote::SingleJobDispatcherRemote()
    : Batch::SingleJobDispatcher()
{}

bool Batch::SingleJobDispatcherRemote::launch()
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

	FilePath logfp( ioparfp );
	logfp.setExtension( "log" );
	logfile = logfp.fullPath();
    }

    // Build machine command
    auto pathstyle = FilePath::Local;
    OS::MachineCommand mc( jobspec_.prognm_ );
    if ( !execlocal )
    {
	mc.setHostName( exechost->getHostName() );
	mc.setHostIsWindows( exechost->isWindows() );
	pathstyle = exechost->pathStyle();
    }
    mc.addArg( ioparfp.fullPath(pathstyle) );
    mc.addArgs( jobspec_.clargs_ );

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

    return mc.execute( jobspec_.execpars_ );
}

mDefModInitFn(MMProc)
{
    mIfNotFirstTime( return );

    Batch::SingleJobDispatcherRemote::initClass();
    Batch::MMJobDispatcher::initClass();
    Batch::ClusterJobDispatcher::initClass();
#ifdef  __win__
    ePDD().add( "od_remoteservice",
       Batch::MMProgDef::sMMProcDesc(), ProcDesc::DataEntry::ODv6 );
#endif //  __win__
}
