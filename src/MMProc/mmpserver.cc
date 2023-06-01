/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmpserver.h"

#include "genc.h"
#include "ioman.h"
#include "mmpkeystr.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netservice.h"
#include "netsocket.h"
#include "oddirs.h"
#include "odjson.h"
#include "odplatform.h"
#include "odver.h"

using namespace Network;
using namespace MMPStr;


MMPServer::MMPServer( PortNr_Type port, int timeout )
    : NamedCallBacker(sMMPServer())
    , timeout_(timeout)
    , server_(*new RequestServer(port))
    , thisservice_(*new Service(server_.getAuthority()))
    , startJob(this)
    , logMsg(this)
    , dataRootChg(this)
    , getLogFile(this)
{
    if ( isOK() )
    {
	mAttachCB(server_.newConnection, MMPServer::newConnectionCB);
	thisservice_.setName( GetExecutableName() );
	thisservice_.setPID( GetPID() );
	thisservice_.setType( Service::Other );
	thisservice_.setViewOnly( true );
    }
    else
	errmsg_.add( server_.errMsg() );
}


MMPServer::~MMPServer()
{
    detachAllNotifiers();
    delete &thisservice_;
    delete &server_;
}


bool MMPServer::isOK() const
{
    return server_.isOK() && errmsg_.isOK();
}


void MMPServer::setLogFile( const char* newlog )
{
    thisservice_.setLogFile( newlog );
}


void MMPServer::newConnectionCB( CallBacker* )
{
    errmsg_.setEmpty();
    const auto* conn = server_.pickupNewConnection();
    if ( !conn )
    {
	errmsg_.add( tr("newConnectionCB - connection error") );
	return;
    }
    if ( !conn->isOK() )
    {
	errmsg_.add( conn->errMsg() );
	deleteAndNullPtr( conn );
	return;
    }

    mAttachCB(conn->packetArrived, MMPServer::packetArrivedCB);
    mAttachCB(conn->connectionClosed, MMPServer::connClosedCB);
}


void MMPServer::packetArrivedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );
    auto* conn = static_cast<RequestConnection*>(cber);
    if ( !conn )
    {
	pErrMsg("Um");
	errmsg_.add( tr("packetArrivedCB - connection error") );
	return;
    }

    packet_ = conn->pickupPacket( reqid, timeout_ );
    if ( !packet_ )
    {
	packet_ = conn->getNextExternalPacket();
	if ( !packet_ )
	{
	    errmsg_.add( tr("packetArrivedCB - no packet") );
	    return;
	}
    }

    OD::JSON::Object request;
    errmsg_ = packet_->getPayload( request );
    if ( !errmsg_.isOK() )
    {
	errmsg_.add( tr("Response: %1").arg( request.dumpJSon() ) );
	return;
    }
    tcpconn_ = conn;
    errmsg_.add( doHandleRequest(request) );
    return;
}


void MMPServer::connClosedCB( CallBacker* cb )
{
    mDynamicCastGet(RequestConnection*, conn, cb);
    if ( !conn )
    {
	pErrMsg("Um");
	return;
    }

    mDetachCB(conn->packetArrived, MMPServer::packetArrivedCB);
    mDetachCB(conn->connectionClosed, MMPServer::connClosedCB);
    deleteAndNullPtr(conn);
    if ( !errmsg_.isOK() )
	logMsg.trigger( errmsg_ );
}


void MMPServer::handleStatusRequest( const OD::JSON::Object& req )
{
    uiRetVal uirv;
    if ( !req.isPresent(sMMPClient()) )
    {
	uirv = tr("Unsupported message:/n%1").arg(req.dumpJSon());
	logMsg.trigger( uirv );
	return;
    }
    uirv = tr("Status request: %1").arg(req.getStringValue(sMMPClient()));
    logMsg.trigger( uirv );

    OD::JSON::Object servobj;
    OD::JSON::Object paramobj;

    thisservice_.fillJSON( servobj );
    paramobj.set( sMMPServer(), servobj.clone() );
    paramobj.set( sODVersion(), GetFullODVersion() );
    paramobj.set( sODPlatform(), OD::Platform::local().longName() );
    const FilePath drfp( GetBaseDataDir( ) );
    paramobj.set( sDataRoot(), drfp );
    uirv = sendResponse( sStatus(), paramobj );
    if ( !uirv.isOK() )
	logMsg.trigger( uirv );
}


extern "C" { mGlobal(Basic) void SetCurBaseDataDir(const char*); }

void MMPServer::handleSetDataRootRequest( const OD::JSON::Object& req )
{
    OD::JSON::Object paramobj;
    if ( req.isPresent(sDataRoot()) )
    {
	const FilePath drfp = req.getFilePath( sDataRoot() );
	const BufferString ddr( drfp.fullPath() );
	if ( IOMan::isValidDataRoot(ddr) )
	{
	    SetCurBaseDataDir( ddr );
	    //TODO: set to IOM() instead? Must have a survey as well then
	    dataRootChg.trigger();
	    paramobj.set( sOK(), "" );
	    const FilePath logfp( thisservice_.logFnm() );
	    paramobj.set( sLogFile(), logfp );
	}
	else
	    paramobj.set( sError(), "Not a valid data root" );
    }

    const uiRetVal uirv = sendResponse( sSetDataRoot(), paramobj );
    if ( !uirv.isOK() )
	logMsg.trigger( uirv );
}


uiRetVal MMPServer::doHandleRequest( const OD::JSON::Object& request )
{
    uiRetVal uirv;
    if ( request.isPresent(sStatus()) )
	handleStatusRequest( *request.getObject(sStatus()) );
    else if ( request.isPresent(sSetDataRoot()) )
	handleSetDataRootRequest( *request.getObject(sSetDataRoot()) );
    else if ( request.isPresent(sGetDataRoot()) )
    {
	OD::JSON::Object paramobj;
	const FilePath drfp( GetBaseDataDir( ) );
	paramobj.set( sDataRoot(), drfp );
	uirv = sendResponse( sGetDataRoot(), paramobj );
	if ( !uirv.isOK() )
	    logMsg.trigger( uirv );
    }
    else if ( request.isPresent(sCheckDataRoot()) )
    {
	const auto& reqobj = *request.getObject( sCheckDataRoot() );
	OD::JSON::Object paramobj;
	if ( reqobj.isPresent(sDataRoot()) )
	{
	    const FilePath ddrfp = reqobj.getFilePath(sDataRoot());
	    const BufferString ddr( ddrfp.fullPath() );
	    if ( IOMan::isValidDataRoot(ddr) )
		paramobj.set( sOK(), "" );
	    else
		paramobj.set( sError(), "Not a valid data root" );
	}

	uirv = sendResponse( sCheckDataRoot(), paramobj );
    }
    else if ( request.isPresent(sGetLogFile()) )
	getLogFile.trigger();
    else if ( request.isPresent(sStartJob()) )
	startJob.trigger( *request.getObject(sStartJob()) );
    else
	uirv.add( tr("Unsupported request: %1").arg(request.dumpJSon()) );

    if ( !uirv.isOK() )
	logMsg.trigger( uirv );

    return uirv;
}


uiRetVal MMPServer::sendResponse( const char* key, const OD::JSON::Object& obj )
{
    uiRetVal uirv;
    OD::JSON::Object response;
    response.set( key, obj.clone() );
    if ( packet_ )
    {
	packet_->setPayload( response );
	if ( tcpconn_ && !tcpconn_->sendPacket(*packet_.ptr()) )
		uirv.add( tcpconn_->errMsg() );

	packet_ = nullptr;
    }
    return uirv;
}
