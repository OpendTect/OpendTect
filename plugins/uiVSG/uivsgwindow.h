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
#include "timer.h"
#include "uieventfilter.h"
#include "uiobjbody.h"

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
class uiObject;
class uiParent;


/*!\brief The native Vulkan window of a VSG viewer, with the widget that
  holds it in the user interface. It makes the VSG viewer that renders in it.
  All the Qt code is in the source file. */

mClass(uiVSG) dGBVSGWindow : public uiObjectBody
{
public:
			dGBVSGWindow(uiParent*,uiObject& handle,
				     const char* nm, int rendertimems);
			~dGBVSGWindow();

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

    const mQtclass(QWidget*)	qwidget_() const override { return container_; }
    uiObject&		uiObjHandle() override		{ return handle_; }
    void		handleEventCB(CallBacker*);
    void		showTimerCB(CallBacker*);
    void		focusInCB(CallBacker*);

    uiObject&			handle_;

    vsg::ref_ptr<dGBVSGViewer>	viewer_;
    dGBVSGNativeWindow*		nativewin_ = nullptr;
    mQtclass(QWidget*)		container_ = nullptr;
    uiEventFilter		eventfilter_;
    uiEventFilter		focusfilter_;
    Timer			showtimer_;

};
