/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: remcommhandler.cc,v 1.2 2010-09-10 11:59:03 cvsranojay Exp $";

#include "remcommhandler.h"

#include "filepath.h"
#include "genc.h"
#include "iopar.h"
#include "oddirs.h"
#include "strmprov.h"
#include "systeminfo.h"
#include "tcpserver.h"


#define mErrRet( s ) { uiErrorMsg( s ); return; }

RemCommHandler::RemCommHandler( const int port )
    : hostaddress_(System::localAddress())
    , port_(port)
    , server_(*new TcpServer)
    , odbinpath_(*new FilePath(GetBinPlfDir()))
    , batfile_("remproc.bat")
{
    server_.readyRead.notify( mCB(this,RemCommHandler,dataReceivedCB) );
}


RemCommHandler::~RemCommHandler()
{
    delete hostaddress_;
    delete &server_;
}


void RemCommHandler::listen() const
{   
    server_.listen(  hostaddress_, port_ );
}


void RemCommHandler::dataReceivedCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,socketid,cb);
    IOPar par;
    server_.read( socketid, par );

    if ( par.size() && !initEnv( par ) ) 
	mErrRet( "Environment Creation Failed" );
    FilePath fp( odbinpath_ );
    fp.add( batfile_ );
    BufferString cmd( "@", fp.fullPath() );
    StreamProvider sp( cmd );
    if ( !sp.executeCommand() )
	mErrRet( "Command Execution failed" );
}


bool RemCommHandler::initEnv( const IOPar& par )
{
    BufferString procnm, hostnm, portnm, survnm, dataroot, parfile, jobid;
    
    par.get( "Proc Name", procnm );
    par.get( "Host Name", hostnm );
    par.get( "Port Name", portnm );
    par.get( "Job ID", jobid );
    par.get( "Par File", parfile );
    
    FILE* fp = fopen( batfile_.buf(), "w" );
    if ( !fp ) return false;
    
    fprintf( fp, "set DTECT_DATA=S:\n" );
    fprintf( fp, "%s -masterhost %s -masterport %s -jobid %s \"%s\""
	       , procnm.buf()
	       , hostnm.buf()
	       , portnm.buf()
	       , jobid.buf()
	       , parfile.buf() );
    fclose( fp );
    return true;
}


void RemCommHandler::uiErrorMsg( const char* msg )
{
    FilePath fp( odbinpath_ );
    fp.add( "DispMsg" );

    BufferString cmd = fp.fullPath();
    cmd += " --err ";
    cmd += msg;
    ExecOSCmd( cmd.buf() );
}
