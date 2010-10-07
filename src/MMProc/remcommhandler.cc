/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: remcommhandler.cc,v 1.8 2010-10-07 07:58:49 cvsnanne Exp $";

#include "remcommhandler.h"

#include "filepath.h"
#include "iopar.h"
#include "oddirs.h"
#include "strmprov.h"
#include "systeminfo.h"
#include "tcpserver.h"
#include <fstream>


#define mErrRet( s ) { uiErrorMsg( s ); writeLog( s ); return; }

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
    bool res = mkCommand( par, tmpcmd );
    BufferString cmd( "@", tmpcmd );
    StreamProvider sp( cmd );
    if ( !sp.executeCommand() )
	mErrRet( "Command Execution failed" );
}


bool RemCommHandler::mkCommand( const IOPar& par, BufferString& cmd )
{
    BufferString procnm, hostnm, portnm, survnm, dataroot, parfile, jobid;
    const bool res = par.get( "Proc Name", procnm ) &&
		     par.get( "Host Name", hostnm ) &&
		     par.get( "Port Name", portnm ) &&
		     par.get( "Job ID", jobid ) &&
		     par.get( "Par File", parfile );
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
    FilePath fp( GetBinPlfDir() );
    fp.add( "od_DispMsg" );

    BufferString cmd = fp.fullPath();
    cmd += " --err ";
    cmd += msg;
    ExecOSCmd( cmd.buf() );
}


std::ostream& RemCommHandler::createLogFile()
{
    FilePath logfp( GetBaseDataDir() );
    logfp.add( "LogFiles" );
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

