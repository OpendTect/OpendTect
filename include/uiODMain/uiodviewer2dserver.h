#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "integerid.h"

class uiODMain;
class uiODViewer2D;


/*!
\brief Creates the 2D viewers of uiODViewer2DMgr.

uiODViewer2DMgr asks the current server for each new 2D viewer. A plugin can
make its own server current with GetViewer2DServer().
*/

mExpClass(uiODMain) uiODViewer2DServer
{
public:

    uiODViewer2DServer();
    virtual 	~uiODViewer2DServer();

    //! Pure virtual to be implemented by derived classes uiODBasicViewer2DServer and uiDGBViewer2DServer. Creates a new viewer with the given application and VisID.
    virtual uiODViewer2D*	createViewer2D(uiODMain&,const VisID&) = 0;

    //! True only for uiODBasicViewer2DServer
    virtual bool		isBasic2DViewer() const		{ return false; }
};


/*!
\brief Default server: creates the native uiODViewer2D.
*/

mExpClass(uiODMain) uiODBasicViewer2DServer : public uiODViewer2DServer
{
public:
uiODBasicViewer2DServer();
~uiODBasicViewer2DServer();

uiODViewer2D*		createViewer2D(uiODMain&,const VisID&) override;
bool			isBasic2DViewer() const override	{ return true; }
};


//! Returns the current server. With set, makes server current and takes it
//! over; a nullptr server makes the basic server current.
mGlobal(uiODMain) uiODViewer2DServer& GetViewer2DServer(bool set=false,
					uiODViewer2DServer* server=nullptr);
