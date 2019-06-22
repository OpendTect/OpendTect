/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/


#include "remjobexec.h"

#include "iopar.h"
#include "oddirs.h"
#include "oscommand.h"
#include "netsocket.h"

#define mErrRet( s ) { uiErrorMsg( s ); exit(0); }


RemoteJobExec::RemoteJobExec( const char* host, const int port )
    : socket_(*new Network::Socket)
    , host_(host)
    , par_(*new IOPar)
    , isconnected_(false)
{
    socket_.setTimeout( 3000 );
    isconnected_ = socket_.connectToHost( host_, port, true );
    checkConnection();
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


void RemoteJobExec::checkConnection()
{
    BufferString errmsg( "Connection to Daemon on ", host_ );
    errmsg += " failed";
    if ( !isconnected_ )
    {
	const uiString socketmsg = socket_.errMsg();
	if ( !socketmsg.isEmpty() )
	    errmsg.add( ": " ).add( socketmsg.getString() );
	OD::DisplayErrorMessage( errmsg );
    }
}
