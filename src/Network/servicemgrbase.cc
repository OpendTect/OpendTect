/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "servicemgrbase.h"

#include "applicationdata.h"
#include "commandlineparser.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "od_ostream.h"
#include "pythonaccess.h"
#include "settings.h"


ServiceMgrBase* ServiceMgrBase::theNewMain( ServiceMgrBase* newmain )
{
    return theNewMain( true, newmain );
}


ServiceMgrBase* ServiceMgrBase::theNewMain( bool local, ServiceMgrBase* newmain)
{
    mDefineStaticLocalObject( ServiceMgrBase*, mainlocalservice, = nullptr )
    mDefineStaticLocalObject( ServiceMgrBase*, maintcpservice, = nullptr )
    if ( newmain )
    {
	if ( local )
	    mainlocalservice = newmain;
	else
	    maintcpservice = newmain;
    }

    return local ? mainlocalservice : maintcpservice;
}


const ServiceMgrBase* ServiceMgrBase::theMain()
{
    return theMain( true );
}


const ServiceMgrBase* ServiceMgrBase::theMain( bool local )
{
    return theNewMain( local, nullptr );
}


ObjectSet<ServiceMgrBase>& allServiceMgrs()
{
    mDefineStaticLocalObject( ObjectSet<ServiceMgrBase>, allservices, )
    return allservices;
}


// Network::ServiceMgrBase

ServiceMgrBase::ServiceMgrBase( const char* servicenm )
    : NamedCallBacker(servicenm)
    , startHandleAction(this)
    , startHandleRequest(this)
{
    addLocalServer();
}


ServiceMgrBase::ServiceMgrBase( const char* servicenm,
			      bool assignport, Network::SpecAddr spec )
    : NamedCallBacker(servicenm)
    , startHandleAction(this)
    , startHandleRequest(this)
{
    addTCPServer( assignport, spec );
}


bool ServiceMgrBase::addLocalServer()
{
    init( true );
    return isOK( true );
}


bool ServiceMgrBase::addTCPServer( bool assignport, Network::SpecAddr spec )
{
    init( false, assignport, spec );
    return isOK( false );
}


bool& ServiceMgrBase::serverIsMine( bool islocal )
{ return islocal ? localserverismine_ : tcpserverismine_; }


void ServiceMgrBase::init( bool islocal, bool assignport,Network::SpecAddr spec)
{
    allServiceMgrs().add( this );
    const ServiceMgrBase* mainserv = theMain( islocal );
    if ( islocal )
	localserver_ = mainserv ? mainserv->localserver_ : nullptr;
    else
	tcpserver_ = mainserv ? mainserv->tcpserver_ : nullptr;

    if ( isServerOK(islocal) )
    {
	serverIsMine(islocal) = false;
	return;
    }

    const char* skeyport = Network::Server::sKeyPort();
    PortNr_Type portid = 0;

    int defport = 0;
    Settings::common().get( skeyport, defport );

    int clport = 0;
    CommandLineParser clp;
    if ( !clp.hasKey(Network::Server::sKeyNoListen()) )
    {
	clp.setKeyHasValue( skeyport );
	if ( clp.hasKey(skeyport) )
	    clp.getVal( skeyport, clport );

	if ( clport > 0 && Network::isPortFree( (PortNr_Type) clport ) )
	    portid = (PortNr_Type) clport;
	else if ( assignport && defport>0
			 && Network::isPortFree((PortNr_Type) defport) )
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
	auto* server = islocal
	    ? new Network::RequestServer(
		    Network::Authority::getAppServerName("odservice").buf())
	    : new Network::RequestServer( portid, spec );
	useServer( server, islocal );
    }

    if ( !isServerOK(islocal) )
	return;

    if ( islocal )
	mAttachCB( localserver_->newConnection,
					    ServiceMgrBase::newConnectionCB );
    else
	mAttachCB( tcpserver_->newConnection, ServiceMgrBase::newConnectionCB );

    mAttachCB( OD::PythA().envChange, ServiceMgrBase::pyenvChangeCB );
    mAttachCB( IOM().surveyChanged, ServiceMgrBase::surveyChangedCB );
    mAttachCB( IOM().applicationClosing, ServiceMgrBase::appClosingCB );
}


ServiceMgrBase::~ServiceMgrBase()
{
    doAppClosing( nullptr );
}


bool ServiceMgrBase::isOK( bool islocal ) const
{
    return getAuthority( islocal  ).isOK();
}


Network::Authority ServiceMgrBase::getAuthority( bool islocal ) const
{
    Network::RequestServer* server =
		islocal ? localserver_ : tcpserver_;
    return server ? server->getAuthority() : Network::Authority();
}


bool ServiceMgrBase::isMainService() const
{
    if ( !tcpserver_ && !localserver_ )
	return false;

    return this == theMain( localserver_ );
}


void ServiceMgrBase::pyenvChangeCB( CallBacker* cb )
{ doPyEnvChange(cb); }


void ServiceMgrBase::surveyChangedCB( CallBacker* cb )
{ doSurveyChanged(cb); }


void ServiceMgrBase::appClosingCB( CallBacker* cb )
{ doAppClosing(cb); }


bool ServiceMgrBase::useServer( Network::RequestServer* server, bool islocal )
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

    if ( isServerOK(islocal) )
    {
        pFDebugMsg( DGB_SERVICES, BufferString( "Listening to: ",
            server->getAuthority().toString() ) );
    }
    else
    {
	pErrMsg( "startServer - failed" );
	stopServer( islocal );
	return false;
    }

    if ( !theMain(islocal) )
	theNewMain( islocal, this );

    return true;
}


bool ServiceMgrBase::isServerOK( bool islocal ) const
{
    if ( islocal )
	return localserver_ ? localserver_->isOK() : false;

    return tcpserver_ ? tcpserver_->isOK() : false;
}


void ServiceMgrBase::stopServer( bool islocal )
{
    if ( serverIsMine(islocal) )
    {
        if ( islocal )
        {
            if ( localserver_ )
                pFDebugMsg( DGB_SERVICES,
                    BufferString( "Stopping listening to: ",
                        getAuthority( true ).toString() ) );
	    deleteAndNullPtr( localserver_ );
        }
        else
        {
            if ( tcpserver_ )
                pFDebugMsg( DGB_SERVICES,
                    BufferString( "Stopping listening to: ",
                        getAuthority( false ).toString() ) );
	    deleteAndNullPtr( tcpserver_ );
        }
    }
}


void ServiceMgrBase::newConnectionCB( CallBacker* )
{
    Network::RequestConnection* conn = localserver_
				     ? localserver_->pickupNewConnection()
				     : tcpserver_->pickupNewConnection();
    if ( !conn || !conn->isOK() )
    {
	BufferString err( "newConnectionCB - connection error" );
    if ( conn )
        err.add( ": " ).add( conn->errMsg() );
    pFDebugMsg( DGB_SERVICES, err );
	return;
    }

    mAttachCB( conn->packetArrived, ServiceMgrBase::packetArrivedCB );
    mAttachCB( conn->connectionClosed, ServiceMgrBase::connClosedCB );
}


void ServiceMgrBase::packetArrivedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );
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
    pFDebugMsg( DGB_SERVICES, "Failed to get packet payload" );
	sendErr( uirv );
	return;
    }

    auto* packetdata = new packetData( tcpconn_ );
    applydata_.add( packetdata );
    const bool res = request.isPresent(sKeyAction())
		   ? canDoAction( request, *packetdata )
		   : canDoRequest( request, *packetdata );
    if ( res )
	sendOK();
    else
	sendErr( packetdata->msg_ );
}


void ServiceMgrBase::connClosedCB( CallBacker* cb )
{
    Network::RequestConnection* conn = (Network::RequestConnection*) cb;
    if ( !conn )
	return;

    mDetachCB( conn->packetArrived, ServiceMgrBase::packetArrivedCB );
    mDetachCB( conn->connectionClosed, ServiceMgrBase::connClosedCB );
    tcpconn_ = nullptr;
    localconn_ = nullptr;

    packetData* todopdata = nullptr;
    for ( auto* applydata : applydata_ )
    {
	if ( applydata->conn_ != conn )
	    continue;

	todopdata = applydata;
	break;
    }

    if ( !todopdata )
	return;

    applydata_ -= todopdata;
    if ( !todopdata->servicemgr_ )
    {
    pFDebugMsg( DGB_SERVICES,
        "Closing a connection without applying an action/request");
	if ( !todopdata->action_.isEmpty() )
        pFDebugMsg( DGB_SERVICES,
                BufferString("Action: ",todopdata->action_));
	if ( todopdata->request_ )
        pFDebugMsg( DGB_SERVICES,
                BufferString("Request: ",todopdata->request_->dumpJSon()));
	return;
    }

    const uiRetVal uirv = applyInOtherThread( *todopdata );
    if ( !uirv.isOK() )
    {
	pErrMsg(BufferString("Cannot apply: ",uirv.getText()));
    }
}


bool ServiceMgrBase::canDoAction( const OD::JSON::Object& actobj,
				 packetData& pdata )
{
    const BufferString action( actobj.getStringValue( sKeyAction()) );
    pFDebugMsg( DGB_SERVICES, BufferString("Received action: ",action.buf()));
    ObjectSet<ServiceMgrBase>& allservicemgrs = allServiceMgrs();
    uiRetVal& ret = pdata.msg_;
    for ( auto* servicemgr : allservicemgrs )
    {
	if ( servicemgr->canParseAction(action,ret) )
	{
	    pdata.servicemgr_ = servicemgr;
	    pdata.action_.set( action );
	    return ret.isOK();
	}
    }

    canParseAction( action, ret );
    ret.add( tr("Unknown action: %1").arg( action ) );
    return ret.isOK();
}


bool ServiceMgrBase::canDoRequest( const OD::JSON::Object& request,
				  packetData& pdata )
{
    pFDebugMsg( DGB_SERVICES,
            BufferString("Received request: ",request.dumpJSon()));
    ObjectSet<ServiceMgrBase>& allservicemgrs = allServiceMgrs();
    uiRetVal& ret = pdata.msg_;
    for ( auto* servicemgr : allservicemgrs )
    {
	if ( servicemgr->canParseRequest(request,ret) )
	{
	    pdata.servicemgr_ = servicemgr;
	    pdata.request_ = request.clone();
	    return ret.isOK();
	}
    }

    canParseRequest( request, ret );
    ret.add( tr("Unknown request received: %1").arg( request.dumpJSon() ) );
    return ret.isOK();
}


uiRetVal ServiceMgrBase::applyInOtherThread( const packetData& pdata )
{
    pdata.msg_.setOK();
/*    Threads::Thread thread(
	    mCB(applyservicemgr_,ServiceMgrBase,doHandleActionRequest),
		"Network service apply thread" );
    thread.waitForFinish();*/
    const uiRetVal uirv =
		pdata.servicemgr_->handleActionRequestInThread( pdata );
    delete &pdata;
    return uirv;
}


void ServiceMgrBase::doHandleActionRequest( CallBacker* )
{
//    actionreqapply_ = handleActionRequestInThread();
}


uiRetVal ServiceMgrBase::handleActionRequestInThread( const packetData& pdata )
{
    uiRetVal uirv;
    if ( !pdata.action_.isEmpty() )
	uirv = doHandleAction( pdata.action_ );
    else if ( pdata.request_ )
	uirv = doHandleRequest( *pdata.request_ );

    return uirv;
}


bool ServiceMgrBase::canParseAction( const char* action, uiRetVal& uirv )
{
    if ( StringView(action) == sKeyStatusEv() ||
	 StringView(action) == sKeyCloseEv() )
	return true;

    return false;
}


bool ServiceMgrBase::canParseRequest( const OD::JSON::Object& request,
				    uiRetVal& uirv )
{
    return request.isPresent( sKeySurveyChangeEv() ) ||
        request.isPresent( sKeyPyEnvChangeEv() );
}


uiRetVal ServiceMgrBase::doHandleAction( const char* action )
{
    if ( StringView(action) == sKeyCloseEv() )
	closeApp();
    return uiRetVal::OK();
}


uiRetVal ServiceMgrBase::doHandleRequest( const OD::JSON::Object& request )
{
    if ( request.isPresent(sKeySurveyChangeEv()) )
    {
        return survChangedReq( *request.getObject( sKeySurveyChangeEv() ) );
    }
    if ( request.isPresent(sKeyPyEnvChangeEv()) )
    {
        return pythEnvChangedReq( *request.getObject( sKeyPyEnvChangeEv() ) );
    }

    return uiRetVal::OK();
}


uiRetVal ServiceMgrBase::pythEnvChangedReq( const OD::JSON::Object& reqobj )
{
    if ( reqobj.isPresent(sKey::FileName()) && reqobj.isPresent(sKey::Name()) )
    {
	const OD::PythonAccess& pytha = OD::PythA();
	const OD::PythonSource src = pytha.getPythonSource();
	FilePath prevenvrootfp; BufferString prevvirtenvnm;
	if ( src != OD::System )
	{
	    prevenvrootfp = pytha.getPythonEnvFp();
	    prevvirtenvnm.set( pytha.getEnvName() );
	}

	const FilePath envrootfp = reqobj.getFilePath( sKey::FileName() );
	OD::PythonSource source =
			OD::PythonAccess::getPythonSource( &envrootfp );
	if ( src == OD::Internal && source == OD::Custom &&
	     prevenvrootfp == envrootfp )
	    source = OD::Internal;

	const BufferString virtenvnm( reqobj.getStringValue(sKey::Name() ) );
	if ( source != src || prevenvrootfp != envrootfp ||
	     prevvirtenvnm != virtenvnm )
	{
	    if ( source == OD::System )
		return getNonConst( pytha ).setEnvironment( nullptr, nullptr );
	    else
		return getNonConst( pytha ).setEnvironment( &envrootfp,
							    virtenvnm.buf() );
	}
    }

    return uiRetVal::OK();
}


void ServiceMgrBase::getPythEnvRequestInfo( OD::JSON::Object& sinfo )
{
    const OD::PythonAccess& pytha = OD::PythA();
    const OD::PythonSource src = pytha.getPythonSource();
    FilePath envrootfp; BufferString virtenvnm;
    if ( src != OD::System )
    {
	envrootfp = pytha.getPythonEnvFp();
	virtenvnm.set( pytha.getEnvName() );
    }

    sinfo.set( sKey::FileName(), envrootfp );
    sinfo.set( sKey::Name(), virtenvnm.buf() );
}


uiRetVal ServiceMgrBase::survChangedReq( const OD::JSON::Object& reqobj )
{
    const FilePath surveydir = reqobj.getFilePath( sKey::Survey() );
    const uiRetVal uirv = IOMan::setDataSource_( surveydir.fullPath() );
    return uirv;
}


void ServiceMgrBase::doAppClosing( CallBacker* )
{
    detachAllNotifiers();
    allServiceMgrs() -= this;
    stopServer( true );
    stopServer( false );
}


void ServiceMgrBase::closeApp()
{
    IOM().applClosing();
    ApplicationData::exit( 0 );
}


uiRetVal ServiceMgrBase::sendAction( const Network::Authority& auth,
				    const char* servicenm, const char* action )
{
    OD::JSON::Object request;
    request.set( sKeyAction(), action );
    return sendRequest_( auth, servicenm, request );
}


uiRetVal ServiceMgrBase::sendRequest( const Network::Authority& auth,
				     const char* servicenm, const char* reqkey,
				     const OD::JSON::Object& reqobj )
{
    OD::JSON::Object request;
    request.set( reqkey, reqobj.clone() );
    return sendRequest_( auth, servicenm, request );
}


uiRetVal ServiceMgrBase::sendRequest_( const Network::Authority& auth,
				      const char* servicenm,
				      const OD::JSON::Object& request )
{
    Network::RequestConnection conn( auth, false, 2000 );
    if ( !conn.isOK() )
    {
        const uiRetVal msg = tr("Cannot connect to service %1 on %2")
			    .arg( servicenm ).arg( auth.toString() );
        pFreeFDebugMsg( DGB_SERVICES, toString(msg) );
        return msg;
    }

    RefMan<Network::RequestPacket> packet = new Network::RequestPacket;
    packet->setPayload( request );
    packet->setIsNewRequest();
    if ( !conn.sendPacket(*packet) )
    {
	uiRetVal uirv = tr("Cannot send network packet from service %1 to '%2'")
				 .arg(servicenm).arg( auth.toString() );
	uirv.add( conn.errMsg() );
	uirv.add( tr("Packet content: %1").arg(request.dumpJSon()) );
    pFreeFDebugMsg( DGB_SERVICES, toString( uirv ) );
	return uirv;
    }

    ConstRefMan<Network::RequestPacket> receivedpacket =
		    conn.pickupPacket( packet->requestID(), 5000 );
    if ( !receivedpacket )
	return uiRetVal(tr("No response from: %1 for request:\n %2")
				    .arg(servicenm).arg(request.dumpJSon()));

    OD::JSON::Object response;
    uiRetVal uirv = receivedpacket->getPayload( response );
    if ( !uirv.isOK() )
    {
	uirv.add( tr("Response: %1").arg( response.dumpJSon() ) );
    pFreeFDebugMsg( DGB_SERVICES, toString( uirv ) );
	return uirv;
    }
    else if ( response.isPresent(sKeyError()) )
    {
	const uiRetVal msg( tr("%1 error: %2.\n%3").arg(servicenm)
	    .arg(response.getStringValue(sKeyError()))
	    .arg(response.dumpJSon()) );
    pFreeFDebugMsg( DGB_SERVICES, toString( msg ) );
    return msg;
    }

    return uiRetVal::OK();
}


void ServiceMgrBase::sendOK()
{
    OD::JSON::Object response;
    response.set( sKeyOK(), BufferString::empty() );
    pFDebugMsg( DGB_SERVICES,
            BufferString("Returning OK: ",response.dumpJSon()) );
    if ( packet_ )
	packet_->setPayload( response );
    if ( packet_ && tcpconn_ && !tcpconn_->sendPacket(*packet_.ptr()) )
	{ pErrMsg("sendOK - failed"); }
    packet_ = nullptr;
}


void ServiceMgrBase::sendErr( uiRetVal& uirv )
{
    OD::JSON::Object response;
    BufferString errmsg( uirv.getText() );
    errmsg.replace('\"','\'');
    response.set( sKeyError(), errmsg );
    if ( packet_ )
    {
	if ( !packet_->setPayload(response) )
	{
	    response.setEmpty();
	    response.set( sKeyError(), "Undetermined error" );
	    packet_->setPayload( response );
	}
    }
    if ( packet_ && tcpconn_ && !tcpconn_->sendPacket(*packet_.ptr()) )
	{ pErrMsg("sendErr - failed"); }
    packet_ = nullptr;
}


bool ServiceMgrBase::addApplicationAuthority( bool local,
					     OS::MachineCommand& mc )
{
    const ServiceMgrBase* mainserv = theMain( local );
    if ( !mainserv )
	return false;

    const Network::Authority auth = mainserv->getAuthority( local );
    if ( !auth.isOK() )
	return false;

    mc.addKeyedArg( sKeyODServer(), auth.toString() );

    return true;
}


// Network::ServiceMgrBase::packetData

ServiceMgrBase::packetData::packetData( const Network::RequestConnection* conn )
    : conn_(conn)
{
}


ServiceMgrBase::ServiceMgrBase::packetData::~packetData()
{
    delete request_;
}
