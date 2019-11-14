/*+
 _ __________________________________________*_____________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Oct 2019
 ________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodservicemgr.h"

#include "uidialog.h"

#include "ioman.h"
#include "ptrman.h"
#include "thread.h"
#include "uimain.h"
#include "uiodmain.h"
#include "envvars.h"
#include "netreqconnection.h"
#include "netserver.h"
#include "netreqpacket.h"
#include "uimsg.h"
#include "settings.h"
#include "pythonaccess.h"
#include "genc.h"
#include "keystrs.h"
#include "oddirs.h"

/*!\brief The OpendTect service manager */

using namespace Network;

uiODServiceMgr& uiODServiceMgr::getMgr()
{
    mDefineStaticLocalObject(uiODServiceMgr,mgrInstance,);
    return mgrInstance;
}


uiODServiceMgr::uiODServiceMgr()
{
    mAttachCB( server_->newConnection, uiODServiceMgr::newConnectionCB );
    mAttachCB( IOM().surveyChanged,uiODServiceMgr::surveyChangedCB );
    mAttachCB( IOM().applicationClosing,uiODServiceMgr::appClosingCB );
    mAttachCB( OD::PythA().envChange,uiODServiceMgr::pyenvChangeCB );
}


uiODServiceMgr::~uiODServiceMgr()
{
    detachAllNotifiers();
}


BufferString uiODServiceMgr::address() const
{
    BufferString addr( GetLocalHostName() );
    addr.add(":").add( port() );

    return addr;
}


uiRetVal uiODServiceMgr::addService( const OD::JSON::Object* jsonobj )
{
    Network::Service* tmp = new Network::Service;;
    uiRetVal uirv = tmp->useJSON( *jsonobj );
    if ( uirv.isOK() ) {
	services_.add( tmp );
    }
    return uirv;
}


uiRetVal uiODServiceMgr::removeService( const OD::JSON::Object* jsonobj )
{
    Network::Service tmp;
    uiRetVal uirv = tmp.useJSON( *jsonobj );
    if ( uirv.isOK() )
	removeService( tmp );

    return uirv;
}


void uiODServiceMgr::removeService( const Network::Service& service )
{
    int idx = indexOfService( service );
    if ( idx>=0 )
    {
	services_.removeSingle( idx );
    }
}


int uiODServiceMgr::indexOfService( const Network::Service& service ) const
{
    for ( int idx=0; idx<services_.size(); idx++ )
    {
	if ( *services_[idx] == service )
	    return idx;
    }
    return -1;
}


uiRetVal uiODServiceMgr::sendAction( const Network::Service& service,
					   const char* action )
{
    int idx = indexOfService( service );
    if ( idx>=0 )
	return sendAction( idx, action );
    else
	return uiRetVal(tr("Unknown service"));
}


uiRetVal uiODServiceMgr::sendAction( int idx, const char* action )
{
    const BufferString servicenm( services_[idx]->name() );
    const BufferString hostname( services_[idx]->hostname() );
    const port_nr_type portID = services_[idx]->port();

    PtrMan<RequestConnection> conn = new RequestConnection( hostname, portID,
							    false, 2000 );
    if ( !conn || !conn->isOK() )
	return uiRetVal(tr("Cannot connect to service %1 on %2:%3")
		    .arg(servicenm).arg(hostname).arg(portID) );

    PtrMan<RequestPacket> packet =  new RequestPacket;
    packet->setIsNewRequest();
    OD::JSON::Object request;
    request.set( sKeyAction(), action );

    packet->setPayload( request );

    if ( !conn->sendPacket( *packet ) )
	return uiRetVal(tr("Message failure to service %1 on %2:%3")
		    .arg(servicenm).arg(hostname).arg(portID) );

    ConstPtrMan<Network::RequestPacket> receivedpacket =
			    conn->pickupPacket( packet->requestID(), 2000 );
    if ( !receivedpacket )
	return uiRetVal(tr("Did not receive response from %1").arg(servicenm));

    OD::JSON::Object response;
    uiRetVal uirv = receivedpacket->getPayload( response );
    if ( !uirv.isOK() )
	return uirv;
    else if ( response.isPresent( sKeyError() ) )
	return uiRetVal( tr("%1 error: %2").arg(servicenm)
			.arg(response.getStringValue(sKeyError())));

    return uiRetVal::OK();
}


uiRetVal uiODServiceMgr::sendAction( const Network::Service& service,
			const char* action, OD::JSON::Object* paramobj )
{
    int idx = indexOfService( service );
    if ( idx>=0 )
	return sendAction( idx, action, paramobj );
    else
	return uiRetVal(tr("Unknown service"));
}


uiRetVal uiODServiceMgr::sendAction( int idx, const char* action,
				     OD::JSON::Object* paramobj )
{
    BufferString servicenm( services_[idx]->name() );
    BufferString hostname( services_[idx]->hostname() );
    port_nr_type portID = services_[idx]->port();

    PtrMan<RequestConnection> conn = new RequestConnection( hostname, portID,
							    false, 2000 );
    if ( !conn || !conn->isOK() )
	return uiRetVal(tr("Cannot connect to service %1 on %2:%3")
	.arg(servicenm).arg(hostname).arg(portID) );

    PtrMan<RequestPacket> packet =  new RequestPacket;
    packet->setIsNewRequest();
    OD::JSON::Object request;
    request.set( sKeyAction(), action );
    request.set( sKey::Pars(), paramobj );

    packet->setPayload( request );

    if ( !conn->sendPacket( *packet ) )
	return uiRetVal(tr("Message failure to service %1 on %2:%3")
	.arg(servicenm).arg(hostname).arg(portID) );

    ConstPtrMan<Network::RequestPacket> receivedpacket =
    conn->pickupPacket( packet->requestID(), 2000 );
    if ( !receivedpacket )
	return uiRetVal(tr("Did not receive response from %1").arg(servicenm));

    OD::JSON::Object response;
    uiRetVal uirv = receivedpacket->getPayload( response );
    if ( !uirv.isOK() )
	return uirv;
    else if ( response.isPresent( sKeyError() ) )
	return uiRetVal( tr("%1 error: %2").arg(servicenm)
	.arg(response.getStringValue(sKeyError())));

    return uiRetVal::OK();
}
void uiODServiceMgr::raise( const Network::Service& service )
{
    uiRetVal uirv = sendAction( service, sKeyRaiseEv() );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}

void uiODServiceMgr::surveyChangedCB( CallBacker* )
{
    OD::JSON::Object* paramobj = new OD::JSON::Object;
    paramobj->set( sKey::Survey(), GetDataDir() );

    for (int idx=0; idx< services_.size(); idx++) {
	uiRetVal uirv = sendAction( idx, sKeySurveyChangeEv(), paramobj );
	if (!uirv.isOK())
	    uiMSG().error(uirv);
    }
}


void uiODServiceMgr::appClosingCB( CallBacker* )
{
    for (int idx=0; idx< services_.size(); idx++) {
	uiRetVal uirv = sendAction( idx, sKeyCloseEv() );
    }
}


void uiODServiceMgr::pyenvChangeCB( CallBacker* )
{
    for (int idx=0; idx< services_.size(); idx++) {
	uiRetVal uirv = sendAction( idx, sKeyPyEnvChangeEv() );
	if (!uirv.isOK())
	    uiMSG().error(uirv);
    }
}

void uiODServiceMgr::newConnectionCB( CallBacker* )
{
    RequestConnection* conn = server_->pickupNewConnection();
    if ( !conn || !conn->isOK() )
    {
	BufferString err("newConnectionCB - connection error: ");
	pErrMsg(err);
	return;
    }

    mAttachCB( conn->packetArrived, uiODServiceMgr::packetArrivedCB );
    mAttachCB( conn->connectionClosed, uiODServiceMgr::connClosedCB );
}

void uiODServiceMgr::packetArrivedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );

    RequestConnection* conn = static_cast<RequestConnection*>( cber );

    PtrMan<RequestPacket> packet = conn->pickupPacket( reqid, 2000 );
    if ( !packet )
    {
	packet = conn->getNextExternalPacket();
	if ( !packet )
	{
	    pErrMsg("packetArrivedCB - no packet");
	    return;
	}
    }
    OD::JSON::Object request;
    uiRetVal uirv = packet->getPayload( request );
    if ( !uirv.isOK() )
    {
	sendErr( conn, packet, uirv );
	return;
    }

    if ( request.isPresent( sKeyRegister() ) )
	uirv = addService( request.getObject( sKeyRegister() ) );
    else if ( request.isPresent( sKeyDeregister() ) )
	uirv = removeService( request.getObject( sKeyDeregister() ) );
    else if ( request.isPresent( sKeyAction() ) )
	uirv = doAction( request.getObject(sKeyAction() ) );

    if ( uirv.isOK() )
	sendOK( conn, packet );
    else
	sendErr( conn, packet, uirv );
}


void uiODServiceMgr::connClosedCB( CallBacker* cb )
{
    RequestConnection* conn = (RequestConnection*) cb;

    mDetachCB( conn->packetArrived, uiODServiceMgr::packetArrivedCB );
    mDetachCB( conn->connectionClosed, uiODServiceMgr::connClosedCB );
}


uiRetVal uiODServiceMgr::doAction( const OD::JSON::Object* actobj )
{
    if ( actobj->isPresent( sKeyAction() ) ) {
	BufferString action( actobj->getStringValue( sKeyAction() ) );
	if ( action == sKeyPyEnvChangeEv() )
	    uiMSG().message(tr("Change Python Environment"));
    }
    return uiRetVal::OK();
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
