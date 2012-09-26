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
#include "strmprov.h"
#include "systeminfo.h"
#include "tcpserver.h"
#include "timefun.h"
#include <fstream>


#define mErrRet( s ) \
{ BufferString msg(Time::getDateTimeString(),": ",s); writeLog( msg ); return; }

RemCommHandler::RemCommHandler( int port )
    : port_(port)
    , server_(*new TcpServer)
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

    BufferString tmpcmd;
    mkCommand( par, tmpcmd );
    BufferString cmd( "@", tmpcmd );
    StreamProvider sp( cmd );
    if ( !sp.executeCommand( true ) )
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
	cmd = procnm;
	cmd.add( " \" " ).add( parfile ).add( "\"" );
	return res; 
    }
    else
    {
	res = par.get( "Proc Name", procnm ) &&
	par.get( "Host Name", hostnm ) &&
	par.get( "Port Name", portnm ) &&
	par.get( "Job ID", jobid ) &&
	par.get( "Par File", parfile );
    }

    if ( !res ) return false;

    cmd = procnm;
    cmd.add( " -masterhost " ).add( hostnm )
       .add( " -masterport " ).add( portnm )
       .add( " -jobid " ).add( jobid )
       .add( " \" " ).add( parfile ).add( "\"" );

    return true;
}


void RemCommHandler::uiErrorMsg( const char* msg )
{
    BufferString cmd = FilePath( GetBinPlfDir(), "od_DispMsg" ).fullPath();
    cmd.add( " --err ").add( msg );
    ExecOSCmd( cmd.buf() );
}


std::ostream& RemCommHandler::createLogFile()
{
    FilePath logfp( GetBaseDataDir(), "LogFiles" );
    BufferString lhname = System::localAddress();
    replaceCharacter( lhname.buf(), '.',  '_' );
    logfp.add( lhname );
    logfp.setExtension( ".log" );
    std::ostream* strm = new std::ofstream( logfp.fullPath() );
    return *strm;
}


void RemCommHandler::writeLog( const char* msg )
{
    logstrm_ << msg << std::endl;
}

