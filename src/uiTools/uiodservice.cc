/*+
 _ __________________________________________*_____________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Oct 2019
 ________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodservice.h"

#include "uimain.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uistatusbar.h"

#include "commandlineparser.h"
#include "ioman.h"
#include "netserver.h"
#include "netservice.h"
#include "odjson.h"
#include "timer.h"



uiODService::uiODService( uiMainWin& mainwin, bool islocal,
					const char* servernm, bool assignport )
   : ODServiceBase(islocal,servernm,assignport)
{
    init( mainwin, islocal );
}


uiODService::uiODService( uiMainWin& mainwin, const char* hostname,
						bool assignport )
    : ODServiceBase(hostname,assignport)
{
    init( mainwin, true );
}


void uiODService::init( uiMainWin& mainwin, bool islocal )
{
    if ( !isMainService() )
	return;

    const CommandLineParser clp;
    const char* skeynolisten = Network::Server::sKeyNoListen();
    if ( !clp.hasKey(skeynolisten) )
    {
	if ( clp.hasKey(sKeyODServer()) )
	{
	    //TODO for local server
	    BufferString odserverstr;
	    if ( clp.getVal(sKeyODServer(),odserverstr) )
	    {
		if ( islocal )
		    odauth_.localFromString( odserverstr );
		else
		    odauth_.fromString( odserverstr );
	    }
	}
    }

    doRegister();
    mAttachCB( mainwin.windowClosed, uiODService::doAppClosing );
}


uiODService::~uiODService()
{
    doAppClosing( nullptr );
}


bool uiODService::isODMainSlave() const
{
    return odauth_.hasAssignedPort();
}


bool uiODService::isMasterAlive() const
{
    uiUserShowWait uisv( uiMain::theMain().topLevel(),
			 tr("Checking status of Master application") );
    const uiRetVal uirv = sendAction( sKeyStatusEv() );
    return uirv.isOK();
}


bool uiODService::doParseAction( const char* action, uiRetVal& uirv )
{
    if ( FixedString(action) == sKeyRaiseEv() )
    {
	uiMain::theMain().topLevel()->showAndActivate();
	uirv.setOK();
	return true;
    }

    return ODServiceBase::doParseAction( action, uirv );
}


bool uiODService::doParseRequest( const OD::JSON::Object& request,
				  uiRetVal& uirv )
{
    return ODServiceBase::doParseRequest( request, uirv );
}


uiRetVal uiODService::sendAction( const char* action ) const
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    const BufferString servicenm( "ODServiceMGr" );
    return ODServiceBase::sendAction( odauth_, servicenm, action );
}


uiRetVal uiODService::sendRequest( const char* reqkey,
				  const OD::JSON::Object& reqobj ) const
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    const BufferString servicenm( "ODServiceMGr" );
    return ODServiceBase::sendRequest( odauth_, servicenm, reqkey, reqobj );
}


void uiODService::setBackground( bool yn )
{
    handleMasterCheckTimer( yn );
}


void uiODService::handleMasterCheckTimer( bool start )
{
    if ( start )
    {
	if ( mastercheck_ )
	    mDetachCB( mastercheck_->tick, uiODService::masterCheckCB );
	else
	    mastercheck_ = new Timer( "Master status check" );
	mAttachCB( mastercheck_->tick, uiODService::masterCheckCB );
	mastercheck_->start( 5000 );
    }
    else
    {
	if ( mastercheck_ )
	    mastercheck_->stop();
    }
}


void uiODService::masterCheckCB( CallBacker* cb )
{
    if ( !isMasterAlive() )
	uiMain::theMain().exit(0);
}


uiRetVal uiODService::doRegister()
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


uiRetVal uiODService::doDeRegister()
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


void uiODService::doAppClosing( CallBacker* cb )
{
    detachAllNotifiers();
    deleteAndZeroPtr( mastercheck_ );
    doDeRegister();
    odauth_.setPort( 0 );
    ODServiceBase::doAppClosing( cb );
}


void uiODService::closeApp()
{
    uiMain::theMain().exit(0);
}


void uiODService::doPyEnvChange( CallBacker* )
{
    if ( !isODMainSlave() )
	return;

    OD::JSON::Object sinfo;
    getPythEnvRequestInfo( sinfo );
    const uiRetVal uirv = ODServiceBase::sendRequest( odauth_, "ODMain",
				       sKeyPyEnvChangeEv(), sinfo );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}
