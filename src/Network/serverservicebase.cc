/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "serverservicebase.h"

#include "commandlineparser.h"
#include "keystrs.h"
#include "netserver.h"
#include "od_ostream.h"
#include "odjson.h"
#include "timer.h"


ServiceServerMgr::ServiceServerMgr( Network::Service::ServType typ,
				    const char* servicenm )
    : ServiceMgrBase(servicenm)
{
    init( typ, true );
}


ServiceServerMgr::ServiceServerMgr( Network::Service::ServType typ,
				    const char* servicenm,
				    bool assignport, Network::SpecAddr addr )
    : ServiceMgrBase(servicenm,assignport,addr)
{
    init( typ, false );
}


ServiceServerMgr::~ServiceServerMgr()
{
    ServiceServerMgr::doAppClosing( nullptr );
}


void ServiceServerMgr::init( Network::Service::ServType typ, bool islocal )
{
    if ( !isMainService() || !isOK(islocal) )
	return;

    delete thisservice_;
    thisservice_ = new Network::Service( getAuthority(islocal) );
    thisservice_->setType( typ );
    thisservice_->setViewOnly();

    delete reportto_;
    CommandLineParser clp;
    if ( !clp.hasKey(Network::Server::sKeyNoListen()) )
    {
	clp.setKeyHasValue( sKeyODServer() );
	if ( clp.hasKey(sKeyODServer()) )
	{
	    BufferString odserverstr;
	    clp.getVal( sKeyODServer(), odserverstr );
	    if ( islocal )
		reportto_ = new Network::Authority( odserverstr );
	    else
	    {
		reportto_ = new Network::Authority();
		reportto_->fromString( odserverstr );
	    }
	}
    }

    doRegister();
}


bool ServiceServerMgr::isRegistered() const
{
    return registerstatus_ == Requested || registerstatus_ == Confirmed;
}


bool ServiceServerMgr::canReceiveRequests() const
{
    return isDependentApp() && isRegistered();
}


bool ServiceServerMgr::isDependentApp() const
{
    return reportto_ && reportto_->isOK();
}


bool ServiceServerMgr::isStandAloneApp() const
{
    return !isDependentApp() || !reportingAppIsAlive();
}


uiRetVal ServiceServerMgr::sendAction( const char* action,
				       const Network::Authority* auth,
				       const char* servicenm ) const
{
    if ( !auth && !canReceiveRequests() )
	return uiRetVal::OK();

    BufferString msg("[SERVER] Sending action: '",action,"' to: ");
    msg.add( auth ? auth->toString() : reportto_->toString() );
    pFDebugMsg( DGB_SERVICES, msg );

    const uiRetVal uirv = ServiceMgrBase::sendAction( auth ? *auth : *reportto_,
			servicenm ? servicenm : name().buf(), action );
    if ( !uirv.isOK() )
	pErrMsg( BufferString("[SERVER] This action was not picked-up: ",
		    uirv.getText()) );
    return uirv;
}


uiRetVal ServiceServerMgr::sendActionRequest( const char* reqkey,
					      const char* action,
					      const OD::JSON::Object* morereq,
					      const Network::Authority* auth,
					      const char* servicenm ) const
{
    PtrMan<OD::JSON::Object> obj = morereq ? morereq->clone()
					   : new OD::JSON::Object;
    obj->set( sKey::Type(), action );
    obj->set( sKey::ID(), thisService()->getID() );
    return sendRequest( reqkey, *obj, auth, servicenm );
}


uiRetVal ServiceServerMgr::sendRequest( const char* reqkey,
					const OD::JSON::Object& reqobj,
					const Network::Authority* auth,
					const char* servicenm ) const
{
    if ( !auth && !canReceiveRequests() )
    {
	if ( StringView(reqkey) != sKeyRegister() )
	    return uiRetVal::OK();
    }

    BufferString msg("[SERVER] Sending request: '",reqkey,"' [");
    msg.add( reqobj.dumpJSon() ). add( "] to: " )
       .add( auth ? auth->toString() : reportto_->toString() );
    pFDebugMsg( DGB_SERVICES, msg );

    const uiRetVal uirv = ServiceMgrBase::sendRequest(auth ? *auth : *reportto_,
			servicenm ? servicenm : name().buf(), reqkey, reqobj );
    if ( !uirv.isOK() )
	pErrMsg( BufferString("[SERVER] This request was not picked-up: ",
		    uirv.getText()) );
    return uirv;
}


uiRetVal ServiceServerMgr::doRegister()
{ return doRegister_( sKeyRegister(), true ); }

uiRetVal ServiceServerMgr::doDeRegister()
{ return doRegister_( sKeyDeregister(), false ); }

uiRetVal ServiceServerMgr::doRegister_( const char* ky, bool doreg )
{
    if ( !thisservice_ || !isDependentApp() || (!doreg && !isRegistered()) )
//    if ( !thisservice_ || isStandAloneApp() )
	return uiRetVal::OK();

    OD::JSON::Object sinfo;
    thisservice_->fillJSON( sinfo );
    uiRetVal uirv = sendRequest( ky, sinfo );
    if ( uirv.isOK() )
	registerstatus_ = doreg ? Requested : Unrequested;
    else
    {
	uirv.add( tr("Registration of service: %1 failed")
				.arg(Network::Service::getServiceName(sinfo)) );
	return uirv;
    }

    return uiRetVal::OK();
}


void ServiceServerMgr::doPyEnvChange( CallBacker* )
{
    OD::JSON::Object sinfo;
    getPythEnvRequestInfo( sinfo );
    const uiRetVal uirv = sendRequest( sKeyPyEnvChangeEv(), sinfo );
    if ( !uirv.isOK() )
	OD::DisplayErrorMessage( uirv.getText() );
}


bool ServiceServerMgr::reportingAppIsAlive() const
{
    const uiRetVal uirv = sendAction( sKeyStatusEv() );
    return uirv.isOK();
}


void ServiceServerMgr::checkOnReportToApplication( bool start, int eachms )
{
    if ( !isDependentApp() )
//    if ( isStandAloneApp() )
	return;

    if ( start )
    {
	if ( reporttocheck_ )
	    mDetachCB( reporttocheck_->tick,
		       ServiceServerMgr::reportToCheckCB );
	else
	    reporttocheck_ = new Timer( "Reporting status check" );
	mAttachCB( reporttocheck_->tick, ServiceServerMgr::reportToCheckCB );
	reporttocheck_->start( eachms );
    }
    else
    {
	if ( reporttocheck_ )
	    reporttocheck_->stop();
    }
}


void ServiceServerMgr::reportToCheckCB( CallBacker* )
{
    if ( !reportingAppIsAlive() )
    {
	//Stop the listening server ?
	closeApp();
    }
}


void ServiceServerMgr::doAppClosing( CallBacker* cb )
{
    detachAllNotifiers();
    deleteAndNullPtr( reporttocheck_ );
    doDeRegister();
    deleteAndNullPtr( thisservice_ );
    deleteAndNullPtr( reportto_ );
    ServiceMgrBase::doAppClosing( cb );
}
