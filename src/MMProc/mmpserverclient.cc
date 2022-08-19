/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmpserverclient.h"

#include "genc.h"
#include "ioman.h"
#include "mmpkeystr.h"
#include "networkcommon.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netservice.h"
#include "netsocket.h"
#include "oddirs.h"
#include "odjson.h"
#include "odplatform.h"
#include "odver.h"
#include "systeminfo.h"

using namespace Network;
using namespace MMPStr;


MMPServerClient::MMPServerClient( const Authority& auth, int timeout )
    : NamedCallBacker(sMMPClient())
    , mmpservice_(*new Service(auth))
    , timeout_(timeout)
    , logFileChg(this)
    , errorNotice(this)
{
    mmpservice_.setPID( 0 );
    mmpservice_.setType( Service::Other );
    mmpservice_.setViewOnly( true );
    checkServer( mmpservice_.getAuthority() );
}


MMPServerClient::MMPServerClient( PortNr_Type port, const char* hostnm,
				  int timeout )
    :MMPServerClient(Authority(hostnm ? hostnm :
					Network::Socket::sKeyLocalHost(), port),
		     timeout)
{}


MMPServerClient::~MMPServerClient()
{
    delete &mmpservice_;
}


bool MMPServerClient::isOK() const
{
    return errMsg().isOK();
}


bool MMPServerClient::refresh()
{
    return checkServer( mmpservice_.getAuthority() );
}


bool MMPServerClient::validServerDataRoot( const char* dataroot )
{
    OD::JSON::Object reqobj;
    const FilePath drfp( dataroot );
    reqobj.set( sDataRoot(), drfp );
    OD::JSON::Object response = sendRequest( sCheckDataRoot(), reqobj );

    return errmsg_.isOK() && response.isPresent(sOK());
}


bool MMPServerClient::getServerDataRoot()
{
    OD::JSON::Object reqobj;
    OD::JSON::Object response = sendRequest( sGetDataRoot(), reqobj );
    if ( response.isPresent(sDataRoot()) )
	svr_drfp_ = response.getFilePath( sDataRoot() );

    return errmsg_.isOK();
}


bool MMPServerClient::setServerDataRoot( const char* dataroot )
{
    OD::JSON::Object reqobj;
    FilePath drfp( dataroot );
    reqobj.set( sDataRoot(), drfp );
    OD::JSON::Object response = sendRequest( sSetDataRoot(), reqobj );
    if ( response.isPresent(sOK()) )
    {
	const FilePath newlogfp = response.getFilePath( sLogFile() );
	const BufferString newlogfile( newlogfp.fullPath() );
	if ( !newlogfile.isEmpty() && newlogfile!=mmpservice_.logFnm() )
	{
	    mmpservice_.setLogFile( newlogfp );
	    logFileChg.trigger();
	}
    }

    return errmsg_.isOK() && response.isPresent(sOK());
}


void MMPServerClient::stopServer( bool removelog )
{
    mmpservice_.setViewOnly( false );
    mmpservice_.stop( removelog );
    mmpservice_.setViewOnly( true );
}


bool MMPServerClient::checkServer( const Authority& auth )
{
    errmsg_.setEmpty();
    OD::JSON::Object reqobj;
    BufferString id( GetExecutableName()," on ", System::localFullHostName() );
    reqobj.set(sMMPClient(), id );

    OD::JSON::Object status = sendRequest( auth, sStatus(), reqobj );
    if ( !errmsg_.isOK() )
    {
	errorNotice.trigger();
	return false;
    }

    if ( !status.isPresent(sMMPServer()) ||
	 !status.isPresent(sODVersion()) ||
	 !status.isPresent(sDataRoot()) ||
	 !status.isPresent(sODPlatform()) )
    {
	errmsg_ = tr("Unsupported message:\n%1").arg(status.dumpJSon());
	errorNotice.trigger();
	return false;
    }

    errmsg_ = mmpservice_.useJSON( *status.getObject(sMMPServer()) );
    mmpservice_.setAuthority( auth );
    mmpservice_.setViewOnly( true );

    if ( !errmsg_.isOK() )
    {
	errorNotice.trigger();
	return false;
    }

    svr_odver_ = status.getStringValue( sODVersion() );
    svr_drfp_ = status.getFilePath( sDataRoot() );
    BufferString plfname = status.getStringValue( sODPlatform() );
    if ( !OD::Platform::isValidName(plfname, false) )
    {
	errmsg_ = tr("Invalid platform: %1").arg(plfname);
	errorNotice.trigger();
	return false;
    }
    svr_platform_ = OD::Platform( plfname, false );

    return true;
}


OD::JSON::Object MMPServerClient::sendRequest( const char* reqkey,
					       const OD::JSON::Object& reqobj )
{
    const Authority& auth = mmpservice_.getAuthority();
    return sendRequest( auth, reqkey, reqobj );
}


OD::JSON::Object MMPServerClient::sendRequest( const Authority& auth,
					       const char* reqkey,
					       const OD::JSON::Object& reqobj )
{
    OD::JSON::Object response;
    errmsg_.setEmpty();
    const BufferString svrstr( sMMPServer(), " at ", auth.toString() );

    RequestConnection conn( auth, false, timeout_ );
    if ( !conn.isOK() )
    {
	errmsg_= tr("Cannot connect to %1").arg(svrstr);
	errmsg_.add( conn.errMsg() );
	errorNotice.trigger();
	return response;
    }

    RefMan<RequestPacket> packet = new RequestPacket;
    OD::JSON::Object request;
    request.set( reqkey, reqobj.clone() );

    packet->setPayload( request );
    packet->setIsNewRequest();
    if ( !conn.sendPacket(*packet) )
    {
	errmsg_ = tr("Cannot send packet to %1").arg(svrstr);
	errmsg_.add( conn.errMsg() );
	errmsg_.add( tr("Packet content: %1").arg(request.dumpJSon()) );
	errorNotice.trigger();
	return response;
    }

    ConstRefMan<RequestPacket> rcvpack = conn.pickupPacket( packet->requestID(),
							    timeout_ );
    if ( !rcvpack )
    {
	errmsg_ = tr("No response from %1").arg(svrstr);
	errorNotice.trigger();
	return response;
    }

    errmsg_ = rcvpack->getPayload( response );
    if ( !errmsg_.isOK() )
    {
	errmsg_.add( tr("Response: %1").arg( response.dumpJSon() ) );
	errorNotice.trigger();
	return response;
    }
    else if ( response.isPresent(sError()) )
    {
	errmsg_ =  tr("Error response from %1").arg(svrstr);
	errmsg_.add( tr("Response: %1").arg( response.dumpJSon() ) );
	errorNotice.trigger();
	return response;
    }

    return *response.getObject( reqkey );
}
