/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchserviceservermgr.h"

#include "batchprog.h"
#include "clientservicebase.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "odjson.h"



BatchServiceServerMgr& BatchServiceServerMgr::getMgr()
{
    mDefineStaticLocalObject(BatchServiceServerMgr,mgrInstance,);
    return mgrInstance;
}



BatchServiceServerMgr::BatchServiceServerMgr()
    : ServiceServerMgr(Network::Service::ODBatch,"odbatchservice")
    , bp_(BP())
{
    if ( isDependentApp() )
    {
	mAttachCB( bp_.startDoWork, BatchServiceServerMgr::workStartedCB );
	mAttachCB( bp_.pause, BatchServiceServerMgr::pausedCB );
	mAttachCB( bp_.resume, BatchServiceServerMgr::resumedCB );
	mAttachCB( bp_.killed , BatchServiceServerMgr::killedCB );
    }

    mAttachCB( bp_.endWork, BatchServiceServerMgr::workEnded );
    checkOnReportToApplication( true, 3000 );
}


BatchServiceServerMgr::~BatchServiceServerMgr()
{
    BatchServiceServerMgr::doAppClosing( nullptr );
}


bool BatchServiceServerMgr::isOK() const
{
    return ServiceMgrBase::isOK( true );
}


void BatchServiceServerMgr::workStartedCB( CallBacker* )
{
    checkOnReportToApplication( false );
    lastreport_ = sendActionRequest_( BatchServiceClientMgr::sKeyDoWork() );
}


void BatchServiceServerMgr::pausedCB( CallBacker* )
{
    lastreport_ = sendActionRequest_( BatchServiceClientMgr::sKeyPaused() );
}


void BatchServiceServerMgr::resumedCB( CallBacker* )
{
    lastreport_ = sendActionRequest_( BatchServiceClientMgr::sKeyResumed() );
}


void BatchServiceServerMgr::killedCB( CallBacker* cb )
{
    checkOnReportToApplication( false );
    lastreport_ = sendActionRequest_( BatchServiceClientMgr::sKeyKilled() );
    doAppClosing( cb );
}


void BatchServiceServerMgr::workEnded( CallBacker* cb )
{
    checkOnReportToApplication( false );
    lastreport_ = sendActionRequest_( BatchServiceClientMgr::sKeyFinished() );
    doAppClosing( cb );
}


void BatchServiceServerMgr::reportToCheckCB( CallBacker* )
{
    const BatchProgram::Status state = bp_.status_;
    if ( reportingAppIsAlive() )
    {
#ifdef __debug__
	OD::JSON::Object obj;
	obj.set( sKey::Status(), state );
	lastreport_ = sendActionRequest_( sKey::Status(), &obj );
#endif
	return;
    }

    if ( state == BatchProgram::WorkWait || state == BatchProgram::MoreToDo )
	closeApp();
}


uiRetVal BatchServiceServerMgr::sendActionRequest_( const char* action,
					     const OD::JSON::Object* morereq )
{
    return sendActionRequest( BatchServiceClientMgr::sKeyBatchRequest(),
			      action, morereq );
}


bool BatchServiceServerMgr::canParseAction( const char* action, uiRetVal& uirv )
{
    if ( StringView(action) == BatchServiceClientMgr::sKeyDoWork() ||
        StringView(action) == sKeyClientAppCloseEv() )
	return true;

    return ServiceServerMgr::canParseAction( action, uirv );
}


bool BatchServiceServerMgr::canParseRequest( const OD::JSON::Object& request,
					     uiRetVal& uirv )
{
    if ( request.isPresent(BatchServiceClientMgr::sKeyStoreInfo()) )
	return true;

    return ServiceServerMgr::canParseRequest( request, uirv );
}


uiRetVal BatchServiceServerMgr::doHandleAction( const char* action )
{
    if ( StringView(action) == BatchServiceClientMgr::sKeyDoWork() )
    {
	*bp_.strm_ << "Starting batch program from request" << od_endl;
	bp_.startDoWork.trigger();
    return uiRetVal::OK();
    }
    else if ( StringView(action) == sKeyClientAppCloseEv() )
    {
        setUnregistered();
        return uiRetVal::OK();
    }
    else if ( StringView(action) == sKeyCloseEv() )
	return uiRetVal::OK();

    return ServiceServerMgr::doHandleAction( action );
}


uiRetVal BatchServiceServerMgr::doHandleRequest(
					      const OD::JSON::Object& request )
{
    if ( request.isPresent(BatchServiceClientMgr::sKeyStoreInfo()) )
    {
	*bp_.strm_ << "Storing a batch program request" << od_endl;
	const OD::JSON::Object& obj =
		    *request.getObject(BatchServiceClientMgr::sKeyStoreInfo());
	bp_.requests_.add( obj.clone() );
    return uiRetVal::OK();
    }

    return ServiceServerMgr::doHandleRequest( request );
}


void BatchServiceServerMgr::doAppClosing( CallBacker* cb )
{
    detachAllNotifiers();
    // keep last:
    ServiceServerMgr::doAppClosing( cb );
}


void BatchServiceServerMgr::closeApp()
{
    detachAllNotifiers();
    bp_.endWorkCB( nullptr );
}
