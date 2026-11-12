#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgmod.h"
#include "commondefs.h"

#ifdef min
# undef min
#endif
#ifdef max
# undef max
#endif

#include <vsg/app/WindowTraits.h>
#include <vsg/vk/Device.h>


/*! Shared Vulkan logical device for all uiVSG windows in this process.

    Sharing the Device avoids the VSG_MAX_DEVICES limit when multiple
    scenes/windows are open. How multiple Viewers/windows cooperate on
    that Device still needs a proper multi-view design (one Viewer with
    several windows, or per-view screen text state): see vsgGeo::Text
    setViewportHeight being process-global.
*/
mClass(uiVSG) uiVSGContext
{
public:
    static uiVSGContext&	instance();

    bool			hasDevice() const;
    vsg::ref_ptr<vsg::Device>	device() const;

    void			applyToTraits(vsg::WindowTraits&) const;
				/*!<If a device exists, set traits.device so
				    Window::create reuses it. */

    void			setDeviceFromWindow(vsg::Window&);
				/*!<Call after the first successful window
				    init. No-op if already set. */

private:
				uiVSGContext() = default;

    vsg::ref_ptr<vsg::Device>	device_;
};