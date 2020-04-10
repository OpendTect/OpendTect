/*+
* ________________________________________________________________________
*
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* Author:	Prajjaval Singh
* Date:		April 2020
* RCS:		$Id$
* ________________________________________________________________________
*
* -*/

#include "odservicebase.h"

#include "commandlineparser.h"
#include "ioman.h"
#include "pythonaccess.h"

#include "filepath.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "netservice.h"
#include "settings.h"
#include "timer.h"


ODServiceBase::ODServiceBase( bool assignport )
{
    const char* skeyport = Network::Server::sKeyPort();
    const char* skeynolisten = Network::Server::sKeyNoListen();
    PortNr_Type portid = 0;

    int defport = 0;
    Settings::common().get( skeyport, defport );

    int clport = 0;
    const CommandLineParser* clp = new CommandLineParser;
    if ( !clp->hasKey(skeynolisten) )
    {
	if ( clp->hasKey( skeyport ) )
	    clp->getVal( skeyport, clport );

	if ( clport > 0 && Network::isPortFree( (PortNr_Type) clport ) )
	    portid = (PortNr_Type) clport;
	else if ( assignport && defport>0
	    && Network::isPortFree((PortNr_Type) defport) )
	    portid = (PortNr_Type) defport;
	else if ( assignport )
	{
	    uiRetVal uirv;
	    portid = Network::getUsablePort( uirv );
	    if ( !uirv.isOK() )
	    {
		pErrMsg( "unable to find usable port" );
		portid = 0;
	    }
	}
    }
    delete clp;

    if ( portid>0 )
	startServer( portid );

    if ( server_ )
	mAttachCB( server_->newConnection, ODServiceBase::newConnectionCB );

    mAttachCB( IOM().surveyChanged, ODServiceBase::surveyChangedCB );
    mAttachCB( IOM().applicationClosing, ODServiceBase::appClosingCB );
    mAttachCB( OD::PythA().envChange, ODServiceBase::pyenvChangeCB );
}


ODServiceBase::~ODServiceBase()
{
    detachAllNotifiers();
    stopServer();
}


bool ODServiceBase::isOK() const
{
    return getAuthority().hasAssignedPort();
}


Network::Authority ODServiceBase::getAuthority() const
{
    return server_ ? server_->getAuthority()
	: Network::Authority();
}


void ODServiceBase::surveyChangedCB( CallBacker* cb )
{ doSurveyChanged(cb); }


void ODServiceBase::appClosingCB( CallBacker* cb )
{ doAppClosing(cb); }


void ODServiceBase::pyenvChangeCB( CallBacker* cb )
{ doPyEnvChange(cb); }


void ODServiceBase::startServer( PortNr_Type portid )
{
    server_ = new Network::RequestServer( portid );
    if ( !server_ || !server_->isOK() )
    {
	pErrMsg( "startServer - failed" );
	stopServer();
	return;
    }
}


void ODServiceBase::stopServer()
{
    deleteAndZeroPtr( server_ );
}


uiRetVal ODServiceBase::doAction( const OD::JSON::Object& actobj )
{
    const BufferString action( actobj.getStringValue( sKeyAction()) );
    uiRetVal ret;
    if ( action == sKeyStatusEv() )
	ret.add( tr("Status") );


    return ret;
}


uiRetVal ODServiceBase::doRequest( const OD::JSON::Object& request )
{
    if ( request.isPresent(sKeyPyEnvChangeEv()) )
	return pythEnvChangedReq( request );
    else if ( request.isPresent(sKeySurveyChangeEv()) )
	return survChangedReq( request );

    return uiRetVal( tr("Unknown JSON packet type: %1")
	.arg( request.dumpJSon() ) );
}


uiRetVal ODServiceBase::doCloseAct()
{
    needclose_ = true;
    return uiRetVal::OK();
}


const OD::JSON::Object* ODServiceBase::getSubObj(
    const OD::JSON::Object& request,
    const char* key )
{
    return request.isPresent(key) ? request.getObject( key ) : nullptr;
}


uiRetVal ODServiceBase::survChangedReq( const OD::JSON::Object& request )
{
    const OD::JSON::Object* paramobj = ODServiceBase::getSubObj( request,
	sKeySurveyChangeEv() );
    if ( paramobj && paramobj->isPresent(sKey::Survey()) )
    {
	const FilePath surveydir( paramobj->getStringValue(sKey::Survey() ) );
	const bool success = IOM().setRootDir( surveydir.pathOnly() ) &&
	    IOM().setSurvey( surveydir.fileName() );
	if ( !success )
	    return uiRetVal( toUiString(IOM().message()) );
    }

    return uiRetVal::OK();
}


uiRetVal ODServiceBase::pythEnvChangedReq( const OD::JSON::Object& request )
{
    const OD::JSON::Object* paramobj = ODServiceBase::getSubObj( request,
	sKeyPyEnvChangeEv() );
    if ( paramobj && paramobj->isPresent(sKey::FileName()) &&
	paramobj->isPresent(sKey::Name()) )
    {
	OD::PythonAccess& pytha = OD::PythA();
	FilePath prevactivatefp;
	if ( pytha.activatefp_ )
	    prevactivatefp = *pytha.activatefp_;
	const BufferString prevvirtenvnm( pytha.virtenvnm_ );
	const FilePath activatefp(
	    paramobj->getStringValue(sKey::FileName()) );
	const BufferString virtenvnm( paramobj->getStringValue(sKey::Name() ) );
	if ( prevactivatefp != activatefp || prevvirtenvnm != virtenvnm )
	{
	    NotifyStopper ns( pytha.envChange );
	    if ( activatefp.isEmpty() )
		deleteAndZeroPtr( pytha.activatefp_ );
	    else if ( pytha.activatefp_ )
		*pytha.activatefp_ = activatefp;
	    else
		pytha.activatefp_ = new FilePath( activatefp );
	    pytha.virtenvnm_ = virtenvnm;
	    pytha.istested_ = true;
	    pytha.isusable_ = true;
	    pytha.envChangeCB( nullptr );
	}
    }

    return uiRetVal::OK();
}


void ODServiceBase::getPythEnvRequestInfo( OD::JSON::Object& sinfo )
{
    const OD::PythonAccess& pytha = OD::PythA();
    sinfo.set( sKey::FileName(), pytha.activatefp_
	? pytha.activatefp_->fullPath()
	: BufferString::empty() );
    sinfo.set( sKey::Name(), pytha.virtenvnm_ );
}


void ODServiceBase::newConnectionCB( CallBacker* )
{
    Network::RequestConnection* conn = server_->pickupNewConnection();
    if ( !conn || !conn->isOK() )
    {
	BufferString err("newConnectionCB - connection error: ");
	err.add( conn->errMsg().getFullString() );
	pErrMsg(err);
	return;
    }

    mAttachCB( conn->packetArrived, ODServiceBase::packetArrivedCB );
    mAttachCB( conn->connectionClosed, ODServiceBase::connClosedCB );
}


void ODServiceBase::packetArrivedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );

    conn_ = static_cast<Network::RequestConnection*>( cber );
    if ( !conn_ )
	return;

    packet_ = conn_->pickupPacket( reqid, 2000 );
    if ( !packet_ )
    {
	packet_ = conn_->getNextExternalPacket();
	if ( !packet_ )
	{
	    pErrMsg("packetArrivedCB - no packet");
	    return;
	}
    }

    OD::JSON::Object request;
    uiRetVal uirv = packet_->getPayload( request );
    if ( !uirv.isOK() )
    {
	sendErr( uirv );
	packet_.release();
	return;
    }

    uirv = request.isPresent(sKeyAction()) ? doAction( request )
	: doRequest( request );
    if ( uirv.isOK() )
	sendOK();
    else
	sendErr( uirv );
}


void ODServiceBase::connClosedCB( CallBacker* cb )
{
    Network::RequestConnection* conn = (Network::RequestConnection*) cb;
    if ( !conn )
	return;

    mDetachCB( conn->packetArrived, ODServiceBase::packetArrivedCB );
    mDetachCB( conn->connectionClosed, ODServiceBase::connClosedCB );
    conn_ = nullptr;

}


uiRetVal ODServiceBase::sendAction( const Network::Authority& auth,
    const char* servicenm,
    const char* action )
{
    PtrMan<Network::RequestConnection> conn =
	new Network::RequestConnection( auth, false, 2000 );
    if ( !conn || !conn->isOK() )
    {
	return uiRetVal(tr("Cannot connect to service %1 on %2")
	    .arg(servicenm).arg(auth.toString(true)) );
    }

    PtrMan<Network::RequestPacket> packet = new Network::RequestPacket;
    packet->setIsNewRequest();
    OD::JSON::Object request;
    request.set( sKeyAction(), action );

    packet->setPayload( request );
    if ( !conn->sendPacket(*packet) )
    {
	return uiRetVal(tr("Message failure to service %1 on %2")
	    .arg(servicenm).arg(auth.toString(true)) );
    }

    ConstPtrMan<Network::RequestPacket> receivedpacket =
	conn->pickupPacket( packet->requestID(), 2000 );
    if ( !receivedpacket )
	return uiRetVal(tr("Did not receive response from %1").arg(servicenm));

    OD::JSON::Object response;
    const uiRetVal uirv = receivedpacket->getPayload( response );
    if ( !uirv.isOK() )
	return uirv;
    else if ( response.isPresent(sKeyError()) )
    {
	return uiRetVal( tr("%1 error: %2").arg(servicenm)
	    .arg(response.getStringValue(sKeyError())));
    }

    return uiRetVal::OK();
}


uiRetVal ODServiceBase::sendRequest( const Network::Authority& auth,
    const char* servicenm,
    const char* reqkey,
    const OD::JSON::Object& reqobj )
{
    PtrMan<Network::RequestConnection> conn =
	new Network::RequestConnection( auth, false, 2000 );
    if ( !conn || !conn->isOK() )
    {
	return uiRetVal(tr("Cannot connect to %1 server on %2")
	    .arg(servicenm).arg(auth.toString(true)) );
    }

    PtrMan<Network::RequestPacket> packet = new Network::RequestPacket;
    packet->setIsNewRequest();
    OD::JSON::Object request;
    request.set( reqkey, reqobj.clone() );

    packet->setPayload( request );
    if ( packet && conn && !conn->sendPacket(*packet,true) )
	return uiRetVal( tr("Request packet failed.") );

    return uiRetVal::OK();
}


void ODServiceBase::sendOK()
{
    OD::JSON::Object response;
    response.set( sKeyOK(), BufferString::empty() );
    if ( packet_ )
	packet_->setPayload( response );
    if ( packet_ && conn_ && !conn_->sendPacket(*packet_.ptr()) )
    { pErrMsg("sendOK - failed"); }
    packet_.release();
}


void ODServiceBase::sendErr( uiRetVal& uirv )
{
    OD::JSON::Object response;
    response.set( sKeyError(), uirv.getText() );
    if ( packet_ )
	packet_->setPayload( response );
    if ( packet_ && conn_ && !conn_->sendPacket(*packet_.ptr()) )
    { pErrMsg("sendErr - failed"); }
    packet_.release();
}
