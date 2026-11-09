#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 View-mode half of the VSG window. Counterpart of the view-mode part
 of OD3DViewer: Esc, the cursor, which mouse buttons may move the
 camera, and whether the scene is actively pickable. In view mode,
 Ctrl+left pans like the middle button and does not pick.

-*/

#include "uivsgmod.h"
#include "callback.h"
#include "commondefs.h"

#include <vsg/core/Inherit.h>

class dGBVSGWindow;
namespace ODVsg { class Scene; }
namespace vsgGeo { class Trackball; }

mExpClass(uiVSG) uiVSGWindowHandler
			: public vsg::Inherit<vsg::Visitor,uiVSGWindowHandler>
{
public:
				uiVSGWindowHandler(dGBVSGWindow*);

    void			setTrackball(vsgGeo::Trackball*);
    void			setScene(ODVsg::Scene*);

    void			setViewMode(bool yn);
				/*!<True: left button orbits, Ctrl+left pans.
				    False: left button picks and drags. */
    bool			isViewMode() const	{ return viewmode_; }
    void			setViewModeToggledCB(const CallBack&);
				//!<Called when the Esc key switched the mode
    void			syncCursor();
    void			syncPointerPolicy();

    void			apply(vsg::ButtonPressEvent&) override;
    void			apply(vsg::ButtonReleaseEvent&) override;
    void			apply(vsg::MoveEvent&) override;
    void			apply(vsg::KeyPressEvent&) override;
    void			apply(vsg::KeyReleaseEvent&) override;

private:

    void			refresh();

    bool				viewmode_ = true;
    CallBack				viewmodetoggledcb_;
    dGBVSGWindow*			window_ = nullptr;
    vsg::ref_ptr<vsgGeo::Trackball>	trackball_;
    ODVsg::Scene*		scene_ = nullptr;
};
