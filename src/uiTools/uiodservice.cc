/*+
 _ __________________________________________*_____________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Oct 2019
 ________________________________________________________________________

 -*/

#include "uiodservice.h"

#include "uimain.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "envvars.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "commandlineparser.h"
#include "settings.h"
#include "odjson.h"

/*!\brief Base class for OpendTect Service Manager and external services/apps */

uiODServiceBase::uiODServiceBase( bool assignport )
{
    typedef Network::RequestConnection NRC;
    const char* skeyport = Network::Server::sKeyPort();
    port_nr_type portid = 0;

    int defport = 0;
    Settings::common().get( skeyport, defport );

    int clport = 0;
    const CommandLineParser& clp = uiMain::CLP();
    clp.setKeyHasValue( skeyport );
    if ( clp.hasKey( skeyport ) )
	clp.getKeyedInfo( skeyport, clport );

    if ( clport > 0 && NRC::isPortFree( (port_nr_type) clport ) )
	portid = (port_nr_type) clport;
    else if ( assignport && defport>0
			 && NRC::isPortFree((port_nr_type) defport) )
	portid = (port_nr_type) defport;
    else if ( assignport )
    {
	uiRetVal uirv;
	portid = NRC::getUsablePort( uirv );
	if ( !uirv.isOK() ) {
	    pErrMsg( "unable to find usable port" );
	    portid = 0;
	}
    }

    if ( portid>0 )
	startServer( portid );
}


uiODServiceBase::~uiODServiceBase()
{
    stopServer();
}


void uiODServiceBase::startServer( port_nr_type portid )
{
    server_ = new Network::RequestServer( portid );
    if ( !server_ || !server_->isOK() )
    {
	pErrMsg( "startServer - failed" );
	stopServer();
	return;
    }
}


void uiODServiceBase::stopServer()
{
    deleteAndZeroPtr( server_ );
}


void uiODServiceBase::sendOK( Network::RequestConnection* conn,
				 RefMan<Network::RequestPacket> packet )
{
    OD::JSON::Object response;
    response.set( sKeyOK(), BufferString::empty() );
    packet->setPayload( response );
    if ( !conn->sendPacket( *packet ) )
	pErrMsg("sendOK - failed");
}


void uiODServiceBase::sendErr( Network::RequestConnection* conn,
			RefMan<Network::RequestPacket> packet, uiRetVal& uirv )
{
    sendErr( conn, packet, uirv.getText() );
}


void uiODServiceBase::sendErr( Network::RequestConnection* conn,
		    RefMan<Network::RequestPacket> packet, const char* msg )
{
    OD::JSON::Object response;
    response.set( sKeyError(), msg );
    packet->setPayload( response );
    if ( !conn->sendPacket( *packet ) )
	pErrMsg("sendErr - failed");
}



uiODService::uiODService( bool assignport )
    : uiODServiceBase(assignport)
    , odport_(0)
    , serviceinfo_(port())
{
    BufferString odserverStr;
    const CommandLineParser& clp = uiMain::CLP();
    clp.setKeyHasValue( sKeyODServer() );
    if ( clp.hasKey( sKeyODServer() ) )
    {
	clp.getKeyedInfo( sKeyODServer(), odserverStr );
	BufferStringSet tmp;
	tmp.unCat( odserverStr, ":" );
	if ( tmp.size()==2 && tmp.get(1).isNumber(true) )
	{
	    odhostname_ = tmp.get(0);
	    odport_ = tmp.get(1).toInt();
	}
    }

    if ( server_ )
	mAttachCB( server_->newConnection, uiODService::newConnectionCB );

    doRegister();
}


uiODService::~uiODService()
{
    detachAllNotifiers();
}

void uiODService::newConnectionCB( CallBacker* )
{
    Network::RequestConnection* conn = server_->pickupNewConnection();
    if ( !conn || !conn->isOK() )
    {
	BufferString err("newConnectionCB - connection error: ");
	err += conn->errMsg();
	pErrMsg(err);
	return;
    }

    mAttachCB( conn->packetArrived, uiODService::packetArrivedCB );
    mAttachCB( conn->connectionClosed, uiODService::connClosedCB );
}

void uiODService::packetArrivedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );

    Network::RequestConnection* conn =
			static_cast<Network::RequestConnection*>( cber );

    RefMan<Network::RequestPacket> packet = conn->pickupPacket( reqid, 2000 );
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

    if ( !request.isPresent( sKeyAction()) )
    {
	BufferString err( "unknown JSON packet type: ");
	err += request.dumpJSon();
	sendErr( conn, packet, err );
	return;
    }

    BufferString action( request.getStringValue( sKeyAction()) );
    if ( action == sKeyCloseEv() )
	sendOK( conn, packet );

    uirv = doAction( action );
    if ( uirv.isOK() )
	sendOK( conn, packet );
    else
	sendErr( conn, packet, uirv );
}

void uiODService::connClosedCB( CallBacker* cb )
{
    Network::RequestConnection* conn = (Network::RequestConnection*) cb;
    mDetachCB( conn->packetArrived, uiODService::packetArrivedCB );
    mDetachCB( conn->connectionClosed, uiODService::connClosedCB );
}

uiRetVal uiODService::doAction( BufferString action )
{
    if ( action == sKeyCloseEv() )
	uiMain::theMain().exit(0);
    else if ( action == sKeyPyEnvChangeEv() )
	statusMsg(tr("Python Environment Change"));
    else if ( action == sKeyRaiseEv() )
    {
	uiMain::theMain().topLevel()->show();
	uiMain::theMain().topLevel()->raise();
    }
    else if ( action == sKeyStatusEv() )
	statusMsg(tr("Status"));
    else if ( action == sKeySurveyChangeEv() )
	statusMsg(tr("Survey Change"));

    return uiRetVal::OK();
}

void uiODService::statusMsg( uiString msg )
{
    uiMain::theMain().topLevel()->statusBar()->message( msg );
}

uiRetVal uiODService::doRegister()
{
    if ( mIsUdf(odport_) )
	return uiRetVal::OK();
    PtrMan<Network::RequestConnection> conn =
	new Network::RequestConnection( odhostname_, odport_, false, 2000 );
    if ( !conn || !conn->isOK() )
	return uiRetVal(tr("Cannot connect to ODMain server on %2:%3")
	.arg(odhostname_).arg(odport_) );

    RefMan<Network::RequestPacket> packet =  new Network::RequestPacket;
    packet->setIsNewRequest();
    OD::JSON::Object* sinfo = new OD::JSON::Object;
    serviceinfo_.fillJSON( *sinfo );
    OD::JSON::Object request;
    request.set( sKeyRegister(), sinfo );
    packet->setPayload( request );
    if ( packet && conn && !conn->sendPacket( *packet, true ) ) {
	return uiRetVal(tr("Registration of service: %1 failed")
	.arg(serviceinfo_.name()) );
    }
    return uiRetVal::OK();
}


uiRetVal uiODService::doDeRegister()
{
    if ( mIsUdf(odport_) )
	return uiRetVal::OK();

    PtrMan<Network::RequestConnection> conn =
	new Network::RequestConnection( odhostname_, odport_, false, 2000 );
    if ( !conn || !conn->isOK() )
	return uiRetVal(tr("Cannot connect to ODMain server on %2:%3")
	.arg(odhostname_).arg(odport_) );

    RefMan<Network::RequestPacket> packet =  new Network::RequestPacket;
    packet->setIsNewRequest();
    OD::JSON::Object* sinfo = new OD::JSON::Object;
    serviceinfo_.fillJSON( *sinfo );

    OD::JSON::Object request;
    request.set( sKeyDeregister(), sinfo );

    packet->setPayload( request );
    if ( !conn->sendPacket( *packet, true ) )
	return uiRetVal(tr("DeRegistration of service: %1 failed")
	.arg(serviceinfo_.name()) );

    return uiRetVal::OK();
}

uiRetVal uiODService::sendAction( OD::JSON::Object* actobj )
{
    PtrMan<Network::RequestConnection> conn =
	new Network::RequestConnection( odhostname_, odport_, false, 2000 );
    if ( !conn || !conn->isOK() )
	return uiRetVal(tr("Cannot connect to ODServiceMgr %1 on %1:%2")
	.arg(odhostname_).arg(odport_) );

    RefMan<Network::RequestPacket> packet =  new Network::RequestPacket;
    packet->setIsNewRequest();
    OD::JSON::Object request;
    request.set( sKeyAction(), actobj );

    packet->setPayload( request );

    if ( !conn->sendPacket( *packet ) )
	return uiRetVal(tr("Message failure to ODServiceMgr on %1:%2")
	.arg(odhostname_).arg(odport_) );

    ConstRefMan<Network::RequestPacket> receivedpacket =
    conn->pickupPacket( packet->requestID(), 2000 );
    if ( !receivedpacket )
	return uiRetVal(tr("Did not receive response from ODServiceMgr"));

    OD::JSON::Object response;
    uiRetVal uirv = receivedpacket->getPayload( response );
    if ( !uirv.isOK() )
	return uirv;
    else if ( response.isPresent( sKeyError() ) )
	return uiRetVal( tr("ODServiceMgr error: %1")
		    .arg(response.getStringValue(sKeyError())));

    return uiRetVal::OK();
}
