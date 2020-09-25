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

#include "applicationdata.h"
#include "commandlineparser.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "netservice.h"
#include "pythonaccess.h"
#include "settings.h"


ODServiceBase::ODServiceBase()
    : externalAction(this)
    , externalRequest(this)
{
    addLocalServer();
}


ODServiceBase::ODServiceBase( bool assignport, Network::SpecAddr spec )
    : externalAction(this)
    , externalRequest(this)
{
    addTCPServer( assignport, spec );
}


bool ODServiceBase::addLocalServer()
{
    init( true );
    return isOK( true );
}


bool ODServiceBase::addTCPServer( bool assignport, Network::SpecAddr spec )
{
    init( false, assignport, spec );
    return isOK( false );
}


bool& ODServiceBase::serverIsMine( bool islocal )
{ return islocal ? localserverismine_ : tcpserverismine_; }


void ODServiceBase::init( bool islocal, bool assignport, Network::SpecAddr spec)
{
    ODServiceBase* mainserv = theMain();
    if ( islocal )
	localserver_ = mainserv ? mainserv->localserver_ : nullptr;
    else
	tcpserver_ = mainserv ? mainserv->tcpserver_ : nullptr;

    if ( isServerOK(islocal) )
    {
	serverIsMine(islocal) = false;
	if ( this != mainserv )
	{
	    mAttachCB( mainserv->externalAction,
		       ODServiceBase::externalActionCB );
	    mAttachCB( mainserv->externalRequest,
		       ODServiceBase::externalRequestCB );
	}
	return;
    }

    const char* skeyport = Network::Server::sKeyPort();
    const char* skeynolisten = Network::Server::sKeyNoListen();
    PortNr_Type portid = 0;

    int defport = 0;
    Settings::common().get( skeyport, defport );

    int clport = 0;
    const CommandLineParser clp;
    if ( !clp.hasKey(skeynolisten) )
    {
	if ( clp.hasKey( skeyport ) )
	    clp.getVal( skeyport, clport );

	if ( clport > 0 && Network::isPortFree( (PortNr_Type) clport ) )
	    portid = (PortNr_Type) clport;
	else if ( assignport && defport>0 &&
		  Network::isPortFree((PortNr_Type) defport) )
	    portid = (PortNr_Type) defport;
	else if ( assignport && !islocal )
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

    if ( portid>0 || islocal )
    {
	Network::RequestServer* server = islocal
	    ? new Network::RequestServer(
                Network::Authority::getAppServerName("odservice"))
	    : new Network::RequestServer( portid, spec );
	useServer( server, islocal );
    }

    if ( !isServerOK(islocal) )
	return;

    if ( islocal )
	mAttachCB( localserver_->newConnection,
					    ODServiceBase::newConnectionCB );
    else
	mAttachCB( tcpserver_->newConnection, ODServiceBase::newConnectionCB );

    mAttachCB( IOM().surveyChanged, ODServiceBase::surveyChangedCB );
    mAttachCB( IOM().applicationClosing, ODServiceBase::appClosingCB );
    mAttachCB( OD::PythA().envChange, ODServiceBase::pyenvChangeCB );
    mAttachCB( this->externalAction, ODServiceBase::externalActionCB );
    mAttachCB( this->externalRequest, ODServiceBase::externalRequestCB );
}


ODServiceBase::~ODServiceBase()
{
    doAppClosing( nullptr );
}


bool ODServiceBase::isOK( bool islocal	) const
{
    return getAuthority( islocal  ).isUsable();
}


Network::Authority ODServiceBase::getAuthority( bool islocal ) const
{
    Network::RequestServer* server =
		islocal ? localserver_ : tcpserver_;
    return server ? server->getAuthority() : Network::Authority();
}


bool ODServiceBase::isMainService() const
{
    return this == theMain();
}


void ODServiceBase::surveyChangedCB( CallBacker* cb )
{ doSurveyChanged(cb); }


void ODServiceBase::appClosingCB( CallBacker* cb )
{ doAppClosing(cb); }


void ODServiceBase::pyenvChangeCB( CallBacker* cb )
{ doPyEnvChange(cb); }


bool ODServiceBase::useServer( Network::RequestServer* server, bool islocal )
{
    if ( !server || !server->isOK() )
    {
	delete server;
	return false;
    }

    if ( islocal )
	localserver_ = server;
    else
	tcpserver_ = server;

    if ( !isServerOK(islocal) )
    {
	pErrMsg( "startServer - failed" );
	stopServer( islocal );
	return false;
    }

    if ( !theMain() )
	theMain( this );

    return true;
}


bool ODServiceBase::isServerOK( bool islocal ) const
{
    if ( islocal )
	return localserver_ ? localserver_->isOK() : false;

    return tcpserver_ ? tcpserver_->isOK() : false;
}


void ODServiceBase::stopServer( bool islocal )
{
    if ( serverIsMine(islocal) )
    {
	if ( islocal )
	    deleteAndZeroPtr( localserver_ );
	else
	    deleteAndZeroPtr( tcpserver_ );
    }
}


uiRetVal ODServiceBase::doAction( const OD::JSON::Object& actobj )
{
    const BufferString action( actobj.getStringValue( sKeyAction()) );
    uirv_ = tr("Unknown action: %1").arg( action );
    externalAction.trigger( action );

    return uirv_;
}


uiRetVal ODServiceBase::doRequest( const OD::JSON::Object& request )
{
    uirv_ = tr("Unknown JSON packet type: %1").arg( request.dumpJSon() );
    externalRequest.trigger( &request );

    return uirv_;
}


bool ODServiceBase::doParseAction( const char* action, uiRetVal& uirv )
{
    if ( FixedString(action) == sKeyStatusEv() )
    {
	uirv = uiRetVal::OK();
	return true;
    }
    else if ( FixedString(action) == sKeyCloseEv() )
    {
	uirv = doPrepareForClose();
	return true;
    }

    return false;
}


bool ODServiceBase::doParseRequest( const OD::JSON::Object& request,
				    uiRetVal& uirv )
{
    if ( request.isPresent(sKeyPyEnvChangeEv()) )
    {
	uirv = pythEnvChangedReq( request );
	return true;
    }
    else if ( request.isPresent(sKeySurveyChangeEv()) )
    {
	uirv = survChangedReq( request );
	return true;
    }

    return false;
}


uiRetVal ODServiceBase::doPrepareForClose()
{
    needclose_ = true;
    return uiRetVal::OK();
}


void ODServiceBase::closeApp()
{
    IOM().applClosing();
    ApplicationData::exit( 0 );
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
    //TODO: Capsule to tell us from which server is comes
    Network::RequestConnection* conn = localserver_
				     ? localserver_->pickupNewConnection()
				     : tcpserver_->pickupNewConnection();
    if ( !conn || !conn->isOK() )
    {
	BufferString err("newConnectionCB - connection error: ");
	err += conn->errMsg();
	pErrMsg(err);
	return;
    }

    mAttachCB( conn->packetArrived, ODServiceBase::packetArrivedCB );
    mAttachCB( conn->connectionClosed, ODServiceBase::connClosedCB );
}


void ODServiceBase::packetArrivedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );
    //TODO: no local server packet pickup?

    tcpconn_ = static_cast<Network::RequestConnection*>(cber);
    if ( !tcpconn_ )
	return;

    packet_ = tcpconn_->pickupPacket( reqid, 2000 );
    if ( !packet_ )
    {
	packet_ = tcpconn_->getNextExternalPacket();
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
    tcpconn_ = nullptr;
    localconn_ = nullptr;

    doConnClosed( cb );
    if ( needclose_ )
	closeApp();
}


void ODServiceBase::externalActionCB( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(BufferString,actstr,cb);
    if ( actstr.isEmpty() )
	return;

    uiRetVal uirv;
    if ( doParseAction(actstr.buf(),uirv) )
	uirv_ = uirv;
}


void ODServiceBase::externalRequestCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const OD::JSON::Object*,reqobj,cb);
    if ( !reqobj )
	return;

    uiRetVal uirv;
    if ( doParseRequest(*reqobj,uirv) )
	uirv_ = uirv;
}


uiRetVal ODServiceBase::sendAction( const Network::Authority& auth,
				    const char* servicenm, const char* action )
{
    PtrMan<Network::RequestConnection> conn =
		    new Network::RequestConnection( auth, false, 2000 );
    if ( !conn || !conn->isOK() )
    {
	return uiRetVal(tr("Cannot connect to service %1 on %2")
			    .arg(servicenm).arg(auth.toString()) );
    }

    RefMan<Network::RequestPacket> packet = new Network::RequestPacket;
    packet->setIsNewRequest();
    OD::JSON::Object request;
    request.set( sKeyAction(), action );

    packet->setPayload( request );
    if ( !conn->sendPacket(*packet) )
    {
	return uiRetVal(tr("Message failure to service %1 on %2")
			    .arg(servicenm).arg(auth.toString()) );
    }

    ConstRefMan<Network::RequestPacket> receivedpacket =
		    conn->pickupPacket( packet->requestID(), 15000 );
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
				     const char* servicenm, const char* reqkey,
				     const OD::JSON::Object& reqobj )
{
    PtrMan<Network::RequestConnection> conn =
		    new Network::RequestConnection( auth, false, 2000 );
    if ( !conn || !conn->isOK() )
    {
	return uiRetVal(tr("Cannot connect to %1 server on %2")
			    .arg(servicenm).arg(auth.toString()) );
    }

    RefMan<Network::RequestPacket> packet = new Network::RequestPacket;
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
    if ( packet_ && tcpconn_ && !tcpconn_->sendPacket(*packet_.ptr()) )
    { pErrMsg("sendOK - failed"); }
    packet_ = nullptr;
}


void ODServiceBase::sendErr( uiRetVal& uirv )
{
    OD::JSON::Object response;
    response.set( sKeyError(), uirv.getText() );
    if ( packet_ )
	packet_->setPayload( response );
    if ( packet_ && tcpconn_ && !tcpconn_->sendPacket(*packet_.ptr()) )
    { pErrMsg("sendErr - failed"); }
    packet_ = nullptr;
}


void ODServiceBase::doAppClosing( CallBacker* )
{
    detachAllNotifiers();
    stopServer( true );
    stopServer( false );
}


ODServiceBase* ODServiceBase::theMain( ODServiceBase* newmain )
{
    mDefineStaticLocalObject( ODServiceBase*, mainservice, = nullptr )
    if ( newmain )
	mainservice = newmain;
    return mainservice;
}
