/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/

#include "remcommhandler.h"

#include "filepath.h"
#include "iopar.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "oscommand.h"
#include "systeminfo.h"
#include "netserver.h"
#include "timefun.h"


#define mErrRet( s ) \
{ BufferString msg(Time::getDateTimeString(),": ",s); writeLog( msg ); return; }

RemCommHandler::RemCommHandler( PortNr_Type port )
    : port_(port)
    , server_(*new Network::Server(false))
    , logstrm_(createLogFile())
{
    mAttachCB( server_.readyRead, RemCommHandler::dataReceivedCB );
}


RemCommHandler::~RemCommHandler()
{
    detachAllNotifiers();
    delete &server_;
    delete &logstrm_;
}


void RemCommHandler::listen() const
{
    server_.listen( Network::Any, port_ );
}


void RemCommHandler::dataReceivedCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,socketid,cb);
    IOPar par;
    server_.read( socketid, par );
    if ( par.isEmpty() )
	mErrRet( "Could not read any parameters from server" );

    BufferString procnm, parfile;
    par.get( "Proc Name", procnm );
    par.get( "Par File", parfile );

    BufferString hostnm, portnm, jobid;
    par.get( "Host Name", hostnm );
    par.get( "Port Name", portnm );
    par.get( "Job ID", jobid );

    OS::MachineCommand machcomm( procnm );
    if ( !hostnm.isEmpty() )
	machcomm.addKeyedArg( OS::MachineCommand::sKeyPrimaryHost(),
			      hostnm, OS::OldStyle );
    if ( !portnm.isEmpty() )
	machcomm.addKeyedArg( OS::MachineCommand::sKeyPrimaryPort(),
			      portnm, OS::OldStyle );
    if ( !jobid.isEmpty() )
	machcomm.addKeyedArg( "jobid", jobid, OS::OldStyle );
    machcomm.addArg( parfile );

    OS::CommandLauncher cl( machcomm );
    OS::CommandExecPars pars( OS::Batch );
    pars.createstreams( true );
    uiRetVal uirv;
    const bool res = cl.execute( pars );
    if ( !res )
    {
	const uiString errmsg = cl.errorMsg();
	if ( errmsg.isEmpty() )
	    uirv.add( tr("Cannot launch '%1'").arg( machcomm.toString(&pars) ));
	else
	    uirv.add( errmsg );
    }

    BufferString stderrstr;
    od_istream* stderrstrm = cl.getStdError();
    if ( stderrstrm )
    {
	stderrstrm->getAll( stderrstr );
	if ( !stderrstr )
	    uirv.add( toUiString(stderrstr) );
    }

    if ( !uirv.isOK() )
	mErrRet( uirv.getText() )
}


od_ostream& RemCommHandler::createLogFile()
{
    FilePath logfp( GetBaseDataDir(), "LogFiles" );
    BufferString lhname = System::localAddress();
    lhname.replace( '.',  '_' );
    logfp.add( lhname );
    logfp.setExtension( ".log" );
    od_ostream* strm = new od_ostream( logfp.fullPath() );
    return *strm;
}


void RemCommHandler::writeLog( const char* msg )
{
    logstrm_ << msg << od_endl;
}
