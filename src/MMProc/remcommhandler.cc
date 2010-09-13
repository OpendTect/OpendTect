/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: remcommhandler.cc,v 1.5 2010-09-13 11:03:15 cvsranojay Exp $";

#include "remcommhandler.h"

#include "filepath.h"
#include "iopar.h"
#include "oddirs.h"
#include "strmprov.h"
#include "systeminfo.h"
#include "tcpserver.h"


#define mErrRet( s ) { uiErrorMsg( s ); return; }

static const char* sRemProcFile() 	{ return "remproc.bat"; }

RemCommHandler::RemCommHandler( int port )
    : port_(port)
    , server_(*new TcpServer)
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

#ifdef __win__
    FilePath batfnm( GetBinPlfDir() );
    batfnm.add( sRemProcFile() );
    FILE* fp = fopen( batfnm.fullPath(), "w" );
    if ( !fp ) return false;

    fprintf( fp, "%s", cmd.buf() );
    fclose( fp );
    cmd = batfnm.fullPath();
#endif

    return true;
}


void RemCommHandler::uiErrorMsg( const char* msg )
{
    FilePath fp( GetBinPlfDir() );
    fp.add( "DispMsg" );

    BufferString cmd = fp.fullPath();
    cmd += " --err ";
    cmd += msg;
    ExecOSCmd( cmd.buf() );
}
