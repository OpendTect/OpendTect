/*+
_ __________________________________________*_____________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author: Wayne Mogg
Date:		Oct 2019
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odbatchservice.h"

#include "commandlineparser.h"
#include "ioman.h"
#include "filepath.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "netservice.h"
#include "pythonaccess.h"
#include "settings.h"
#include "timer.h"

/*!\brief Base class for OpendTect Service Manager and external services/apps */




ODBatchService::ODBatchService( bool assignport )
    : ODServiceBase(true,"batchserv",assignport)
{ /*Not used, only for compatibility */ }


ODBatchService::ODBatchService( bool islocal, const char* servernm,
							    bool assignport )
    : ODServiceBase(islocal,servernm,assignport)
{
    init( islocal );
}


ODBatchService::ODBatchService( const char* hostname, bool assignport)
    : ODServiceBase(hostname,assignport)
{
    init( true );
}


ODBatchService::~ODBatchService()
{
    detachAllNotifiers();
    delete mastercheck_;
    doDeRegister();
}


void ODBatchService::init( bool islocal )
{
    const CommandLineParser* clp = new CommandLineParser;
    const char* skeynolisten = Network::Server::sKeyNoListen();
    if ( !clp->hasKey(skeynolisten) )
    {
	if ( clp->hasKey(sKeyODServer()) )
	{
	    BufferString odserverstr;
	    if ( clp->getVal( sKeyODServer(), odserverstr ) )
	    {
		if ( islocal )
		    odauth_.localFromString( odserverstr );
		else
		    odauth_.fromString( odserverstr, islocal );
	    }
	}
    }
    delete clp;

    doRegister();
}


bool ODBatchService::isODMainSlave() const
{
    return odauth_.hasAssignedPort();
}


ODBatchService& ODBatchService::getMgr() // always local;
{
    return getMgr( true );
}


ODBatchService& ODBatchService::getMgr(bool islocal) // always local;
{
    mDefineStaticLocalObject(ODBatchService,mgrInstance,(islocal));
    return mgrInstance;
}


bool ODBatchService::doParseAction( const char* action ,uiRetVal& ret )
{
    return ODServiceBase::doParseAction( action, ret );
}


uiRetVal ODBatchService::sendAction( const char* action ) const
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    const BufferString servicenm( "ODBatchServiceMGr" );
    return ODServiceBase::sendAction( odauth_, servicenm, action );
}


uiRetVal ODBatchService::sendRequest( const char* reqkey,
    const OD::JSON::Object& reqobj ) const
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    const BufferString servicenm( "ODBatchServiceMGr" );
    return ODServiceBase::sendRequest( odauth_, servicenm, reqkey, reqobj );
}


uiRetVal ODBatchService::close()
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();
    uiRetVal retval;
    doParseAction( sKeyCloseEv(), retval );
    return retval;
}


void ODBatchService::processingComplete()
{
    sendAction( sKeyProcessingDone() );
}


void ODBatchService::handleMasterCheckTimer( bool start )
{
    if ( !start )
    {
	if ( mastercheck_ )
	    mastercheck_->stop();
    }
}


uiRetVal ODBatchService::doRegister()
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    OD::JSON::Object sinfo;
    Network::Service::fillJSON( getAuthority(odauth_.isLocal()), sinfo );
    uiRetVal uirv = ODServiceBase::sendRequest( odauth_, "ODMain",
	sKeyRegister(), sinfo );
    if ( !uirv.isOK() )
    {
	uirv.add( tr("Registration of service: %1 failed")
	    .arg(Network::Service::getServiceName(sinfo)) );
	return uirv;
    }

    servid_ = Network::Service::getID( sinfo );

    return uiRetVal::OK();
}


uiRetVal ODBatchService::doDeRegister()
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    OD::JSON::Object sinfo;
    Network::Service::fillJSON( getAuthority(odauth_.isLocal()), sinfo );
    uiRetVal uirv = ODServiceBase::sendRequest( odauth_, "ODMain",
	sKeyDeregister(), sinfo );
    if ( !uirv.isOK() )
    {
	uirv.add( tr("DeRegistration of service: %1 failed")
	    .arg(Network::Service::getServiceName(sinfo)) );
	return uirv;
    }

    servid_ = 0;

    return uiRetVal::OK();
}


void ODBatchService::doAppClosing( CallBacker* cb )
{
    deleteAndZeroPtr( mastercheck_ );
    if ( !isODMainSlave() )
    {
	ODServiceBase::doAppClosing( cb );
	return;
    }

    odauth_.setPort( 0 );
    // all the things in desctructor should be here
    // deleteinstacne things goes here
    // No exit program calls now
    // after given time no instruction received close on itself
    //
}
