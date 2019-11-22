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
#include "uimainwin.h"
#include "uimsg.h"

#include "ioman.h"
#include "keystrs.h"
#include "oddirs.h"
#include "netreqconnection.h"
#include "netreqpacket.h"


/*!\brief The OpendTect service manager */

using namespace Network;

uiODServiceMgr& uiODServiceMgr::getMgr()
{
    mDefineStaticLocalObject(uiODServiceMgr,mgrInstance,);
    return mgrInstance;
}


uiODServiceMgr::uiODServiceMgr()
    : serviceAdded(this)
    , serviceRemoved(this)
{
}


uiODServiceMgr::~uiODServiceMgr()
{
    doAppClosing( nullptr );
    deepErase( services_ );
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
				     const char* action,
				     const OD::JSON::Object* paramobj ) const
{
    const Network::Service* service = getService( servid );
    if ( !service )
	return uiRetVal( tr("Service with ID %1 not registered").arg( servid ));

    return sendAction( *service, action, paramobj );
}


uiRetVal uiODServiceMgr::sendAction( const Network::Service& service,
				     const char* action,
				     const OD::JSON::Object* paramobj ) const
{
    const BufferString servicenm( "Service ", service.name() );
    return uiODServiceBase::sendAction( service.getAuthority(),
					servicenm, action, paramobj );
}


uiRetVal uiODServiceMgr::sendRequest( const Network::Service& service,
				      const char* reqkey,
				      const OD::JSON::Object& reqinfo ) const
{
    const BufferString servicenm( "Service ", service.name() );
    return uiODServiceBase::sendRequest( service.getAuthority(),
					 servicenm, reqkey, reqinfo );
}


bool uiODServiceMgr::isPresent( const Network::Service::ID servid ) const
{
    return getService( servid );
}


BufferString uiODServiceMgr::serviceName(
				    const Network::Service::ID servid ) const
{
    const Network::Service* service = getService( servid );
    return service ? service->name() : BufferString::empty();
}


void uiODServiceMgr::raise( const Network::Service::ID servid ) const
{
    const uiRetVal uirv = sendAction( servid, sKeyRaiseEv() );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}


void uiODServiceMgr::doAppClosing( CallBacker* )
{
    for ( int idx=services_.size()-1; idx>=0; idx-- )
	removeService( services_[idx]->getID() );
}


void uiODServiceMgr::doSurveyChanged( CallBacker* )
{
    OD::JSON::Object paramobj;
    paramobj.set( sKey::Survey(), GetDataDir() );
    for ( int idx=0; idx<services_.size(); idx++ )
    {
	const uiRetVal uirv = sendAction( *services_[idx], sKeySurveyChangeEv(),
					  &paramobj );
	if ( !uirv.isOK() )
	    uiMSG().error( uirv );
    }
}


void uiODServiceMgr::doPyEnvChange( CallBacker* )
{
    OD::JSON::Object sinfo;
    uiODServiceBase::getPythEnvRequestInfo( sinfo );
    for ( int idx=0; idx<services_.size(); idx++ )
    {
	const uiRetVal uirv = sendRequest( *services_[idx], sKeyPyEnvChangeEv(),
					   sinfo );
	if ( !uirv.isOK() )
	    uiMSG().error( uirv );
    }
}


uiRetVal uiODServiceMgr::doRequest( const OD::JSON::Object& request )
{
    if ( request.isPresent(sKeyRegister()) )
	return addService( request.getObject(sKeyRegister()) );
    else if ( request.isPresent(sKeyDeregister()) )
	return removeService( request.getObject(sKeyDeregister()) );

    return uiODServiceBase::doRequest( request );
}

/*
class uiODRequestServerDlg : public uiDialog
{ mODTextTranslationClass(uiODRequestServerDlg);
public:
    uiODRequestServerDlg( uiParent* p )
    : uiDialog(p,Setup(tr("External Services"),mNoDlgTitle,mTODOHelpKey))
    {
	setCtrlStyle( CloseOnly );
	setShrinkAllowed(true);

	uiListBox::Setup lsu( OD::ChooseOnlyOne, tr("Service") );
	servicefld_ = new uiListBox( this, lsu, "Services" );

	butgrp_ = new uiButtonGroup( this, "buttons", OD::Vertical );
	butgrp_->attach( rightOf, servicefld_ );
    }

protected:
    bool acceptOK( CallBacker* )
    {
	return true;
    }

    uiListBox		servicefld_;
    uiButtonGrp*	butgrp_;

};
*/
