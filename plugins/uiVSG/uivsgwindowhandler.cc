/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgwindowhandler.h"

#include "visvsgsurvscene.h"
#include "vsggeotrackball.h"

#include <QGuiApplication>
#include <QWidget>
#include <QWindow>

#include <vsg/ui/KeyEvent.h>
#include <vsg/ui/PointerEvent.h>

uiVSGWindowHandler::uiVSGWindowHandler( QWindow* window, QWidget* container )
    : window_(window)
    , container_(container)
{}


void uiVSGWindowHandler::setTrackball( vsgGeo::Trackball* trackball )
{
    trackball_ = trackball;
}


void uiVSGWindowHandler::setScene( dGBVisSurvey::Scene* scene )
{
    scene_ = scene;
}


void uiVSGWindowHandler::setViewMode( bool yn )
{
    viewmode_ = yn;
    if ( scene_ )
	scene_->setInteractionEnabled( !yn );

    syncCursor();
    syncPointerPolicy();
}


void uiVSGWindowHandler::syncCursor()
{
    const bool ctrl = QGuiApplication::queryKeyboardModifiers() &
		      Qt::ControlModifier;
    const Qt::CursorShape shape = ctrl && !viewmode_ ? Qt::CrossCursor
				       : (viewmode_ ? Qt::PointingHandCursor
						    : Qt::ArrowCursor);
    if ( window_ )
	window_->setCursor( shape );
    if ( container_ )
	container_->setCursor( shape );
}


void uiVSGWindowHandler::syncPointerPolicy()
{
    const bool ctrl = QGuiApplication::queryKeyboardModifiers() &
		      Qt::ControlModifier;
    int mask = int(vsg::BUTTON_MASK_2) | int(vsg::BUTTON_MASK_3);
    if ( viewmode_ )
	mask |= int(vsg::BUTTON_MASK_1);

    if ( trackball_ )
    {
	trackball_->enableDragging( vsg::ButtonMask(mask) );
	trackball_->setLeftButtonPans( viewmode_ && ctrl );
    }

    const bool active = !viewmode_;
    if ( scene_ && (scene_->isPickable(true)!=active ||
		    !scene_->isPickable(false)) )
	scene_->setPickable( active, true );
}


void uiVSGWindowHandler::refresh()
{
    syncCursor();
    syncPointerPolicy();
}


void uiVSGWindowHandler::apply( vsg::ButtonPressEvent& )
{
    refresh();
}


void uiVSGWindowHandler::apply( vsg::ButtonReleaseEvent& )
{
    refresh();
}


void uiVSGWindowHandler::apply( vsg::MoveEvent& )
{
    refresh();
}


void uiVSGWindowHandler::apply( vsg::KeyPressEvent& ev )
{
    refresh();
    if ( ev.keyBase==vsg::KEY_Escape && ev.repeatCount==0 )
    {
	setViewMode( !viewmode_ );
	ev.handled = true;
    }
}


void uiVSGWindowHandler::apply( vsg::KeyReleaseEvent& )
{
    refresh();
}
