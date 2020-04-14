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
#include "filepath.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "netservice.h"
#include "pythonaccess.h"
#include "settings.h"
#include "timer.h"


/*!\brief Base class for OpendTect Service Manager and external services/apps */




uiODService::uiODService( bool assignport )
   : ODServiceBase(assignport)
{
    const CommandLineParser* clp = new CommandLineParser;
    const char* skeynolisten = Network::Server::sKeyNoListen();
    if ( !clp->hasKey(skeynolisten) )
    {
	if ( clp->hasKey(sKeyODServer()) )
	{
	    BufferString odserverstr;
	    if ( clp->getVal( sKeyODServer(), odserverstr ) )
		odauth_.fromString( odserverstr );
	}
    }
    delete clp;
    doRegister();
}


uiODService::~uiODService()
{
    detachAllNotifiers();
    delete mastercheck_;
    doDeRegister();
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


uiRetVal uiODService::doAction( const OD::JSON::Object& actobj )
{
    const BufferString action( actobj.getStringValue( sKeyAction()) );

    if ( action == sKeyCloseEv() )
    {
	return doCloseAct();
    }
    else if ( action == sKeyRaiseEv() )
    {
	uiMainWin* mainwin = uiMain::theMain().topLevel();
	if ( mainwin->isMinimized() || mainwin->isHidden() )
	{
	    mainwin->showNormal();
	    mainwin->raise();
	}
    }

    uiRetVal ret = ODServiceBase::doAction( actobj );
    if ( ret.isError() )
	uiMain::theMain().topLevel()->statusBar()->message( ret );

    return ret;
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


uiRetVal uiODService::close()
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    OD::JSON::Object request;
    request.set( sKeyAction(), sKeyCloseEv() );
    return doAction( request );
}


void uiODService::setBackground()
{
    handleMasterCheckTimer( true );
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
    {
	//TODO: only if top dialog is hidden ?
	// uiMsg ask confirmation ?
	uiMain::theMain().exit(0);
    }
}


uiRetVal uiODService::doRegister()
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    OD::JSON::Object sinfo;
    Network::Service::fillJSON( getAuthority(), sinfo );
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
    Network::Service::fillJSON( getAuthority(), sinfo );
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
    deleteAndZeroPtr( mastercheck_ );
    if ( !isODMainSlave() )
    {
	ODServiceBase::doAppClosing( cb );
	return;
    }

    odauth_.setPort( 0 );
}


void uiODService::doPyEnvChange( CallBacker* )
{
    if ( !isODMainSlave() )
	return;

    OD::JSON::Object sinfo;
    ODServiceBase::getPythEnvRequestInfo( sinfo );
    const uiRetVal uirv = ODServiceBase::sendRequest( odauth_, "ODMain",
						sKeyPyEnvChangeEv(), sinfo );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}


void uiODService::connClosedCB( CallBacker* cb )
{
    ODServiceBase::connClosedCB(cb);

    if ( needclose_ )
	uiMain::theMain().exit(0);

}
