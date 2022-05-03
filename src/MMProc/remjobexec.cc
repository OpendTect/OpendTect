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
#include "mmpkeystr.h"
#include "mmpserverclient.h"
#include "netsocket.h"


using namespace MMPStr;

RemoteJobExec::RemoteJobExec( const Network::Authority& auth )
    : auth_(auth)
{
    checkConnection();
}


RemoteJobExec::~RemoteJobExec()
{
}


bool RemoteJobExec::launchProc() const
{
    if ( !par_.isEmpty() )
    {
	MMPServerClient mmpserver( auth_ );
	if ( mmpserver.isOK() )
	{
	    OD::JSON::Object resp = mmpserver.sendRequest( sStartJob(), par_ );
	    return (mmpserver.isOK() && resp.isPresent(sOK()));
	}
    }

    return false;
}


void RemoteJobExec::addPar( const OD::JSON::Object& par )
{ par_ = par; }


void RemoteJobExec::checkConnection()
{
    BufferString errmsg( "Connection to Daemon on ", auth_.getHost() );
    errmsg += " failed";
    MMPServerClient mmpserver( auth_ );
    if ( !mmpserver.isOK() )
    {
	const BufferString msg = mmpserver.errMsg().getText();
	if ( !msg.isEmpty() )
	    errmsg.add( ": " ).add( msg );
	OD::DisplayErrorMessage( errmsg );
    }
}

