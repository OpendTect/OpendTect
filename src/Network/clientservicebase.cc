/*+
* ________________________________________________________________________
*
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* Author:	A. Huck
* Date:		Oct 2020
* ________________________________________________________________________
*
* -*/

#include "clientservicebase.h"

#include "filepath.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "odjson.h"


ServiceClientMgr::ServiceClientMgr( const char* servicenm )
    : ServiceMgrBase(servicenm)
    , serviceToBeAdded(this)
    , serviceToBeRemoved(this)
    , serviceAdded(this)
    , serviceRemoved(this)
{
    init( true );
}


ServiceClientMgr::ServiceClientMgr( const char* servicenm, bool assignport,
				    Network::SpecAddr addr )
    : ServiceMgrBase(servicenm,assignport,addr)
    , serviceToBeAdded(this)
    , serviceToBeRemoved(this)
    , serviceAdded(this)
    , serviceRemoved(this)
{
    init( false );
}


ServiceClientMgr::~ServiceClientMgr()
{
    ServiceClientMgr::doAppClosing( nullptr );
}


void ServiceClientMgr::init( bool islocal )
{
}


bool ServiceClientMgr::isPresent( const Network::Service::ID servid ) const
{
    return getService( servid );
}


bool ServiceClientMgr::isAlive( const Network::Service::ID servid ) const
{
    const Network::Service* serv = getService( servid );
    return serv && serv->isAlive();
}


BufferString ServiceClientMgr::serviceName( const Network::Service::ID servid )
									 const
{
    const Network::Service* service = getService( servid );
    const FilePath fp( service->name() );
    return service ? fp.baseName() : BufferString::empty();
}


Network::Service::SubID ServiceClientMgr::serviceSubID(
					const Network::Service::ID sid ) const
{
    const Network::Service* service = getService( sid );
    return service ? service->getSubID() : -1;
}


bool ServiceClientMgr::stopService( const Network::Service::ID servid )
{
    uiRetVal uirv;
    const Network::Service* service = getService( servid );
    if ( service )
    {
	if ( service->isAlive() )
	    uirv = sendAction( servid, sKeyCloseEv() );
    else
        removeService( servid );
	return uirv.isOK();
    }

    uirv = tr("Cannot stop unregistered service");

    return false;
}


uiRetVal ServiceClientMgr::sendAction( const Network::Service::ID servid,
				       const char* action ) const
{
    const Network::Service* service = getService( servid );
    if ( !service )
    {
    pFDebugMsg(DGB_SERVICES,"sending action to unregistered service");
	return uiRetVal( tr("Service with ID %1 not registered").arg( servid ));
    }

    return sendAction( *service, action );
}


uiRetVal ServiceClientMgr::sendRequest( const Network::Service::ID servid,
				   const char* reqkey,
				   const OD::JSON::Object& reqinfo ) const
{
    const Network::Service* service = getService( servid );
    if ( !service )
    {
    pFDebugMsg( DGB_SERVICES,"sending request to unregistered service");
	return uiRetVal( tr("Service with ID %1 not registered").arg( servid ));
    }

    return sendRequest( *service, reqkey, reqinfo );
}


void ServiceClientMgr::printInfo( const Network::Service::ID servid,
				  const char* desc, od_ostream* ostrm ) const
{
    const Network::Service* service = getService( servid );
    if ( service )
	service->printInfo( desc, ostrm );
}


bool ServiceClientMgr::addApplicationAuthority( OS::MachineCommand& mc )
{
    return ServiceMgrBase::addApplicationAuthority( true, mc );
}


bool ServiceClientMgr::canParseAction( const char* action, uiRetVal& uirv )
{
    return ServiceMgrBase::canParseAction( action, uirv );
}


bool ServiceClientMgr::canParseRequest( const OD::JSON::Object& request,
					uiRetVal& uirv )
{
    const bool isregrequest = request.isPresent( sKeyRegister() );
    const bool isderegrequest = request.isPresent( sKeyDeregister() );
    if ( isregrequest || isderegrequest )
    {
	const char* regkey = isregrequest ? sKeyRegister() : sKeyDeregister();
	Network::Service service( *request.getObject(regkey) );
	service.setViewOnly();
	return service.isOK() && canClaimService( service );
    }

    return ServiceMgrBase::canParseRequest( request, uirv );
}


uiRetVal ServiceClientMgr::doHandleAction( const char* action )
{
    return ServiceMgrBase::doHandleAction( action );
}


uiRetVal ServiceClientMgr::doHandleRequest( const OD::JSON::Object& request )
{
    if ( request.isPresent(sKeyRegister()) )
	return addService( *request.getObject(sKeyRegister()) );
    else  if ( request.isPresent(sKeyDeregister()) )
	return removeService( *request.getObject(sKeyDeregister()) );

    return ServiceMgrBase::doHandleRequest( request );
}


uiRetVal ServiceClientMgr::addService( const OD::JSON::Object& jsonobj )
{
    auto* service = new Network::Service( jsonobj );
    if ( !service->isOK() )
    {
	uiRetVal uirv = service->message();
	if ( uirv.isEmpty() )
	    uirv.add( tr("Service registration failed") );
    pFDebugMsg(DGB_SERVICES,"Service registration failed");
	service->setViewOnly();
	delete service;
	return uirv;
    }

    addService( *service );

    return uiRetVal::OK();
}


uiRetVal ServiceClientMgr::removeService( const OD::JSON::Object& jsonobj )
{
    Network::Service service( jsonobj );
    service.setViewOnly();
    if ( !service.isOK() )
    {
	uiRetVal ret = service.message();
    pFDebugMsg(DGB_SERVICES,"Service deregistration failed");
	return ret;
    }

    return removeService( service.getID() ) ? uiRetVal::OK()
	    : uiRetVal(tr("Deregistration error"));
}


void ServiceClientMgr::addService( Network::Service& service )
{
    cleanupServices();
    service.setViewOnly( false );
    serviceToBeAdded.trigger( &service );
    services_.add( &service );
    serviceAdded.trigger( service.getID() );
}


bool ServiceClientMgr::removeService( const Network::Service::ID servid )
{
    bool found = false;
    for ( int idx = services_.size() - 1; idx >= 0; idx-- )
    {
    Network::Service* service = services_.get( idx );
	if ( service->getID() != servid )
	    continue;

	serviceToBeRemoved.trigger( service );
	services_.removeSingle( idx );
	delete service;
	if ( found )
	    { pErrMsg( "Multiple services found for the same process"); }
	else
	    serviceRemoved.trigger( servid );
	found = true;
    }

    return found;
}


BufferString ServiceClientMgr::getLockFileFP(
					const Network::Service::ID id ) const
{
    BufferString ret;
    const Network::Service* serv = getService( id );
    if ( serv )
	ret = serv->lockFnm();

    return ret;
}


const Network::Service* ServiceClientMgr::getService(
				 const Network::Service::ID servid ) const
{
    return const_cast<ServiceClientMgr&>(*this).getService( servid );
}


Network::Service* ServiceClientMgr::getService(
				  const Network::Service::ID servid )
{
    for ( auto* service : services_ )
    {
	if ( service->getID() == servid )
	    return service;
    }

    return nullptr;
}


bool ServiceClientMgr::checkService( const Network::Service::ID servid ) const
{
    const Network::Service* service = getService( servid );
    if ( !service )
	return false;
    if ( !service->isAlive() )
    {
	const_cast<ServiceClientMgr*>( this )->removeService( servid );
	return false;
    }

    return true;
}


void ServiceClientMgr::cleanupServices()
{
    for ( auto* service : services_ )
    {
	if ( service->isAlive() )
	    continue;

	services_ -= service;
	serviceRemoved.trigger( service->PID() );
	delete service;
    }
}


uiRetVal ServiceClientMgr::sendAction( const Network::Service& service,
				       const char* action ) const
{
    BufferString msg("[CLIENT] Sending action: '",action,"' to: ");
    msg.add( service.getAuthority().toString() );
    pFDebugMsg( DGB_SERVICES, msg );
    const BufferString servicenm( "Service ", service.name() );
    const uiRetVal uirv = ServiceMgrBase::sendAction( service.getAuthority(),
						     servicenm, action );
    if ( !uirv.isOK() )
    {
	pErrMsg( BufferString("[CLIENT] Failed to send request: ",
		    uirv.getText()) );
    }

    return uirv;
}


uiRetVal ServiceClientMgr::sendRequest( const Network::Service& service,
					const char* reqkey,
					const OD::JSON::Object& reqobj ) const
{
    BufferString msg("[CLIENT] Sending request: '",reqkey,"' [");
    msg.add( reqobj.dumpJSon() ). add( "] to: " )
       .add( service.getAuthority().toString() );
    pFDebugMsg( DGB_SERVICES, msg );
    const BufferString servicenm( "Service ", service.name() );
    const uiRetVal uirv = ServiceMgrBase::sendRequest( service.getAuthority(),
						    servicenm, reqkey, reqobj );
    if ( !uirv.isOK() )
    {
	pErrMsg( BufferString("[CLIENT] Failed to send request: ",
		    uirv.getText()) );
    }

    return uirv;
}


void ServiceClientMgr::doAppClosing( CallBacker* cb )
{
    detachAllNotifiers();
    for ( int idx=services_.size()-1; idx>=0; idx-- )
    {
        Network::Service* service = services_.get( idx );
        sendAction( *service, sKeyClientAppCloseEv() );
        const Network::Service::ID servid = service->getID();
        stopService( servid );
        removeService( servid );
    }
    deepErase( services_ );

    ServiceMgrBase::doAppClosing( cb );
}


void ServiceClientMgr::closeApp()
{
    ServiceMgrBase::closeApp();
}



BatchServiceClientMgr& BatchServiceClientMgr::getMgr()
{
    mDefineStaticLocalObject(BatchServiceClientMgr,mgrInstance,);
    return mgrInstance;
}


BatchServiceClientMgr::BatchServiceClientMgr()
    : ServiceClientMgr("batchclient")
    , batchStarted(this)
    , batchEnded(this)
    , batchHasStarted(this)
    , batchPaused(this)
    , batchResumed(this)
    , batchKilled(this)
    , batchFinished(this)
{
    mAttachCB( serviceToBeAdded, BatchServiceClientMgr::batchServiceToBeAdded );
    mAttachCB( serviceAdded, BatchServiceClientMgr::batchServiceAdded );
    mAttachCB( serviceRemoved, BatchServiceClientMgr::batchServiceRemoved );
}


BatchServiceClientMgr::~BatchServiceClientMgr()
{
    BatchServiceClientMgr::doAppClosing( nullptr );
}


bool BatchServiceClientMgr::isOK() const
{
    return ServiceClientMgr::isOK( true );
}


bool BatchServiceClientMgr::canParseAction( const char* action, uiRetVal& uirv )
{
    return ServiceClientMgr::canParseAction( action, uirv );
}


bool BatchServiceClientMgr::canParseRequest( const OD::JSON::Object& request,
					     uiRetVal& uirv )
{
    if ( !request.isPresent(sKeyBatchRequest()) )
	return ServiceClientMgr::canParseRequest( request, uirv );

    uiString errstr = tr("Incorrect batch request: %1");

    const OD::JSON::Object* reqobj = request.getObject( sKeyBatchRequest() );
    if ( !reqobj || !reqobj->isPresent(sKey::Type())
		 || !reqobj->isPresent(sKey::ID()) )
	{ uirv = errstr.arg("Type/ID is missing"); return false; }

    const Network::Service::ID servid = mCast(Network::Service::ID,
					reqobj->getIntValue(sKey::ID()) );
    const Network::Service* service = getService( servid );
    if ( !service || !service->isOK() )
	{ uirv = errstr.arg("Cannot determine the service"); return false; }

    return true;
}


uiRetVal BatchServiceClientMgr::doHandleAction( const char* action )
{
    return ServiceClientMgr::doHandleAction( action );
}


uiRetVal BatchServiceClientMgr::doHandleRequest( const OD::JSON::Object&
								    request )
{
    if ( !request.isPresent(sKeyBatchRequest()) )
	return ServiceClientMgr::doHandleRequest( request );

    const OD::JSON::Object* reqobj = request.getObject( sKeyBatchRequest() );
    const BufferString reqtype = reqobj->getStringValue( sKey::Type() );
    const Network::Service::ID servid = mCast(Network::Service::ID,
					reqobj->getIntValue(sKey::ID()) );

    if ( reqtype == StringView(sKey::Status()) )
    {
	const BufferString status = reqobj->getStringValue( sKey::Status() );
	od_cout() << "Heartbeat [status]: " << status << od_endl;
    }
    else if ( reqtype == StringView(sKeyDoWork()) )
	batchHasStarted.trigger( servid );
    else if ( reqtype == StringView(sKeyPaused()) )
	batchPaused.trigger( servid );
    else if ( reqtype == StringView(sKeyResumed()) )
	batchResumed.trigger( servid );
    else if ( reqtype == StringView(sKeyKilled()) )
	batchKilled.trigger( servid );
    else if ( reqtype == StringView(sKeyFinished()) )
	batchFinished.trigger( servid );
    else
	od_cout() << "Heartbeat: " << reqtype << od_endl;

    return uiRetVal::OK();
}


bool BatchServiceClientMgr::canClaimService( const Network::Service& service
									) const
{
    return service.isBatch();
}


void BatchServiceClientMgr::batchServiceToBeAdded( CallBacker* cb )
{
    mCBCapsuleUnpack(Network::Service*,service,cb);
    if ( service )
	service->setViewOnly(); //Keep it alive even if this client app stops
}


void BatchServiceClientMgr::batchServiceAdded( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(Network::Service::ID,servid,cb);
    Network::Service* service = getService( servid );
    if ( service )
	batchStarted.trigger( servid );
}


void BatchServiceClientMgr::batchServiceRemoved( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(Network::Service::ID,servid,cb);
    batchEnded.trigger( servid );
}


void BatchServiceClientMgr::doAppClosing( CallBacker* )
{
    detachAllNotifiers();
    ServiceClientMgr::doAppClosing( nullptr );
}
