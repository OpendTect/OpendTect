/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uieventfilter.h"
#include "uiobj.h"
#include "i_qptr.h"

#include <QEvent>
#include <QWidget>

mUseQtnamespace

class uiEventFilterImpl : public QObject
{
public:
					uiEventFilterImpl( uiEventFilter& uif )
					    : uif_( uif )
					{}

    bool				eventFilter(QObject*,QEvent*) override;
    static uiEventFilter::EventType	translate(QEvent::Type);
    static QEvent::Type			translate(uiEventFilter::EventType);

					//Set at the events
    QEvent*				currentevent_;
    bool				blockevent_;

    TypeSet<QEvent::Type>		eventtypes_;

    void				attachFilter(QObject*);
    void				detachFilter();

protected:
    i_QPtrImpl				qobj_;
    uiEventFilter&			uif_;
};



uiEventFilter::uiEventFilter()
    : eventhappened( this )
    , impl_( new uiEventFilterImpl(*this) )
{}


uiEventFilter::~uiEventFilter()
{
    detach();
    delete impl_;
}


void uiEventFilter::addEventType( uiEventFilter::EventType tp )
{
    impl_->eventtypes_ += uiEventFilterImpl::translate( tp );
}


void uiEventFilter::removeEventType( uiEventFilter::EventType tp )
{
    impl_->eventtypes_ -= uiEventFilterImpl::translate( tp );
}


uiEventFilter::EventType uiEventFilter::getCurrentEventType() const
{ return uiEventFilterImpl::translate( impl_->currentevent_->type() ); }


void uiEventFilter::setBlockEvent(bool yn)
{ impl_->blockevent_ = yn; }


bool uiEventFilter::getBlockEvent() const
{ return impl_->blockevent_; }


QEvent* uiEventFilter::getCurrentEvent()
{ return impl_->currentevent_; }


const QEvent* uiEventFilter::getCurrentEvent() const
{ return impl_->currentevent_; }


void uiEventFilter::attach( uiBaseObject* obj )
{
    if ( !obj->getWidget() )
	return;

    attachToQObj( obj->getWidget() );
}


void uiEventFilter::attachToQObj(QObject* qobj )
{
    impl_->attachFilter( qobj );
}


void uiEventFilter::detach()
{
    impl_->detachFilter();
}


// uiEventFilterImpl
void uiEventFilterImpl::attachFilter( QObject* obj )
{
    if ( qobj_ ) return;

    qobj_ = obj;
    qobj_->installEventFilter( this );
}


void uiEventFilterImpl::detachFilter()
{
    Threads::Locker lckr( qobj_.lock_ );

    if ( qobj_ ) qobj_->removeEventFilter( this );
    qobj_ = 0;
}


bool uiEventFilterImpl::eventFilter( QObject* obj, QEvent* ev )
{
    Threads::Locker lckr( qobj_.lock_ );

    if ( !qobj_ )
	return false;

    if ( qobj_!=obj )
	return false;

    lckr.unlockNow(); //We only protect qobj_.

    if ( !eventtypes_.isPresent( ev->type() ) )
	return false;

    blockevent_ = false;
    currentevent_ = ev;

    uif_.eventhappened.trigger();

    currentevent_ = 0;
    return blockevent_;
}


#define mImplCase( fromtp, totp, enm ) \
    case fromtp::enm: return totp::enm

#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0) )
# define mEventsQt5( fromnmspace, tonmspace ) \
	mImplCase( fromnmspace, tonmspace, ApplicationStateChange ); \
	mImplCase( fromnmspace, tonmspace, Expose ); \
	mImplCase( fromnmspace, tonmspace, FocusAboutToChange ); \
	mImplCase( fromnmspace, tonmspace, InputMethodQuery ); \
	mImplCase( fromnmspace, tonmspace, OrientationChange ); \
	mImplCase( fromnmspace, tonmspace, ScrollPrepare ); \
	mImplCase( fromnmspace, tonmspace, Scroll ); \
	mImplCase( fromnmspace, tonmspace, TouchCancel );
# ifdef QT_KEYPAD_NAVIGATION
#  define mKeyPadEvents( fromnmspace, tonmspace ) \
	mImplCase( fromnmspace, tonmspace, EnterEditFocus ); \
	mImplCase( fromnmspace, tonmspace, LeaveEditFocus );
# else
# define mKeyPadEvents( fromnmspace, tonmspace )
# endif
#else
# define mEventsQt5( fromnmspace, tonmspace )
# define mKeyPadEvents( fromnmspace, tonmspace )
#endif

#define mImpTransform( fromnmspace, tonmspace ) \
    switch (tp) \
    { \
	mImplCase( fromnmspace, tonmspace, None ); \
	mImplCase( fromnmspace, tonmspace, ActionAdded ); \
	mImplCase( fromnmspace, tonmspace, ActionChanged ); \
	mImplCase( fromnmspace, tonmspace, ActionRemoved ); \
	mImplCase( fromnmspace, tonmspace, ActivationChange ); \
	mImplCase( fromnmspace, tonmspace, ApplicationFontChange ); \
	mImplCase( fromnmspace, tonmspace, ApplicationLayoutDirectionChange ); \
	mImplCase( fromnmspace, tonmspace, ApplicationPaletteChange ); \
	mImplCase( fromnmspace, tonmspace, ApplicationWindowIconChange ); \
	mImplCase( fromnmspace, tonmspace, ChildAdded ); \
	mImplCase( fromnmspace, tonmspace, ChildPolished ); \
	mImplCase( fromnmspace, tonmspace, ChildRemoved ); \
	mImplCase( fromnmspace, tonmspace, Clipboard ); \
	mImplCase( fromnmspace, tonmspace, Close ); \
	mImplCase( fromnmspace, tonmspace, CloseSoftwareInputPanel ); \
	mImplCase( fromnmspace, tonmspace, ContentsRectChange ); \
	mImplCase( fromnmspace, tonmspace, ContextMenu ); \
	mImplCase( fromnmspace, tonmspace, CursorChange ); \
	mImplCase( fromnmspace, tonmspace, DeferredDelete ); \
	mImplCase( fromnmspace, tonmspace, DragEnter ); \
	mImplCase( fromnmspace, tonmspace, DragLeave ); \
	mImplCase( fromnmspace, tonmspace, DragMove ); \
	mImplCase( fromnmspace, tonmspace, Drop ); \
	mImplCase( fromnmspace, tonmspace, DynamicPropertyChange ); \
	mImplCase( fromnmspace, tonmspace, EnabledChange ); \
	mImplCase( fromnmspace, tonmspace, Enter ); \
	mImplCase( fromnmspace, tonmspace, EnterWhatsThisMode ); \
	mImplCase( fromnmspace, tonmspace, FileOpen ); \
	mImplCase( fromnmspace, tonmspace, FocusIn ); \
	mImplCase( fromnmspace, tonmspace, FocusOut ); \
	mImplCase( fromnmspace, tonmspace, FontChange ); \
	mImplCase( fromnmspace, tonmspace, Gesture ); \
	mImplCase( fromnmspace, tonmspace, GestureOverride ); \
	mImplCase( fromnmspace, tonmspace, GrabKeyboard ); \
	mImplCase( fromnmspace, tonmspace, GrabMouse ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneContextMenu ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneDragEnter ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneDragLeave ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneDragMove ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneDrop ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneHelp ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneHoverEnter ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneHoverLeave ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneHoverMove ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneMouseDoubleClick ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneMouseMove ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneMousePress ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneMouseRelease ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneMove ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneResize ); \
	mImplCase( fromnmspace, tonmspace, GraphicsSceneWheel ); \
	mImplCase( fromnmspace, tonmspace, Hide ); \
	mImplCase( fromnmspace, tonmspace, HideToParent ); \
	mImplCase( fromnmspace, tonmspace, HoverEnter ); \
	mImplCase( fromnmspace, tonmspace, HoverLeave ); \
	mImplCase( fromnmspace, tonmspace, HoverMove ); \
	mImplCase( fromnmspace, tonmspace, IconDrag ); \
	mImplCase( fromnmspace, tonmspace, IconTextChange ); \
	mImplCase( fromnmspace, tonmspace, InputMethod ); \
	mImplCase( fromnmspace, tonmspace, KeyboardLayoutChange ); \
	mImplCase( fromnmspace, tonmspace, KeyPress ); \
	mImplCase( fromnmspace, tonmspace, KeyRelease ); \
	mImplCase( fromnmspace, tonmspace, LanguageChange ); \
	mImplCase( fromnmspace, tonmspace, LayoutDirectionChange ); \
	mImplCase( fromnmspace, tonmspace, LayoutRequest ); \
	mImplCase( fromnmspace, tonmspace, Leave ); \
	mImplCase( fromnmspace, tonmspace, LeaveWhatsThisMode ); \
	mImplCase( fromnmspace, tonmspace, LocaleChange ); \
	mImplCase( fromnmspace, tonmspace, NonClientAreaMouseButtonDblClick ); \
	mImplCase( fromnmspace, tonmspace, NonClientAreaMouseButtonPress ); \
	mImplCase( fromnmspace, tonmspace, NonClientAreaMouseButtonRelease ); \
	mImplCase( fromnmspace, tonmspace, NonClientAreaMouseMove ); \
	mImplCase( fromnmspace, tonmspace, MacSizeChange ); \
	mImplCase( fromnmspace, tonmspace, MetaCall ); \
	mImplCase( fromnmspace, tonmspace, ModifiedChange ); \
	mImplCase( fromnmspace, tonmspace, MouseButtonDblClick ); \
	mImplCase( fromnmspace, tonmspace, MouseButtonPress ); \
	mImplCase( fromnmspace, tonmspace, MouseButtonRelease ); \
	mImplCase( fromnmspace, tonmspace, MouseMove ); \
	mImplCase( fromnmspace, tonmspace, MouseTrackingChange ); \
	mImplCase( fromnmspace, tonmspace, Move ); \
	mImplCase( fromnmspace, tonmspace, Paint ); \
	mImplCase( fromnmspace, tonmspace, PaletteChange ); \
	mImplCase( fromnmspace, tonmspace, ParentAboutToChange ); \
	mImplCase( fromnmspace, tonmspace, ParentChange ); \
	mImplCase( fromnmspace, tonmspace, PlatformPanel ); \
	mImplCase( fromnmspace, tonmspace, Polish ); \
	mImplCase( fromnmspace, tonmspace, PolishRequest ); \
	mImplCase( fromnmspace, tonmspace, QueryWhatsThis ); \
	mImplCase( fromnmspace, tonmspace, RequestSoftwareInputPanel ); \
	mImplCase( fromnmspace, tonmspace, Resize ); \
	mImplCase( fromnmspace, tonmspace, Shortcut ); \
	mImplCase( fromnmspace, tonmspace, ShortcutOverride ); \
	mImplCase( fromnmspace, tonmspace, Show ); \
	mImplCase( fromnmspace, tonmspace, ShowToParent ); \
	mImplCase( fromnmspace, tonmspace, SockAct ); \
	mImplCase( fromnmspace, tonmspace, StateMachineSignal ); \
	mImplCase( fromnmspace, tonmspace, StateMachineWrapped ); \
	mImplCase( fromnmspace, tonmspace, StatusTip ); \
	mImplCase( fromnmspace, tonmspace, StyleChange ); \
	mImplCase( fromnmspace, tonmspace, TabletMove ); \
	mImplCase( fromnmspace, tonmspace, TabletPress ); \
	mImplCase( fromnmspace, tonmspace, TabletRelease ); \
	mImplCase( fromnmspace, tonmspace, OkRequest ); \
	mImplCase( fromnmspace, tonmspace, TabletEnterProximity ); \
	mImplCase( fromnmspace, tonmspace, TabletLeaveProximity ); \
	mImplCase( fromnmspace, tonmspace, ThreadChange ); \
	mImplCase( fromnmspace, tonmspace, Timer ); \
	mImplCase( fromnmspace, tonmspace, ToolBarChange ); \
	mImplCase( fromnmspace, tonmspace, ToolTip ); \
	mImplCase( fromnmspace, tonmspace, ToolTipChange ); \
	mImplCase( fromnmspace, tonmspace, TouchBegin ); \
	mImplCase( fromnmspace, tonmspace, TouchEnd ); \
	mImplCase( fromnmspace, tonmspace, TouchUpdate ); \
	mImplCase( fromnmspace, tonmspace, UngrabKeyboard ); \
	mImplCase( fromnmspace, tonmspace, UngrabMouse ); \
	mImplCase( fromnmspace, tonmspace, UpdateLater ); \
	mImplCase( fromnmspace, tonmspace, UpdateRequest ); \
	mImplCase( fromnmspace, tonmspace, WhatsThis ); \
	mImplCase( fromnmspace, tonmspace, WhatsThisClicked ); \
	mImplCase( fromnmspace, tonmspace, Wheel ); \
	mImplCase( fromnmspace, tonmspace, WinEventAct ); \
	mImplCase( fromnmspace, tonmspace, WindowActivate ); \
	mImplCase( fromnmspace, tonmspace, WindowBlocked ); \
	mImplCase( fromnmspace, tonmspace, WindowDeactivate ); \
	mImplCase( fromnmspace, tonmspace, WindowIconChange ); \
	mImplCase( fromnmspace, tonmspace, WindowStateChange ); \
	mImplCase( fromnmspace, tonmspace, WindowTitleChange ); \
	mImplCase( fromnmspace, tonmspace, WindowUnblocked ); \
	mImplCase( fromnmspace, tonmspace, WinIdChange ); \
	mImplCase( fromnmspace, tonmspace, ZOrderChange ); \
	mEventsQt5( fromnmspace, tonmspace ) \
	mKeyPadEvents( fromnmspace, tonmspace ) \
	     \
	default: \
	    break; \
    } \
    return tonmspace::None

QEvent::Type uiEventFilterImpl::translate( uiEventFilter::EventType tp )
{
    mImpTransform( uiEventFilter, QEvent );
}


uiEventFilter::EventType uiEventFilterImpl::translate( QEvent::Type tp )
{
    mImpTransform( QEvent, uiEventFilter );
}
