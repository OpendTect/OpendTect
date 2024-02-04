/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "remjobexec.h"

#include "envvars.h"
#include "iopar.h"
#include "oddirs.h"
#include "oscommand.h"
#include "mmpkeystr.h"
#include "mmpserverclient.h"
#include "networkcommon.h"
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
    if ( par_ && !par_->isEmpty() )
    {
	MMPServerClient mmpserver( auth_ );
	if ( mmpserver.isOK() )
	{
	    OD::JSON::Object resp = mmpserver.sendRequest( sStartJob(), *par_ );
	    return (mmpserver.isOK() && resp.isPresent(sOK()));
	}
    }

    return false;
}


void RemoteJobExec::addPar( const OD::JSON::Object& par )
{
    par_.set( par.clone() );
}


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


PortNr_Type RemoteJobExec::legacyRemoteHandlerPort()
{
    return mCast(PortNr_Type,5050);
}


PortNr_Type RemoteJobExec::stdRemoteHandlerPort()
{
    static int portnr = GetEnvVarIVal( "DTECT_MM_ODREMPROC", 15050 );
    return mCast(PortNr_Type,portnr);
}


PortNr_Type RemoteJobExec::getLocalHandlerPort()
{
    mDefineStaticLocalObject( PortNr_Type, remport,
		      = Network::isPortFree( stdRemoteHandlerPort() )
				? stdRemoteHandlerPort()
				: legacyRemoteHandlerPort() );
    return remport;
}


bool RemoteJobExec::remoteHostOK( const Network::Authority& auth )
{
    MMPServerClient mmpserver( auth );
    return mmpserver.isOK();
}
