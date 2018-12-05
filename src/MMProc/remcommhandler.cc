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
#include "od_ostream.h"
#include "oscommand.h"
#include "systeminfo.h"
#include "netserver.h"
#include "timefun.h"


#define mErrRet( s ) \
{ \
    const BufferString msg( Time::getUsrDateTimeString(), ": ", s ); \
    writeLog( msg ); \
    return; \
}

RemCommHandler::RemCommHandler( int port )
    : port_(port)
    , server_(*new Network::Server)
    , logstrm_(createLogFile())
{
    server_.readyRead.notify( mCB(this,RemCommHandler,dataReceivedCB) );
}


RemCommHandler::~RemCommHandler()
{
    delete &server_;
    delete &logstrm_;
}


void RemCommHandler::listen() const
{
    server_.listen( System::localAddress(), port_ );
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
	machcomm.addKeyedArg( "masterhost", hostnm, OS::OldStyle );
    if ( !portnm.isEmpty() )
	machcomm.addKeyedArg( "masterport", portnm, OS::OldStyle );
    if ( !jobid.isEmpty() )
	machcomm.addKeyedArg( "jobid", jobid, OS::OldStyle );
    machcomm.addArg( parfile );

    OS::CommandLauncher(machcomm).execute( OS::RunInBG );
}


od_ostream& RemCommHandler::createLogFile()
{
    File::Path logfp( GetBaseDataDir(), "LogFiles" );
    BufferString lhname = System::localAddress();
    lhname.replace( '.',  '_' );
    logfp.add( lhname );
    logfp.setExtension( ".log", false );
    od_ostream* strm = new od_ostream( logfp.fullPath() );
    return *strm;
}


void RemCommHandler::writeLog( const char* msg )
{
    logstrm_ << msg << od_endl;
}
