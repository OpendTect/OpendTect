/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "commandlineparser.h"
#include "hostdata.h"
#include "mmpkeystr.h"
#include "moddepmgr.h"
#include "netserver.h"
#include "netsocket.h"
#include "od_ostream.h"
#include "oscommand.h"
#include "odjson.h"
#include "remjobexec.h"
#include "singlebatchjobdispatch.h"
#include "systeminfo.h"
#include "timer.h"

#include "prog.h"


static void printBatchUsage()
{
    od_ostream& strm = od_ostream::logStream();
    strm << "Usage: " << OS::MachineCommand::odRemExecCmd();
    strm << " [OPTION]... [PARFILE]...\n";
    strm << "Executes an OpendTect program on a remote machine.\n";
    strm << "Hosts can be specified by host name or IP address.\n\n";
    strm << "Mandatory arguments:\n";
    strm << "\t --" << OS::MachineCommand::sKeyRemoteHost() <<"\tremote_host\n";
    strm << "\t --" << OS::MachineCommand::sKeyRemoteCmd() << "\t\tod_cmd\n\n";
    strm << "Optional arguments:\n";
    strm << "\t --" << OS::MachineCommand::sKeyPrimaryHost() << "\t";
    strm <<  Network::Socket::sKeyLocalHost() << od_newline;
    strm << "\t --" << OS::MachineCommand::sKeyPrimaryPort() << "\t";
    strm << Network::Server::sKeyPort() << od_newline;
    strm << "\t --" << OS::MachineCommand::sKeyJobID() << "\tjobid\n" ;
    strm << od_newline;
}


#define mExitRet() \
{ \
    ApplicationData::exit(1); \
    return; \
}

#define mErrRet() \
{ \
    printBatchUsage(); \
    mExitRet() \
}

using namespace MMPStr;

class RemExecHandler : public CallBacker
{
public:

RemExecHandler( const CommandLineParser& clp, od_ostream& strm )
    : clp_(clp)
    , strm_(strm)
{
    if ( clp_.nrArgs() < 5 )
    {
	printBatchUsage();
	return;
    }

    clp_.setKeyHasValue( OS::MachineCommand::sKeyRemoteHost() );
    clp_.setKeyHasValue( OS::MachineCommand::sKeyRemoteCmd() );
    clp_.setKeyHasValue( OS::MachineCommand::sKeyPrimaryHost() );
    clp_.setKeyHasValue( OS::MachineCommand::sKeyPrimaryPort() );
    clp_.setKeyHasValue( OS::MachineCommand::sKeyJobID() );
    clp_.setKeyHasValue( OS::CommandExecPars::sKeyPriority() );

    if ( !clp_.getVal(OS::MachineCommand::sKeyRemoteHost(),machine_) ||
	 !clp_.getVal(OS::MachineCommand::sKeyRemoteCmd(),remotecmd_) )
    {
	printBatchUsage();
	return;
    }

    isok_ = true;
    mAttachCB( timer_.tick, RemExecHandler::doWork );
    timer_.start( 100, true );
}

~RemExecHandler()
{
    detachAllNotifiers();
}

bool isOK() const
{
    return isok_;
}

private:

void doWork( CallBacker* )
{
    mDetachCB( timer_.tick, RemExecHandler::doWork );
    if ( !isOK() )
	mExitRet()

    const HostDataList hdl( false );
    const HostData* hd = hdl.find( machine_.str() );
    if ( !hd )
    {
	strm_ << "[Error] The requested remote machine " << machine_;
	strm_ << " is not registered in the BatchHosts configuration file\n";
	mExitRet()
    }

    BufferString remhostaddress( hd->connAddress() );
    if ( remhostaddress.isEmpty() )
	remhostaddress = machine_.str();

    OD::JSON::Object par;
    par.set( sProcName(), remotecmd_.str() );

    BufferString primaryhost;
    int primaryport = -1, jobid = 0;
    const bool hasprimaryhost =
	  clp_.getVal( OS::MachineCommand::sKeyPrimaryHost(), primaryhost );
    const bool hasprimaryport =
	  clp_.getVal( OS::MachineCommand::sKeyPrimaryPort(), primaryport );
    const bool hasjobid = clp_.getVal( OS::MachineCommand::sKeyJobID(), jobid );
    if ( hasprimaryhost && hasprimaryport && hasjobid )
    {
	par.set( sHostName(), primaryhost );
	par.set( sPortName(), primaryport );
	par.set( sJobID(), jobid );
    }
    else if ( hasprimaryhost || hasprimaryport || hasjobid )
    {
	strm_ << "Error: --" << OS::MachineCommand::sKeyPrimaryHost() << ", --";
	strm_ << OS::MachineCommand::sKeyPrimaryPort() << ", --";
	strm_ << OS::MachineCommand::sKeyJobID();
	strm_ << " arguments must be set together\n\n";
	mErrRet()
    }

    BufferStringSet normalarguments;
    clp_.getNormalArguments( normalarguments );
    if ( normalarguments.isEmpty() )
	mErrRet()

    const FilePath parfp( normalarguments.get(0) );
    par.set( sParFile(), parfp );

    const Network::Authority auth( remhostaddress,
				   RemoteJobExec::remoteHandlerPort() );
    PtrMan<RemoteJobExec> rje = new RemoteJobExec( auth );
    rje->addPar( par );
    ApplicationData::exit( rje->launchProc() ? 0 : 1 );
}

    const CommandLineParser&	clp_;
    bool		isok_ = false;
    Timer		timer_;
    BufferString	machine_;
    BufferString	remotecmd_;
    od_ostream&		strm_;
};


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::BatchProgCtxt )
    SetProgramArgs( argc, argv, false );
    ApplicationData app;

    OD::ModDeps().ensureLoaded( "Network" );

    CommandLineParser clp( argc, argv );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "MMProc" );
    od_ostream& strm = od_ostream::logStream();
    PtrMan<RemExecHandler> handler = new RemExecHandler( clp, strm );
    PIM().loadAuto( true );

    const bool ret = handler && handler->isOK() ? app.exec() : 1;
    handler = nullptr;
    return ret;
}
