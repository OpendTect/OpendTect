#ifndef uieventfilter_h
#define uieventfilter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Nov 2012
 RCS:           $Id: uiobj.h 26297 2012-09-21 00:16:36Z nanne.hemstra@dgbes.com $
________________________________________________________________________

-*/

#include "callback.h"

mFDQtclass(QObject);
mFDQtclass(QEvent);

class uiEventFilterImpl;
class uiObject;
/*!\ The base class for most UI elements. */

mClass(uiBase) uiEventFilter : public CallBacker
{
public:
				uiEventFilter();
    virtual 			~uiEventFilter();

    enum EventType { None, AccessibilityDescription, AccessibilityHelp,
	AccessibilityPrepare, ActionAdded, ActionChanged, ActionRemoved,
	ActivationChange, ApplicationActivate, 
	ApplicationDeactivate, ApplicationFontChange,
	ApplicationLayoutDirectionChange, ApplicationPaletteChange,
	ApplicationWindowIconChange, ChildAdded, 
	ChildPolished, ChildRemoved, Clipboard, Close, CloseSoftwareInputPanel,
	ContentsRectChange, ContextMenu, CursorChange, DeferredDelete,
	DragEnter, DragLeave, DragMove, Drop, EnabledChange, Enter,
	EnterWhatsThisMode, FileOpen, FocusIn, FocusOut,
	FontChange, GrabKeyboard, GrabMouse, GraphicsSceneContextMenu,
	GraphicsSceneDragEnter, GraphicsSceneDragLeave, GraphicsSceneDragMove,
	GraphicsSceneDrop, GraphicsSceneHelp, GraphicsSceneHoverEnter,
	GraphicsSceneHoverLeave, GraphicsSceneHoverMove,
	GraphicsSceneMouseDoubleClick, GraphicsSceneMouseMove,
	GraphicsSceneMousePress, GraphicsSceneMouseRelease, GraphicsSceneMove,
	GraphicsSceneResize, GraphicsSceneWheel, Hide, HideToParent,
	HoverEnter, HoverLeave, HoverMove, IconDrag, IconTextChange,
	InputMethod, KeyPress, KeyRelease, LanguageChange,
	LayoutDirectionChange, LayoutRequest, Leave, 
	LeaveWhatsThisMode, LocaleChange, NonClientAreaMouseButtonDblClick,
	NonClientAreaMouseButtonPress, NonClientAreaMouseButtonRelease,
	NonClientAreaMouseMove, MacSizeChange, MenubarUpdated, MetaCall,
	ModifiedChange, MouseButtonDblClick, MouseButtonPress,
	MouseButtonRelease, MouseMove, MouseTrackingChange, Move, Paint,
	PaletteChange, ParentAboutToChange, ParentChange, Polish,
	PolishRequest, QueryWhatsThis, RequestSoftwareInputPanel, Resize,
	Shortcut, ShortcutOverride, Show, ShowToParent, SockAct,
	StateMachineSignal, StateMachineWrapped, StatusTip, StyleChange,
	TabletMove, TabletPress, TabletRelease, OkRequest,
	TabletEnterProximity, TabletLeaveProximity, Timer, ToolBarChange,
	ToolTip, ToolTipChange, UngrabKeyboard, UngrabMouse, UpdateLater,
	UpdateRequest, WhatsThis, WhatsThisClicked, Wheel, WinEventAct,
	WindowActivate, WindowBlocked, WindowDeactivate, WindowIconChange,
	WindowStateChange, WindowTitleChange, WindowUnblocked, ZOrderChange,
	KeyboardLayoutChange, DynamicPropertyChange, TouchBegin, TouchUpdate,
	TouchEnd, WinIdChange, Gesture, GestureOverride };

    void			addEventType(EventType);
    void			removeEventType(EventType);

    Notifier<uiEventFilter>	eventhappened;
    EventType			getCurrentEventType() const;
    				//Only set when notifier is triggered.
    const QEvent*		getCurrentEvent() const;
				//Only set when notifier is triggered.

    void			setBlockEvent(bool yn);
    bool			getBlockEvent() const;

    void			attachFilter(uiObject*);
    void			detachFilter();
    
protected:
    
    void			objRemovedCB(CallBacker*);

    uiEventFilterImpl*		impl_;
    uiObject*			obj_;
};


#endif

