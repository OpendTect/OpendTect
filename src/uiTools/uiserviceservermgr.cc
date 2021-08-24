/*+
 _ __________________________________________*_____________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Oct 2019
 ________________________________________________________________________

 -*/

#include "uiserviceservermgr.h"

#include "uimain.h"
#include "uimainwin.h"
#include "uimsg.h"


uiServiceServerMgr::uiServiceServerMgr( const char* servicenm,
					uiMainWin& mainwin )
   : ServiceServerMgr(Network::Service::ODGui,servicenm)
{
    mAttachCB( mainwin.windowClosed, uiServiceServerMgr::doAppClosing );
}


uiServiceServerMgr::uiServiceServerMgr( const char* servicenm,
					uiMainWin& mainwin, bool assignport,
					Network::SpecAddr spec )
    : ServiceServerMgr(Network::Service::ODGui,servicenm,assignport,spec)
{
    mAttachCB( mainwin.windowClosed, uiServiceServerMgr::doAppClosing );
}


uiServiceServerMgr::~uiServiceServerMgr()
{
    uiServiceServerMgr::doAppClosing( nullptr );
}


bool uiServiceServerMgr::canParseAction( const char* action, uiRetVal& uirv )
{
    if ( FixedString(action) == sKeyRaiseEv() ||
        FixedString(action) == sKeyClientAppCloseEv() )
	return true;

    return ServiceServerMgr::canParseAction( action, uirv );
}


bool uiServiceServerMgr::canParseRequest( const OD::JSON::Object& request,
					  uiRetVal& uirv )
{
    return ServiceServerMgr::canParseRequest( request, uirv );
}


uiRetVal uiServiceServerMgr::doHandleAction( const char* action )
{
    if ( FixedString(action) == sKeyRaiseEv() )
    {
	uiMain::theMain().topLevel()->showAndActivate();
	return uiRetVal::OK();
    }
    else if ( FixedString(action) == sKeyClientAppCloseEv() )
    {
        setUnregistered();
        return uiRetVal::OK();
    }

    return ServiceServerMgr::doHandleAction( action );
}


uiRetVal uiServiceServerMgr::doHandleRequest( const OD::JSON::Object& request )
{
    return ServiceServerMgr::doHandleRequest( request );
}


bool uiServiceServerMgr::reportingAppIsAlive() const
{
    uiUserShowWait uisv( uiMain::theMain().topLevel(),
			 tr("Checking status of Main application") );
    return ServiceServerMgr::reportingAppIsAlive();
}


void uiServiceServerMgr::setBackground( bool yn )
{
    checkOnReportToApplication( yn );
}


void uiServiceServerMgr::doAppClosing( CallBacker* cb )
{
    detachAllNotifiers();
    ServiceServerMgr::doAppClosing( cb );
}


void uiServiceServerMgr::closeApp()
{
    uiMain::theMain().exit(0);
}
