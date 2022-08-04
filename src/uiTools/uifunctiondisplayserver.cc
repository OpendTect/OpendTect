/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		Nov 2021
________________________________________________________________________

-*/

#include "uifunctiondisplayserver.h"

#include "uifunctiondisplay.h"
#include "uiwindowfuncseldlg.h"
#include "uimsg.h"


mExternCPP(uiTools) uiFunctionDisplayServer&
GetFunctionDisplayServer( bool set, uiFunctionDisplayServer* fds )
{
    static PtrMan<uiFunctionDisplayServer> funcdispsvr_ =
						new uiODFunctionDisplayServer;
    if ( set && fds )
	funcdispsvr_ = fds;
    else if ( set && !fds && !funcdispsvr_->isBasic() )
	funcdispsvr_ = new uiODFunctionDisplayServer;

    return *funcdispsvr_.ptr();
}


uiFunctionDisplayServer::uiFunctionDisplayServer()
{}


uiFunctionDisplayServer::~uiFunctionDisplayServer()
{}


uiODFunctionDisplayServer::uiODFunctionDisplayServer()
    : uiFunctionDisplayServer()
{}


uiODFunctionDisplayServer::~uiODFunctionDisplayServer()
{}


uiFuncDispBase* uiODFunctionDisplayServer::createFunctionDisplay( uiParent* p,
					     const uiFuncDispBase::Setup& su )
{
    return new uiFunctionDisplay( p, su );
}


uiFuncDrawerBase* uiODFunctionDisplayServer::createFunctionDrawer( uiParent* p,
					     const uiFuncDrawerBase::Setup& su )
{
    return new uiFunctionDrawer( p, su );
}
