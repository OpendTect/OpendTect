/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodviewer2dserver.h"

#include "uiodviewer2d.h"

#include "ptrman.h"


uiODViewer2DServer& GetViewer2DServer( bool set, uiODViewer2DServer* server )
{
    mDefineStaticLocalObject( PtrMan<uiODViewer2DServer>, theserver,
			      = new uiODBasicViewer2DServer );
    if ( set && server && server != theserver.ptr() )
	theserver = server;
    else if ( set && !server && !theserver->isBasic2DViewer() )
	theserver = new uiODBasicViewer2DServer;

    return *theserver.ptr();
}


uiODViewer2DServer::uiODViewer2DServer()
{}


uiODViewer2DServer::~uiODViewer2DServer()
{}


// uiODBasicViewer2DServer

uiODBasicViewer2DServer::uiODBasicViewer2DServer()
{}


uiODBasicViewer2DServer::~uiODBasicViewer2DServer()
{}


uiODViewer2D* uiODBasicViewer2DServer::createViewer2D( uiODMain& appl,
						       const VisID& visid )
{
    return new uiODViewer2D( appl, visid );
}
