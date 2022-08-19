#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "callback.h"

mFDQtclass(QObject);
mFDQtclass(QEvent);

class uiEventFilterImpl;
class uiBaseObject;


/*!\brief is a class that is able to recieve events from Qt and trigger an
  OD-style notification/callback. */

mExpClass(uiBase) uiEventFilter : public CallBacker
{
public:
				uiEventFilter();
    virtual			~uiEventFilter();

    enum EventType {
	None,
	ActionAdded,
	ActionChanged,
	ActionRemoved,
	ActivationChange,
	ApplicationActivate,
	ApplicationActivated,
	ApplicationFontChange,
	ApplicationLayoutDirectionChange,
	ApplicationPaletteChange,
	ApplicationStateChange,
	ApplicationWindowIconChange,
	ChildAdded,
	ChildPolished,
	ChildRemoved,
	Clipboard,
	Close,
	CloseSoftwareInputPanel,
	ContentsRectChange,
	ContextMenu,
	CursorChange,
	DeferredDelete,
	DragEnter,
	DragLeave,
	DragMove,
	Drop,
	DynamicPropertyChange,
	EnabledChange,
	Enter,
	EnterEditFocus,
	EnterWhatsThisMode,
	Expose,
	FileOpen,
	FocusIn,
	FocusOut,
	FocusAboutToChange,
	FontChange,
	Gesture,
	GestureOverride,
	GrabKeyboard,
	GrabMouse,
	GraphicsSceneContextMenu,
	GraphicsSceneDragEnter,
	GraphicsSceneDragLeave,
	GraphicsSceneDragMove,
	GraphicsSceneDrop,
	GraphicsSceneHelp,
	GraphicsSceneHoverEnter,
	GraphicsSceneHoverLeave,
	GraphicsSceneHoverMove,
	GraphicsSceneMouseDoubleClick,
	GraphicsSceneMouseMove,
	GraphicsSceneMousePress,
	GraphicsSceneMouseRelease,
	GraphicsSceneMove,
	GraphicsSceneResize,
	GraphicsSceneWheel,
	Hide,
	HideToParent,
	HoverEnter,
	HoverLeave,
	HoverMove,
	IconDrag,
	IconTextChange,
	InputMethod,
	InputMethodQuery,
	KeyboardLayoutChange,
	KeyPress,
	KeyRelease,
	LanguageChange,
	LayoutDirectionChange,
	LayoutRequest,
	Leave,
	LeaveEditFocus,
	LeaveWhatsThisMode,
	LocaleChange,
	NonClientAreaMouseButtonDblClick,
	NonClientAreaMouseButtonPress,
	NonClientAreaMouseButtonRelease,
	NonClientAreaMouseMove,
	MacSizeChange,
	MetaCall,
	ModifiedChange,
	MouseButtonDblClick,
	MouseButtonPress,
	MouseButtonRelease,
	MouseMove,
	MouseTrackingChange,
	Move,
	OrientationChange,
	Paint,
	PaletteChange,
	ParentAboutToChange,
	ParentChange,
	PlatformPanel,
	Polish,
	PolishRequest,
	QueryWhatsThis,
	RequestSoftwareInputPanel,
	Resize,
	ScrollPrepare,
	Scroll,
	Shortcut,
	ShortcutOverride,
	Show,
	ShowToParent,
	SockAct,
	StateMachineSignal,
	StateMachineWrapped,
	StatusTip,
	StyleChange,
	TabletMove,
	TabletPress,
	TabletRelease,
	OkRequest,
	TabletEnterProximity,
	TabletLeaveProximity,
	ThreadChange,
	Timer,
	ToolBarChange,
	ToolTip,
	ToolTipChange,
	TouchBegin,
	TouchCancel,
	TouchEnd,
	TouchUpdate,
	UngrabKeyboard,
	UngrabMouse,
	UpdateLater,
	UpdateRequest,
	WhatsThis,
	WhatsThisClicked,
	Wheel,
	WinEventAct,
	WindowActivate,
	WindowBlocked,
	WindowDeactivate,
	WindowIconChange,
	WindowStateChange,
	WindowTitleChange,
	WindowUnblocked,
	WinIdChange,
	ZOrderChange
    };

    void			addEventType(EventType);
    void			removeEventType(EventType);

    Notifier<uiEventFilter>	eventhappened;
    EventType			getCurrentEventType() const;
				//!<Only set when notifier is triggered.
    QEvent*			getCurrentEvent();
				//!<Only set when notifier is triggered.
    const QEvent*		getCurrentEvent() const;
				//!<Only set when notifier is triggered.

    void			setBlockEvent(bool yn);
    bool			getBlockEvent() const;

    void			attachToQObj(QObject*);
    void			attach(uiBaseObject*);
    void			detach();

protected:

    uiEventFilterImpl*		impl_;

};
