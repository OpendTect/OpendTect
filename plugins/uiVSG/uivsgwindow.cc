/*+
________________________________________________________________________

 Copyright: (C) 1995-2026 dGB Beheer B.V.
 License:   https://dgbes.com/licensing
________________________________________________________________________

 Adapted from vsgQt 0.5.0 (https://github.com/vsg-dev/vsgQt).
 -*/

#if defined(_WIN32)
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
#endif

#if defined(WIN32)
# define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
# define VK_USE_PLATFORM_MACOS_MVK
#else
# define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vsg/all.h>

#include "uivsgwindow.h"

#include <cmath>
#include <map>

#include <QExposeEvent>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPlatformSurfaceEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QWheelEvent>


namespace
{

class QtKeyboardMap
{
public:
		QtKeyboardMap();

    bool	getKeySymbol(const QKeyEvent&,vsg::KeySymbol& keysymbol,
			     vsg::KeySymbol& modifiedkeysymbol,
			     vsg::KeyModifier& keymodifier) const;

private:

    std::map<uint32_t,vsg::KeySymbol>	keymap_;

};


QtKeyboardMap::QtKeyboardMap()
    : keymap_{
    { 0x0, vsg::KEY_Undefined },
    { Qt::Key_Space, vsg::KEY_Space },
    { Qt::Key_Home, vsg::KEY_Home },
    { Qt::Key_Left, vsg::KEY_Left },
    { Qt::Key_Up, vsg::KEY_Up },
    { Qt::Key_Right, vsg::KEY_Right },
    { Qt::Key_Down, vsg::KEY_Down },
    { Qt::Key_PageUp, vsg::KEY_Page_Up },
    { Qt::Key_Home, vsg::KEY_Next },
    { Qt::Key_PageDown, vsg::KEY_Page_Down },
    { Qt::Key_End, vsg::KEY_End },
    { '!', vsg::KEY_Exclaim },
    { '"', vsg::KEY_Quotedbl },
    { '#', vsg::KEY_Hash },
    { '$', vsg::KEY_Dollar },
    { '&', vsg::KEY_Ampersand },
    { Qt::Key_QuoteLeft, vsg::KEY_Quote },
    { '(', vsg::KEY_Leftparen },
    { ')', vsg::KEY_Rightparen },
    { '*', vsg::KEY_Asterisk },
    { '+', vsg::KEY_Plus },
    { Qt::Key_Comma, vsg::KEY_Comma },
    { Qt::Key_Minus, vsg::KEY_Minus },
    { Qt::Key_Period, vsg::KEY_Period },
    { Qt::Key_Slash, vsg::KEY_Slash },
    { ':', vsg::KEY_Colon },
    { Qt::Key_Semicolon, vsg::KEY_Semicolon },
    { '<', vsg::KEY_Less },
    { Qt::Key_Equal, vsg::KEY_Equals },
    { '>', vsg::KEY_Greater },
    { '?', vsg::KEY_Question },
    { '@', vsg::KEY_At },
    { Qt::Key_BracketLeft, vsg::KEY_Leftbracket },
    { Qt::Key_Backslash, vsg::KEY_Backslash },
    { Qt::Key_BracketRight, vsg::KEY_Rightbracket },
    { '|', vsg::KEY_Caret },
    { '_', vsg::KEY_Underscore },
    { 0xc0, vsg::KEY_Backquote },
    { Qt::Key_Back, vsg::KEY_BackSpace },
    { Qt::Key_Tab, vsg::KEY_Tab },
    { Qt::Key_Backtab, vsg::KEY_Tab },
    { Qt::Key_Clear, vsg::KEY_Clear },
    { Qt::Key_Return, vsg::KEY_Return },
    { Qt::Key_Pause, vsg::KEY_Pause },
    { Qt::Key_ScrollLock, vsg::KEY_Scroll_Lock },
    { Qt::Key_Escape, vsg::KEY_Escape },
    { Qt::Key_Delete, vsg::KEY_Delete },
    { Qt::Key_Select, vsg::KEY_Select },
    { Qt::Key_Print, vsg::KEY_Print },
    { Qt::Key_Execute, vsg::KEY_Execute },
    { Qt::Key_Insert, vsg::KEY_Insert },
    { Qt::Key_Menu, vsg::KEY_Menu },
    { Qt::Key_Cancel, vsg::KEY_Cancel },
    { Qt::Key_Help, vsg::KEY_Help },
    { Qt::Key_NumLock, vsg::KEY_Num_Lock },
    { Qt::Key_F1, vsg::KEY_F1 },
    { Qt::Key_F2, vsg::KEY_F2 },
    { Qt::Key_F3, vsg::KEY_F3 },
    { Qt::Key_F4, vsg::KEY_F4 },
    { Qt::Key_F5, vsg::KEY_F5 },
    { Qt::Key_F6, vsg::KEY_F6 },
    { Qt::Key_F7, vsg::KEY_F7 },
    { Qt::Key_F8, vsg::KEY_F8 },
    { Qt::Key_F9, vsg::KEY_F9 },
    { Qt::Key_F10, vsg::KEY_F10 },
    { Qt::Key_F11, vsg::KEY_F11 },
    { Qt::Key_F12, vsg::KEY_F12 },
    { Qt::Key_F13, vsg::KEY_F13 },
    { Qt::Key_F14, vsg::KEY_F14 },
    { Qt::Key_F15, vsg::KEY_F15 },
    { Qt::Key_F16, vsg::KEY_F16 },
    { Qt::Key_F17, vsg::KEY_F17 },
    { Qt::Key_F18, vsg::KEY_F18 },
    { Qt::Key_F19, vsg::KEY_F19 },
    { Qt::Key_F20, vsg::KEY_F20 },
    { Qt::Key_F21, vsg::KEY_F21 },
    { Qt::Key_F22, vsg::KEY_F22 },
    { Qt::Key_F23, vsg::KEY_F23 },
    { Qt::Key_F24, vsg::KEY_F24 },
    { Qt::Key_Shift, vsg::KEY_Shift_L },
    { Qt::Key_Control, vsg::KEY_Control_R },
    { Qt::Key_CapsLock, vsg::KEY_Caps_Lock }
    }
{
}


bool QtKeyboardMap::getKeySymbol( const QKeyEvent& event,
    vsg::KeySymbol& keysymbol,
    vsg::KeySymbol& modifiedkeysymbol,
    vsg::KeyModifier& keymodifier ) const
{
    uint16_t modifiermask = 0;
    if ( event.modifiers() & Qt::ShiftModifier )
	modifiermask |= vsg::KeyModifier::MODKEY_Shift;

    if ( event.modifiers() & Qt::ControlModifier )
	modifiermask |= vsg::KeyModifier::MODKEY_Control;

    if ( event.modifiers() & Qt::AltModifier )
	modifiermask |= vsg::KeyModifier::MODKEY_Alt;

    if ( event.modifiers() & Qt::MetaModifier )
	modifiermask |= vsg::KeyModifier::MODKEY_Meta;

    keymodifier = static_cast<vsg::KeyModifier>( modifiermask );

    const auto it = keymap_.find( static_cast<uint32_t>(event.key()) );
    if ( it != keymap_.end() )
    {
	keysymbol = it->second;
	modifiedkeysymbol = keysymbol;
	return true;
    }

    keysymbol = static_cast<vsg::KeySymbol>( event.key() );
    if ( keysymbol>='A' && keysymbol<='Z' )
	keysymbol = static_cast<vsg::KeySymbol>(
					int(keysymbol) + int('a'-'A') );

    modifiedkeysymbol = static_cast<vsg::KeySymbol>(
					*(event.text().toLatin1().data()) );
    return true;
}


QtKeyboardMap& keyboardMap()
{
    static QtKeyboardMap map;
    return map;
}


std::pair<vsg::ButtonMask,uint32_t> mouseButtons( const QMouseEvent& event )
{
    uint16_t mask = 0;
    uint32_t button = 0;
    if ( event.buttons() & Qt::LeftButton )
	mask = mask | vsg::BUTTON_MASK_1;

    if ( event.buttons() & Qt::MiddleButton )
	mask = mask | vsg::BUTTON_MASK_2;

    if ( event.buttons() & Qt::RightButton )
	mask = mask | vsg::BUTTON_MASK_3;

    switch ( event.button() )
    {
	case Qt::LeftButton:
	    button = 1;
	    break;
	case Qt::MiddleButton:
	    button = 2;
	    break;
	case Qt::RightButton:
	    button = 3;
	    break;
	default:
	    break;
    }

    return { static_cast<vsg::ButtonMask>(mask), button };
}

} // namespace


dGBVSGWindow::dGBVSGWindow( vsg::ref_ptr<dGBVSGViewer> viewer,
    vsg::ref_ptr<vsg::WindowTraits> traits,
    QScreen* targetscreen )
    : QWindow(targetscreen)
    , viewer_(viewer)
{
    if ( traits )
    {
	traits_ = vsg::WindowTraits::create( *traits );
	setGeometry( traits_->x, traits_->y,
		 traits_->width, traits_->height );
    }
    else
    {
	traits_ = vsg::WindowTraits::create();
	traits_->x = x();
	traits_->y = y();
	traits_->width = width();
	traits_->height = height();
    }
}


dGBVSGWindow::~dGBVSGWindow()
{
    cleanup();
}


void dGBVSGWindow::setLeaveCallback( const std::function<void()>& cb )
{
    leavecb_ = cb;
}


int dGBVSGWindow::convertCoord( double coord ) const
{
    return static_cast<int>( std::round(static_cast<qreal>(coord) *
	devicePixelRatio()) );
}


void dGBVSGWindow::initializeWindow()
{
    if ( vsgwindow_ )
	return;

    if ( !traits_ )
	traits_ = vsg::WindowTraits::create();

    #if defined(VK_USE_PLATFORM_WIN32_KHR)
    traits_->nativeWindow = reinterpret_cast<HWND>( winId() );
    #elif defined(VK_USE_PLATFORM_XLIB_KHR)
    traits_->nativeWindow = static_cast<::Window>( winId() );
    #elif defined(VK_USE_PLATFORM_XCB_KHR)
    traits_->nativeWindow = static_cast<xcb_window_t>( winId() );
    #elif defined(VK_USE_PLATFORM_MACOS_MVK)
    traits_->nativeWindow = winId();
    #endif

    traits_->x = convertCoord( x() );
    traits_->y = convertCoord( y() );
    traits_->width = convertCoord( width() );
    traits_->height = convertCoord( height() );
    vsgwindow_ = vsg::Window::create( traits_ );
    initialized_ = true;
}


void dGBVSGWindow::cleanup()
{
    if ( vsgwindow_ )
    {
	if ( viewer_ )
	{
	    viewer_->deviceWaitIdle();
	    viewer_->removeWindow( vsgwindow_ );
	}

	vsgwindow_->releaseWindow();
    }

    vsgwindow_ = {};
    viewer_ = {};
}


bool dGBVSGWindow::event( QEvent* event )
{
    if ( event->type() == QEvent::PlatformSurface )
    {
	auto* surfaceevent
		= dynamic_cast<QPlatformSurfaceEvent*>( event );
	if ( surfaceevent && vsgwindow_ &&
	    surfaceevent->surfaceEventType() ==
	    QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed )
	{
	    const vsg::clock::time_point eventtime
			    = vsg::clock::now();
	    vsgwindow_->bufferedEvents.push_back(
		    vsg::CloseWindowEvent::create(vsgwindow_,
				  eventtime) );
	    cleanup();
	}
    }

    if ( event->type()==QEvent::Leave && leavecb_ )
	leavecb_();

    return QWindow::event( event );
}


void dGBVSGWindow::exposeEvent( QExposeEvent* )
{
    if ( !initialized_ && isExposed() )
	initializeWindow();

    if ( viewer_ )
	viewer_->request();
}


void dGBVSGWindow::hideEvent( QHideEvent* )
{
}


void dGBVSGWindow::resizeEvent( QResizeEvent* )
{
    if ( !vsgwindow_ )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    vsgwindow_->bufferedEvents.push_back( vsg::ConfigureWindowEvent::create(
		    vsgwindow_, eventtime, convertCoord(x()), convertCoord(y()),
		    convertCoord(width()), convertCoord(height())) );
    vsgwindow_->resize();
    if ( viewer_ )
	viewer_->request();
}


void dGBVSGWindow::postKey( const QKeyEvent& event, bool press )
{
    if ( !vsgwindow_ )
	return;

    vsg::KeySymbol keysymbol;
    vsg::KeySymbol modifiedkeysymbol;
    vsg::KeyModifier keymodifier;
    if ( !keyboardMap().getKeySymbol(event,keysymbol,
		     modifiedkeysymbol,keymodifier) )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    if ( press )
	vsgwindow_->bufferedEvents.push_back( vsg::KeyPressEvent::create(
					      vsgwindow_,eventtime,keysymbol,
					      modifiedkeysymbol,keymodifier) );
    else
	vsgwindow_->bufferedEvents.push_back( vsg::KeyReleaseEvent::create(
					      vsgwindow_,eventtime,keysymbol,
					      modifiedkeysymbol,keymodifier) );

    if ( viewer_ )
	viewer_->request();
}


void dGBVSGWindow::keyPressEvent( QKeyEvent* event )
{
    if ( event )
	postKey( *event, true );
}


void dGBVSGWindow::keyReleaseEvent( QKeyEvent* event )
{
    if ( event )
	postKey( *event, false );
}


std::pair<int,int> dGBVSGWindow::mousePosition( const QMouseEvent& event ) const
{
    return { convertCoord(event.position().x()),
	     convertCoord(event.position().y()) };
}


void dGBVSGWindow::mouseMoveEvent( QMouseEvent* event )
{
    if ( !vsgwindow_ || !event )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    const auto mask = mouseButtons( *event ).first;
    const auto [xpos, ypos] = mousePosition( *event );
    vsgwindow_->bufferedEvents.push_back(
		vsg::MoveEvent::create(vsgwindow_,eventtime,xpos,ypos,mask) );
    if ( viewer_ )
	viewer_->request();
}


void dGBVSGWindow::mousePressEvent( QMouseEvent* event )
{
    if ( !vsgwindow_ || !event )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    const auto [mask, button] = mouseButtons( *event );
    const auto [xpos, ypos] = mousePosition( *event );
    vsgwindow_->bufferedEvents.push_back(
			vsg::ButtonPressEvent::create(vsgwindow_,eventtime,xpos,
						      ypos,mask,button) );
    if ( viewer_ )
	viewer_->request();
}


void dGBVSGWindow::mouseReleaseEvent( QMouseEvent* event )
{
    if ( !vsgwindow_ || !event )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    const auto [mask, button] = mouseButtons( *event );
    const auto [xpos, ypos] = mousePosition( *event );
    vsgwindow_->bufferedEvents.push_back( vsg::ButtonReleaseEvent::create(
			    vsgwindow_, eventtime, xpos, ypos, mask, button) );
    if ( viewer_ )
	viewer_->request();
}


void dGBVSGWindow::wheelEvent( QWheelEvent* event )
{
    if ( !vsgwindow_ || !event )
	return;

    const vsg::clock::time_point eventtime = vsg::clock::now();
    const vsg::vec3 delta = event->angleDelta().y() < 0
				    ? vsg::vec3( 0.f, -1.f, 0.f )
				    : vsg::vec3( 0.f, 1.f, 0.f );
    vsgwindow_->bufferedEvents.push_back(
		    vsg::ScrollWheelEvent::create(vsgwindow_,eventtime,delta) );
    if ( viewer_ )
	viewer_->request();
}
