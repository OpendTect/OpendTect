/*!
 * @file
 * @brief Defines server interface and default implementation for 2D viewers.
 */

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
 * @class uiODViewer2DServer
 * @brief Creates the 2D viewers of uiODViewer2DMgr.
 *
 * uiODViewer2DMgr asks the current server for each new 2D viewer.
 * A plugin can make its own server current with GetViewer2DServer().
 */
mExpClass(uiODMain) uiODViewer2DServer
{
public:
    		uiODViewer2DServer();

    virtual 	~uiODViewer2DServer();

    /*!
     * @brief Creates a new 2D viewer.
     *
     * The arguments are the application main window and the ID of the 3D
     * visualization object the viewer is created from (VisID::udf() if none).
     * @return The new viewer, a uiODViewer2D or a subclass of it. The caller
     * (uiODViewer2DMgr) takes ownership. Should not be nullptr: in that case
     * uiODViewer2DMgr::addViewer2D() logs a programmer error (debug only)
     * and falls back to the native uiODViewer2D.
     * @note Pure virtual to be implemented by derived uiODBasicViewer2DServer
     * and uiDGBViewer2DServer.
     */
    virtual uiODViewer2D* createViewer2D(uiODMain&,const VisID&) = 0;

    /*!
     * @brief Checks if this server provides the basic 2D viewer.
     * @return True only for uiODBasicViewer2DServer; false otherwise.
     */
    inline virtual bool isBasic2DViewer() const { return false; }
}; // end class uiODViewer2DServer


/*!
 * @class uiODBasicViewer2DServer
 * @brief Default server that creates the native uiODViewer2D (free OpendTect).
 */
mExpClass(uiODMain) uiODBasicViewer2DServer : public uiODViewer2DServer
{
public:
	uiODBasicViewer2DServer();
	~uiODBasicViewer2DServer();

/*!
 * @brief Creates a native 2D viewer instance.
 * @return A new uiODViewer2D for the given application and VisID. The caller
 * takes ownership.
 */
uiODViewer2D* 	createViewer2D(uiODMain&, const VisID&) 	override;

/*!
 * @brief Identifies this server as the basic 2D viewer server.
 * @return Always returns true.
 */
inline bool 	isBasic2DViewer() const 	override	{ return true; }
}; //end class uiODBasicViewer2DServer


/*!
 * @brief Retrieves or sets the global 2D viewer server.
 *
 * Until a server is set, the current server is the default
 * uiODBasicViewer2DServer, created once on the first call.
 *
 * When @p set is true, this function makes @p server the current server
 * and takes ownership of it. With a nullptr @p server, the current server
 * is reset to a uiODBasicViewer2DServer. A replaced server is deleted, so
 * references to it obtained earlier become invalid.
 *
 * @param set If true, changes the current server; if false, only returns it.
 * @param server The new server, or nullptr to reset. Ignored (and not taken
 * over) when @p set is false.
 * @return Reference to the current server, after any change.
 */
mGlobal(uiODMain) uiODViewer2DServer& GetViewer2DServer(bool set=false,
					uiODViewer2DServer* server=nullptr);
