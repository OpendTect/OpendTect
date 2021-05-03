/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/

#include "commandlineparser.h"
#include "hostdata.h"
#include "iopar.h"
#include "netserver.h"
#include "netsocket.h"
#include "od_ostream.h"
#include "oscommand.h"
#include "remjobexec.h"
#include "singlebatchjobdispatch.h"
#include "systeminfo.h"

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


#define mErrRet() \
{ \
    printBatchUsage(); \
    return 1; \
}

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::BatchProgCtxt )
    SetProgramArgs( argc, argv );
    CommandLineParser clp;
    if ( clp.nrArgs() < 5 )
	mErrRet()

    clp.setKeyHasValue( OS::MachineCommand::sKeyRemoteHost() );
    clp.setKeyHasValue( OS::MachineCommand::sKeyRemoteCmd() );
    clp.setKeyHasValue( OS::MachineCommand::sKeyPrimaryHost() );
    clp.setKeyHasValue( OS::MachineCommand::sKeyPrimaryPort() );
    clp.setKeyHasValue( OS::MachineCommand::sKeyJobID() );

    BufferString machine, remotecmd;
    if ( !clp.getVal(OS::MachineCommand::sKeyRemoteHost(),machine) ||
	 !clp.getVal(OS::MachineCommand::sKeyRemoteCmd(),remotecmd) )
	mErrRet()

    const HostDataList hdl( false );
    const HostData* hd = hdl.find( machine.str() );
    if ( !hd )
	mErrRet()

    BufferString remhostaddress( hd->getIPAddress() );
    if ( remhostaddress.isEmpty() )
	remhostaddress = System::hostAddress( machine.str() );
    if ( remhostaddress.isEmpty() )
	remhostaddress = hd->getHostName();
    if ( remhostaddress.isEmpty() )
	remhostaddress = machine.str();

    IOPar par;
    par.set( "Proc Name", remotecmd.str() );

    BufferString primaryhost;
    int primaryport = -1, jobid = 0;
    const bool hasprimaryhost =
	       clp.getVal( OS::MachineCommand::sKeyPrimaryHost(), primaryhost );
    const bool hasprimaryport =
	       clp.getVal( OS::MachineCommand::sKeyPrimaryPort(), primaryport );
    const bool hasjobid = clp.getVal( OS::MachineCommand::sKeyJobID(), jobid );
    if ( hasprimaryhost && hasprimaryport && hasjobid )
    {
	par.set( "Host Name", primaryhost );
	par.set( "Port Name", primaryport );
	par.set( "Job ID", jobid );
    }
    else if ( hasprimaryhost || hasprimaryport || hasjobid )
    {
	od_ostream& strm = od_ostream::logStream();
	strm << "Error: --" << OS::MachineCommand::sKeyPrimaryHost() << ", --";
	strm << OS::MachineCommand::sKeyPrimaryPort() << ", --";
	strm << OS::MachineCommand::sKeyJobID();
	strm << " arguments must be set together\n\n";
	mErrRet()
    }

    BufferStringSet normalarguments;
    clp.getNormalArguments( normalarguments );
    if ( normalarguments.isEmpty() )
	mErrRet()

    par.set( "Par File", normalarguments.get(0) );

    const Network::Authority auth( remhostaddress, mCast(PortNr_Type,5050) );
    PtrMan<RemoteJobExec> rje = new RemoteJobExec( auth );
    rje->addPar( par );
    return rje->launchProc() ? 0 : 1;
}
