/*+
 _ __________________________________________*_____________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Oct 2019
 ________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodservicemgr.h"

#include "uimain.h"
#include "uimsg.h"
#include "uiodapplmgr.h"

#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odjson.h"


/*!\brief The OpendTect service manager */

using namespace Network;

uiODServiceMgr& uiODServiceMgr::getMgr()
{
    mDefineStaticLocalObject(uiODServiceMgr,mgrInstance,);
    return mgrInstance;
}


uiODServiceMgr::uiODServiceMgr()
    : uiODService(*uiMain::theMain().topLevel(),true)
    , serviceAdded(this)
    , serviceRemoved(this)
{
}


uiODServiceMgr::~uiODServiceMgr()
{
    doAppClosing( nullptr );
}


void uiODServiceMgr::setFor( uiMainWin& win )
{
    if ( &win == uiMain::theMain().topLevel() )
	getMgr();
}


uiRetVal uiODServiceMgr::addService( const OD::JSON::Object* jsonobj )
{
    uiRetVal uirv;
    if ( !jsonobj )
    {
	uirv = tr("Empty request");
	return uirv;
    }

    Network::Service* service = new Network::Service( *jsonobj );
    if ( !service->isOK() )
    {
	uirv = service->message();
	delete service;
	return uirv;
    }

    services_.add( service );
    serviceAdded.trigger( service->getID() );
    return uiRetVal::OK();
}


uiRetVal uiODServiceMgr::removeService( const OD::JSON::Object* jsonobj )
{
    uiRetVal uirv;
    if ( !jsonobj )
    {
	uirv = tr("Empty request");
	return uirv;
    }

    const Network::Service service( *jsonobj );
    if ( service.isOK() )
	removeService( service.getID() );

    return service.message();
}


void uiODServiceMgr::removeService( const Network::Service::ID servid )
{
    for ( int idx=services_.size()-1; idx>=0; idx-- )
    {
	if ( services_[idx]->getID() != servid )
	    continue;

	if ( services_[idx]->isAlive() )
	    sendAction( *services_[idx], sKeyCloseEv() );
	delete services_.removeSingle( idx );
	serviceRemoved.trigger( servid );
	return;
    }
}


const Network::Service* uiODServiceMgr::getService(
				 const Network::Service::ID servid ) const
{
    return const_cast<uiODServiceMgr&>(*this).getService( servid );
}


Network::Service* uiODServiceMgr::getService(
				  const Network::Service::ID servid )
{
    for ( int idx=0; idx<services_.size(); idx++ )
    {
	Network::Service* service = services_[idx];
	if ( service->getID() == servid )
	    return service;
    }

    return nullptr;
}


uiRetVal uiODServiceMgr::sendAction( const Network::Service::ID servid,
				     const char* action ) const
{
    const Network::Service* service = getService( servid );
    if ( !service )
	return uiRetVal( tr("Service with ID %1 not registered").arg( servid ));

    return sendAction( *service, action );
}


uiRetVal uiODServiceMgr::sendAction( const Network::Service& service,
				     const char* action ) const
{
    const BufferString servicenm( "Service ", service.name() );
    return ODServiceBase::sendAction( service.getAuthority(),
					servicenm, action );
}


uiRetVal uiODServiceMgr::sendRequest( const Network::Service& service,
				      const char* reqkey,
				      const OD::JSON::Object& reqinfo ) const
{
    const BufferString servicenm( "Service ", service.name() );
    return ODServiceBase::sendRequest( service.getAuthority(),
					 servicenm, reqkey, reqinfo );
}


bool uiODServiceMgr::isPresent( const Network::Service::ID servid ) const
{
    return getService( servid );
}



bool uiODServiceMgr::isAlive( const Network::Service::ID servid ) const
{
    const Network::Service* serv = getService( servid );
    return serv && serv->isAlive();
}


BufferString uiODServiceMgr::serviceName(
				    const Network::Service::ID servid ) const
{
    const Network::Service* service = getService( servid );
    const FilePath fp( service->name() );
    return service ? fp.baseName() : BufferString::empty();
}


void uiODServiceMgr::raise( const Network::Service::ID servid ) const
{
    const uiRetVal uirv = sendAction( servid, sKeyRaiseEv() );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}


void uiODServiceMgr::doAppClosing( CallBacker* cb )
{
    for ( int idx=services_.size()-1; idx>=0; idx-- )
	removeService( services_[idx]->getID() );
    deepErase( services_ );
    uiODService::doAppClosing( cb );
}


void uiODServiceMgr::doSurveyChanged( CallBacker* )
{
    OD::JSON::Object paramobj;
    paramobj.set( sKey::Survey(), GetDataDir() );
    for ( int idx=0; idx<services_.size(); idx++ )
    {
	const uiRetVal uirv = sendRequest( *services_[idx],
					sKeySurveyChangeEv(), paramobj );
	if ( !uirv.isOK() )
	    uiMSG().error( uirv );
    }
}


void uiODServiceMgr::doPyEnvChange( CallBacker* )
{
    OD::JSON::Object sinfo;
    getPythEnvRequestInfo( sinfo );
    for ( int idx=0; idx<services_.size(); idx++ )
    {
	const uiRetVal uirv = sendRequest( *services_[idx], sKeyPyEnvChangeEv(),
					   sinfo );
	if ( !uirv.isOK() )
	    uiMSG().error( uirv );
    }
}


void uiODServiceMgr::closeApp()
{
    auto* mainwin = ODMainWin();
    if ( mainwin )
    {
	mainwin->forceExit();
	return;
    }

    uiMain::theMain().exit(0);
}


bool uiODServiceMgr::doParseAction( const char* action, uiRetVal& uirv )
{
    return uiODService::doParseAction( action, uirv );
}


bool uiODServiceMgr::doParseRequest( const OD::JSON::Object& request,
				     uiRetVal& uirv )
{
    if ( request.isPresent(sKeyRegister()) )
    {
	uirv = addService( request.getObject(sKeyRegister()) );
	return true;
    }
    else if ( request.isPresent(sKeyDeregister()) )
    {
	uirv = removeService( request.getObject(sKeyDeregister()) );
	return true;
    }
    else if ( request.isPresent(sKeyStart()) )
    {
	uirv = startApp( request.getObject(sKeyStart()) );
	return true;
    }

    return uiODService::doParseRequest( request, uirv );
}


uiRetVal uiODServiceMgr::startApp( const OD::JSON::Object* jsonobj )
{
    uiRetVal uirv;
    if ( !jsonobj )
    {
	uirv = tr("Empty startApp request");
	return uirv;
    }

    if ( !jsonobj->isPresent(sKey::Name()) )
    {
	uirv = tr("No application name to start");
	return uirv;
    }

    const BufferString appname( jsonobj->getStringValue( sKey::Name() ) );
    if ( appname==sKey::NN3D() || appname==sKey::NN2D() )
    {
	while ( true )
	{
	    if ( !ODMainWin()->applMgr().editNLA( appname==sKey::NN2D() ) )
		break;
	}
    }
    else if ( appname==sKey::UVQ3D() )
	ODMainWin()->applMgr().uvqNLA( false );
    else if ( appname==sKey::UVQ2D() )
	ODMainWin()->applMgr().uvqNLA( true );

    return uiRetVal::OK();
}
