/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgcontext.h"

#include <vsg/app/Window.h>



uiVSGContext& uiVSGContext::instance()
{
    static uiVSGContext ctx;
    return ctx;
}


bool uiVSGContext::hasDevice() const
{
    return device_.valid();
}


vsg::ref_ptr<vsg::Device> uiVSGContext::device() const
{
    return device_;
}


void uiVSGContext::applyToTraits( vsg::WindowTraits& traits ) const
{
    if ( device_ )
	traits.device = device_;
}


void uiVSGContext::setDeviceFromWindow( vsg::Window& window )
{
    ++windowCount_;
    if ( device_ )
	return;

    device_ = window.getOrCreateDevice();
}

void uiVSGContext::releaseWindow()
{
    if ( windowCount_ == 0 )
	return;

    --windowCount_;
    if ( windowCount_ > 0 )
	return;

    device_ = {};
}
