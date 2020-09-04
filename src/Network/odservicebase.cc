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
#include "ioman.h"
#include "filepath.h"
#include "keystrs.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "netservice.h"
#include "pythonaccess.h"
#include "settings.h"
#include "od_ostream.h"

#include "hiddenparam.h"


static HiddenParam<ODServiceBase,Network::RequestServer*>
					odservbaselocservmgr_(nullptr);
static HiddenParam<ODServiceBase,Network::RequestConnection*>
					odservbaselocalconnmgr_(nullptr);


ODServiceBase::ODServiceBase( bool assignport )
    : externalAction(this)
    , externalRequest(this)
{
    odservbaselocservmgr_.setParam( this, nullptr );
    odservbaselocalconnmgr_.setParam( this, nullptr );
    init( false, nullptr, assignport );
}


ODServiceBase::ODServiceBase( bool islocal, const char* servernm,
							    bool assignport )
    : externalAction(this)
    , externalRequest(this)
{
    odservbaselocservmgr_.setParam( this, nullptr );
    odservbaselocalconnmgr_.setParam( this, nullptr );
    init( islocal, servernm, assignport );
}


ODServiceBase::ODServiceBase( const char* hostname, bool assignport )
    : externalAction(this)
    , externalRequest(this)
{
    odservbaselocservmgr_.setParam( this, nullptr );
    odservbaselocalconnmgr_.setParam( this, nullptr );
    init( true, hostname, assignport );
}


void ODServiceBase::init( bool islocal, const char* hostname,
							    bool assignport )
{
    ODServiceBase* mainserv = theMain();
    if ( islocal )
    {
	odservbaselocservmgr_.setParam( this,
				mainserv ? mainserv->localServer() : nullptr );
    }
    else
	server_ = mainserv ? mainserv->server_ : nullptr;

    if ( server_ || localServer() )
    {
	serverismine_ = false;
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

    if ( portid>0 )
    {
	BufferString servernm( hostname );
	if ( islocal && servernm.isEmpty() )
	    servernm.set( "odservice:" ).add( GetPID() );
	const Network::Authority auth = islocal ?
			    Network::Authority::getLocal( servernm ) :
				    Network::Authority( nullptr, portid );

	startServer( auth );
    }

    if ( islocal )
	mAttachCB( localServer()->newConnection,
					    ODServiceBase::newConnectionCB );
    else
	mAttachCB( server_->newConnection, ODServiceBase::newConnectionCB );

    mAttachCB( IOM().surveyChanged, ODServiceBase::surveyChangedCB );
    mAttachCB( IOM().applicationClosing, ODServiceBase::appClosingCB );
    mAttachCB( OD::PythA().envChange, ODServiceBase::pyenvChangeCB );
    mAttachCB( this->externalAction, ODServiceBase::externalActionCB );
    mAttachCB( this->externalRequest, ODServiceBase::externalRequestCB );

}


ODServiceBase::~ODServiceBase()
{
    doAppClosing( nullptr );
    odservbaselocservmgr_.removeParam( this );
    odservbaselocalconnmgr_.removeAndDeleteParam( this );
}


Network::RequestServer* ODServiceBase::localServer() const
{
    return odservbaselocservmgr_.getParam( this );
}


Network::RequestServer* ODServiceBase::localServer()
{
    return odservbaselocservmgr_.getParam( this );
}


bool ODServiceBase::isOK() const
{
    return isOK( false );
}


bool ODServiceBase::isOK( bool islocal ) const
{
    return getAuthority( islocal ).hasAssignedPort();
}


Network::Authority ODServiceBase::getAuthority() const
{
    return getAuthority( false );
}


Network::Authority ODServiceBase::getAuthority( bool islocal ) const
{
    Network::RequestServer* server =
		islocal ? localServer() : server_;
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


void ODServiceBase::startServer( PortNr_Type portid )
{
    server_ = new Network::RequestServer( portid );
    if ( !server_ || !server_->isOK() )
    {
        pErrMsg( "startServer - failed" );
        stopServer();
        return;
    }

    if ( !theMain() )
        theMain( this );
}


void ODServiceBase::startServer( const Network::Authority& auth )
{
    if ( auth.isLocal() )
	odservbaselocservmgr_.setParam( this,
		new Network::RequestServer( auth.getServerName() ) );
    else
	server_ = new Network::RequestServer( auth, Network::Any );

    if ( (!localServer() || !localServer()->isOK() ) &&
	 (!server_ || !server_->isOK()) )
    {
	pErrMsg( "startServer - failed" );
	stopServer();
	return;
    }

    if ( !theMain() )
	theMain( this );
}


void ODServiceBase::stopServer()
{
    if ( serverismine_ )
    {
	deleteAndZeroPtr( server_ );
	odservbaselocservmgr_.deleteAndZeroPtrParam( this );
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
    //Threads::Locker lckr( lock_ );

    Network::RequestConnection* conn = localServer() ?
			localServer()->pickupNewConnection() : nullptr;
    if ( !conn )
	conn = server_->pickupNewConnection();

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
    odservbaselocalconnmgr_.deleteAndZeroPtrParam( this );

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


void ODServiceBase::doAppClosing( CallBacker* )
{
    detachAllNotifiers();
    stopServer();
}


ODServiceBase* ODServiceBase::theMain( ODServiceBase* newmain )
{
    mDefineStaticLocalObject( ODServiceBase*, mainservice, = nullptr )
    if ( newmain )
	mainservice = newmain;
    return mainservice;
}
