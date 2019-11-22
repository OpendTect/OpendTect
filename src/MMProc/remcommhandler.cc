/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
{ BufferString msg(Time::getDateTimeString(),": ",s); writeLog( msg ); return; }

RemCommHandler::RemCommHandler( PortNr_Type port )
    : port_(port)
    , server_(*new Network::Server)
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

    BufferString cmd;
    mkCommand( par, cmd );
    if ( !OS::ExecCommand( cmd, OS::RunInBG ) )
	mErrRet( "Command Execution failed" );
}


bool RemCommHandler::mkCommand( const IOPar& par, BufferString& cmd )
{
    BufferString procnm, hostnm, portnm, survnm, dataroot, parfile, jobid;
    const int parsz = par.size();
    bool res;
    if ( parsz <= 2 )
    {
	res = par.get( "Proc Name", procnm ) && par.get( "Par File", parfile );
#ifdef __win__
        parfile.quote('\"');
#endif
	cmd = procnm;
	cmd.add( " " ).add( parfile );
	return res;
    }
    else
    {
	res = par.get( "Proc Name", procnm ) &&
	par.get( "Host Name", hostnm ) &&
	par.get( "Port Name", portnm ) &&
	par.get( "Job ID", jobid ) &&
	par.get( "Par File", parfile );
#ifdef __win__
        parfile.quote('\"');
#endif
    }

    if ( !res ) return false;

    cmd = procnm;
    cmd.add( " -masterhost " ).add( hostnm )
       .add( " -masterport " ).add( portnm )
       .add( " -jobid " ).add( jobid )
       .add( " " ).add( parfile );

    return true;
}


void RemCommHandler::uiErrorMsg( const char* msg )
{
    BufferString cmd( "\"", FilePath(GetExecPlfDir(),"od_DispMsg").fullPath() );
    cmd.add( "\" --err ").add( msg );
    OS::ExecCommand( cmd );
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
