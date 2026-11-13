#pragma once
/*+
________________________________________________________________________

 Copyright: (C) 1995-2026 dGB Beheer B.V.
 License:   https://dgbes.com/licensing
________________________________________________________________________

 The window that owns the Vulkan surface for a VSG viewer. Adapted from
 vsgQt 0.5.0 (https://github.com/vsg-dev/vsgQt)
 -*/

#include "uivsgmod.h"
#include "mousecursor.h"

#include "vsgviewer.h"

#ifdef min
# undef min
#endif
#ifdef max
# undef max
#endif

#include <vsg/app/Window.h>

#include <functional>

class dGBVSGNativeWindow;
class dGBVSGWindowBody;
class uiObject;
class uiObjectBody;
class uiParent;


/*!\brief The native Vulkan window of a VSG viewer, with the widget that
  holds it in the user interface. It makes the VSG viewer that renders in it.
  All the Qt code is in the source file. */

mClass(uiVSG) dGBVSGWindow : public CallBacker
{
public:
			dGBVSGWindow(uiObject& handle,uiParent*,
				     const char* nm,int rendertimems);
			~dGBVSGWindow();

    uiObjectBody&	body();
    vsg::ref_ptr<dGBVSGViewer> vsgViewer() const	{ return viewer_; }
    vsg::ref_ptr<vsg::Window> vsgWindow() const;

    void		initializeWindow();
    void		setLeaveCallback(const std::function<void()>&);
    void		setCursor(MouseCursor::Shape);

    Notifier<dGBVSGWindow> shown;
			//!<Triggered after the widget was shown, when the
			//!<native window exists
    Notifier<dGBVSGWindow> resized;

private:

    vsg::ref_ptr<dGBVSGViewer>	viewer_;
    dGBVSGNativeWindow*		nativewin_ = nullptr;
    dGBVSGWindowBody*		body_ = nullptr;

};
