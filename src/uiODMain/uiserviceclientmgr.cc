/*+
 _ __________________________________________*_____________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Oct 2019
 ________________________________________________________________________

 -*/

#include "uiserviceclientmgr.h"

#include "uimain.h"
#include "uimsg.h"
#include "uiodapplmgr.h"

#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odjson.h"
#include "od_ostream.h"
#include "welldata.h"
#include "wellman.h"

/*!\brief The OpendTect service manager */

using namespace Network;

uiServiceClientMgr& uiServiceClientMgr::getMgr()
{
    mDefineStaticLocalObject(uiServiceClientMgr,mgrInstance,);
    return mgrInstance;
}


uiServiceClientMgr::uiServiceClientMgr()
    : ServiceClientMgr("odmainservice")
{
}


uiServiceClientMgr::~uiServiceClientMgr()
{
    uiServiceClientMgr::doAppClosing( nullptr );
}


void uiServiceClientMgr::setFor( uiMainWin& win )
{
    if ( &win == uiMain::theMain().topLevel() )
	getMgr();
}


bool uiServiceClientMgr::isOK() const
{
    return ServiceClientMgr::isOK( true );
}


void uiServiceClientMgr::raise( const Network::Service::ID servid ) const
{
    if ( !checkService(servid) )
	return;

    const uiRetVal uirv = sendAction( servid,
				      uiServiceServerMgr::sKeyRaiseEv() );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}


bool uiServiceClientMgr::canParseAction( const char* action, uiRetVal& uirv )
{
    return ServiceClientMgr::canParseAction( action, uirv );
}


bool uiServiceClientMgr::canParseRequest( const OD::JSON::Object& request,
					  uiRetVal& uirv )
{
    if ( request.isPresent(uiServiceServerMgr::sKeyStart()) )
	return true;

    if ( request.isPresent(uiServiceServerMgr::sKeyLogsChanged()) )
	return true;

    return ServiceClientMgr::canParseRequest( request, uirv );
}


uiRetVal uiServiceClientMgr::doHandleAction( const char* action )
{
    return ServiceClientMgr::doHandleAction( action );
}


uiRetVal uiServiceClientMgr::doHandleRequest( const OD::JSON::Object& request )
{
    if ( request.isPresent(uiServiceServerMgr::sKeyStart()) )
	return startWorkflow(
			*request.getObject(uiServiceServerMgr::sKeyStart()) );
    else if ( request.isPresent(uiServiceServerMgr::sKeyLogsChanged()) )
	return logsChanged(
		    *request.getObject(uiServiceServerMgr::sKeyLogsChanged()) );

    return ServiceClientMgr::doHandleRequest( request );
}


uiRetVal uiServiceClientMgr::startWorkflow( const OD::JSON::Object& jsonobj )
{
    uiRetVal uirv;
    if ( !jsonobj.isPresent(sKey::Name()) )
    {
	uirv = tr("No application name to start");
	return uirv;
    }

    const BufferString appname( jsonobj.getStringValue( sKey::Name() ) );
    if ( appname==sKey::NN3D() || appname==sKey::NN2D() )
    {
	while ( true )
	{
	    if ( !ODMainWin()->applMgr().editNLA( appname==sKey::NN2D() ) )
		break;
	}
    }
    else if ( appname==sKey::UVQ3D() )
	ODMainWin()->applMgr().uvqNLA( false );
    else if ( appname==sKey::UVQ2D() )
	ODMainWin()->applMgr().uvqNLA( true );

    return uiRetVal::OK();
}


uiRetVal uiServiceClientMgr::logsChanged( const OD::JSON::Object& jsonobj )
{
    uiRetVal uirv;
    if ( !jsonobj.isPresent(sKey::ID()) )
    {
	uirv = tr("No well ID to reload");
	return uirv;
    }

    const MultiID wellid( jsonobj.getStringValue( sKey::ID() ) );
    if ( Well::MGR().validID(wellid) )
    {
	if ( Well::MGR().isLoaded(wellid) )
	    Well::MGR().reload( wellid, Well::LoadReqs(Well::LogInfos) );
    }
    else
    {
	uirv = tr("Invalid well ID");
	return uirv;
    }

	return uiRetVal::OK();
}


bool uiServiceClientMgr::canClaimService( const Network::Service& service
									) const
{
    return !service.isBatch();
}


void uiServiceClientMgr::doSurveyChanged( CallBacker* )
{
    OD::JSON::Object paramobj;
    paramobj.set( sKey::Survey(), FilePath(GetDataDir()) );
    for ( const auto service : services_ )
    {
	const uiRetVal uirv = sendRequest( *service,
					sKeySurveyChangeEv(), paramobj );
	if ( !uirv.isOK() )
	    uiMSG().error( uirv );
    }
}


void uiServiceClientMgr::doPyEnvChange( CallBacker* )
{
    OD::JSON::Object sinfo;
    getPythEnvRequestInfo( sinfo );
    for ( const auto* service : services_ )
    {
	const uiRetVal uirv = sendRequest(*service, sKeyPyEnvChangeEv(), sinfo);
	if ( !uirv.isOK() )
	    uiMSG().error( uirv );
    }
}


void uiServiceClientMgr::doAppClosing( CallBacker* cb )
{
    detachAllNotifiers();
    ServiceClientMgr::doAppClosing( cb );
}


void uiServiceClientMgr::closeApp()
{
    auto* mainwin = ODMainWin();
    if ( mainwin )
    {
	mainwin->exit( false, false );
	return;
    }

    uiMain::theMain().exit(0);
}
