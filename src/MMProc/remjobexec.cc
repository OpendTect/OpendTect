/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "remjobexec.h"

#include "filepath.h"
#include "iopar.h"
#include "oddirs.h"
#include "strmprov.h"
#include "netsocket.h"

#define mErrRet( s ) { uiErrorMsg( s ); exit(0); }


RemoteJobExec::RemoteJobExec( const Network::Authority& auth )
    : socket_(*new Network::Socket)
    , auth_(auth)
    , par_(*new IOPar)
    , isconnected_(false)
{
    socket_.setTimeout( 4000 );
    isconnected_ = socket_.connectToHost( auth, true );
    ckeckConnection();
}


RemoteJobExec::~RemoteJobExec()
{
    socket_.disconnectFromHost();
    delete &socket_;
    delete &par_;
}


bool RemoteJobExec::launchProc() const
{
    if ( !par_.isEmpty() )
	return socket_.write( par_ );

    return false;
}


void RemoteJobExec::addPar( const IOPar& par )
{ par_ = par; }


void RemoteJobExec::ckeckConnection()
{
    BufferString errmsg( "Connection to Daemon on ", auth_.getHost() );
    errmsg += " failed";
    if ( !isconnected_ )
    {
	const uiString socketmsg = socket_.errMsg();
	if ( !socketmsg.isEmpty() )
	    errmsg.add( ": " ).add( socketmsg.getFullString() );
	mErrRet( errmsg.buf() );
    }
}


void RemoteJobExec::uiErrorMsg( const char* msg )
{
    BufferString cmd( "\"", FilePath(GetExecPlfDir(),"od_DispMsg").fullPath() );
    cmd.add( "\" --err " ).add( msg );
    OS::ExecCommand( cmd );
}
