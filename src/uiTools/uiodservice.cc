/*+
 _ __________________________________________*_____________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Oct 2019
 ________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodservice.h"

#include "uimain.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uistatusbar.h"

#include "commandlineparser.h"
#include "ioman.h"
#include "filepath.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "netservice.h"
#include "pythonaccess.h"
#include "settings.h"

/*!\brief Base class for OpendTect Service Manager and external services/apps */

uiODServiceBase::uiODServiceBase( bool assignport )
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
	mAttachCB( server_->newConnection, uiODServiceBase::newConnectionCB );

    mAttachCB( IOM().surveyChanged, uiODServiceBase::surveyChangedCB );
    mAttachCB( IOM().applicationClosing, uiODServiceBase::appClosingCB );
    mAttachCB( OD::PythA().envChange, uiODServiceBase::pyenvChangeCB );
}


uiODServiceBase::~uiODServiceBase()
{
    detachAllNotifiers();
    stopServer();
}


bool uiODServiceBase::isOK() const
{
    return getAuthority().hasAssignedPort();
}


Network::Authority uiODServiceBase::getAuthority() const
{
    return server_ ? server_->getAuthority()
		   : Network::Authority();
}


void uiODServiceBase::surveyChangedCB( CallBacker* cb )
{ doSurveyChanged(cb); }


void uiODServiceBase::appClosingCB( CallBacker* cb )
{ doAppClosing(cb); }


void uiODServiceBase::pyenvChangeCB( CallBacker* cb )
{ doPyEnvChange(cb); }


void uiODServiceBase::startServer( PortNr_Type portid )
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


uiRetVal uiODServiceBase::doAction( const OD::JSON::Object& actobj )
{
    const BufferString action( actobj.getStringValue( sKeyAction()) );
    if ( action == sKeyStatusEv() )
	uiMain::theMain().topLevel()->statusBar()->message( tr("Status") );

    return uiRetVal::OK();
}


uiRetVal uiODServiceBase::doRequest( const OD::JSON::Object& request )
{
    if ( request.isPresent(sKeyPyEnvChangeEv()) )
	return pythEnvChangedReq( request );

    return uiRetVal( tr("Unknown JSON packet type: %1")
			.arg( request.dumpJSon() ) );
}


uiRetVal uiODServiceBase::doCloseAct()
{
    needclose_ = true;
    return uiRetVal::OK();
}


const OD::JSON::Object* uiODServiceBase::getSubObj(
					 const OD::JSON::Object& request,
					 const char* key )
{
    return request.isPresent(key) ? request.getObject( key ) : nullptr;
}


uiRetVal uiODServiceBase::survChangedAct( const OD::JSON::Object& request )
{
    const OD::JSON::Object* paramobj = uiODServiceBase::getSubObj( request,
							       sKey::Pars() );
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


uiRetVal uiODServiceBase::pythEnvChangedReq( const OD::JSON::Object& request )
{
    const OD::JSON::Object* paramobj = uiODServiceBase::getSubObj( request,
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


void uiODServiceBase::getPythEnvRequestInfo( OD::JSON::Object& sinfo )
{
    const OD::PythonAccess& pytha = OD::PythA();
    sinfo.set( sKey::FileName(), pytha.activatefp_
				? pytha.activatefp_->fullPath()
				: BufferString::empty() );
    sinfo.set( sKey::Name(), pytha.virtenvnm_ );
}


void uiODServiceBase::newConnectionCB( CallBacker* )
{
    Network::RequestConnection* conn = server_->pickupNewConnection();
    if ( !conn || !conn->isOK() )
    {
	BufferString err("newConnectionCB - connection error: ");
	err.add( conn->errMsg().getFullString() );
	pErrMsg(err);
	return;
    }

    mAttachCB( conn->packetArrived, uiODServiceBase::packetArrivedCB );
    mAttachCB( conn->connectionClosed, uiODServiceBase::connClosedCB );
}


void uiODServiceBase::packetArrivedCB( CallBacker* cb )
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


void uiODServiceBase::connClosedCB( CallBacker* cb )
{
    Network::RequestConnection* conn = (Network::RequestConnection*) cb;
    if ( !conn )
	return;
    mDetachCB( conn->packetArrived, uiODServiceBase::packetArrivedCB );
    mDetachCB( conn->connectionClosed, uiODServiceBase::connClosedCB );
    conn_ = nullptr;
    if ( needclose_ )
	uiMain::theMain().exit(0);
}


uiRetVal uiODServiceBase::sendAction( const Network::Authority& auth,
				      const char* servicenm,
				      const char* action,
				      const OD::JSON::Object* paramobj )
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
    if ( paramobj )
	request.set( sKey::Pars(), paramobj->clone() );

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


uiRetVal uiODServiceBase::sendRequest( const Network::Authority& auth,
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


void uiODServiceBase::sendOK()
{
    OD::JSON::Object response;
    response.set( sKeyOK(), BufferString::empty() );
    if ( packet_ )
	packet_->setPayload( response );
    if ( packet_ && conn_ && !conn_->sendPacket(*packet_.ptr()) )
	{ pErrMsg("sendOK - failed"); }
    packet_.release();
}


void uiODServiceBase::sendErr( uiRetVal& uirv )
{
    OD::JSON::Object response;
    response.set( sKeyError(), uirv.getText() );
    if ( packet_ )
	packet_->setPayload( response );
    if ( packet_ && conn_ && !conn_->sendPacket(*packet_.ptr()) )
	{ pErrMsg("sendErr - failed"); }
    packet_.release();
}



uiODService::uiODService( bool assignport )
   : uiODServiceBase(assignport)
{
    const CommandLineParser* clp = new CommandLineParser;
    const char* skeynolisten = Network::Server::sKeyNoListen();
    if ( !clp->hasKey(skeynolisten) )
    {
	if ( clp->hasKey(sKeyODServer()) )
	{
	    BufferString odserverstr;
	    if ( clp->getVal( sKeyODServer(), odserverstr ) )
		odauth_.fromString( odserverstr );
	}
    }
    delete clp;
    doRegister();
}


uiODService::~uiODService()
{
    doDeRegister();
}


bool uiODService::isODMainSlave() const
{
    return odauth_.hasAssignedPort();
}



uiRetVal uiODService::doAction( const OD::JSON::Object& actobj )
{
    const BufferString action( actobj.getStringValue( sKeyAction()) );

    if ( action == sKeyCloseEv() )
	return doCloseAct();
    else if ( action == sKeyRaiseEv() )
    {
	uiMainWin* mainwin = uiMain::theMain().topLevel();
	if ( mainwin->isMinimized() || mainwin->isHidden() )
	    mainwin->showNormal();
    }
    else if ( action == sKeySurveyChangeEv() )
	return survChangedAct( actobj );

    return uiODServiceBase::doAction( actobj );
}


uiRetVal uiODService::sendAction( const char* action,
				  const OD::JSON::Object* actobj ) const
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    const BufferString servicenm( "ODServiceMGr" );
    return uiODServiceBase::sendAction( odauth_, servicenm, action, actobj );
}


uiRetVal uiODService::close()
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    OD::JSON::Object request;
    request.set( sKeyAction(), sKeyCloseEv() );
    return doAction( request );
}


uiRetVal uiODService::doRegister()
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    OD::JSON::Object sinfo;
    Network::Service::fillJSON( getAuthority(), sinfo );
    uiRetVal uirv = uiODServiceBase::sendRequest( odauth_, "ODMain",
						  sKeyRegister(), sinfo );
    if ( !uirv.isOK() )
    {
	uirv.add( tr("Registration of service: %1 failed")
				.arg(Network::Service::getServiceName(sinfo)) );
	return uirv;
    }

    servid_ = Network::Service::getID( sinfo );

    return uiRetVal::OK();
}


uiRetVal uiODService::doDeRegister()
{
    if ( !isODMainSlave() )
	return uiRetVal::OK();

    OD::JSON::Object sinfo;
    Network::Service::fillJSON( getAuthority(), sinfo );
    uiRetVal uirv = uiODServiceBase::sendRequest( odauth_, "ODMain",
						  sKeyDeregister(), sinfo );
    if ( !uirv.isOK() )
    {
	uirv.add( tr("DeRegistration of service: %1 failed")
				.arg(Network::Service::getServiceName(sinfo)) );
	return uirv;
    }

    servid_ = 0;

    return uiRetVal::OK();
}


void uiODService::doPyEnvChange( CallBacker* )
{
    if ( !isODMainSlave() )
	return;

    OD::JSON::Object sinfo;
    uiODServiceBase::getPythEnvRequestInfo( sinfo );
    const uiRetVal uirv = uiODServiceBase::sendRequest( odauth_, "ODMain",
						sKeyPyEnvChangeEv(), sinfo );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}
