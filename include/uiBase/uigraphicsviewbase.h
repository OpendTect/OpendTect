#ifndef uigraphicsviewbase_h
#define uigraphicsviewbase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"

class Alignment;
class Color;
class uiGraphicsItem;
class uiGraphicsScene;
class uiGraphicsViewBody;
class KeyboardEventHandler;
class MouseEventHandler;
class uiRect;


mExpClass(uiBase) uiGraphicsViewBase : public uiObject
{
friend class uiGraphicsViewBody;
public:
				uiGraphicsViewBase(uiParent*,const char*);
				~uiGraphicsViewBase();

    void			setScene(uiGraphicsScene&);
    				//!<Scene becomes mine
    uiGraphicsScene&		scene();
    void			show();
    enum ODDragMode		{ NoDrag, ScrollHandDrag, RubberBandDrag };
    enum ScrollBarPolicy	{ ScrollBarAsNeeded, ScrollBarAlwaysOff,
				  ScrollBarAlwaysOn };

    void			setScrollBarPolicy(bool hor,ScrollBarPolicy); 
    void			setDragMode(ODDragMode); 
    ODDragMode			dragMode() const;
    bool			isRubberBandingOn() const;

    void			setMouseTracking(bool);
    bool			hasMouseTracking() const;

    int				width() const; 
    int				height() const; 

    int				getSceneBorder() const;
    void			setSceneBorder(int);

    void			centreOn(uiPoint);
    uiRect			getSceneRect() const;
    void			setSceneRect(const uiRect&);
    void			setSceneAlignment(const Alignment&);

    uiPoint			getCursorPos() const; 
    uiPoint			getScenePos(float,float) const; 
    const uiPoint& 		getStartPos() const;
    const uiRect*		getSelectedArea() const	{return selectedarea_;}
    void			setViewArea(double x,double y,
	    				    double w,double h);
    uiRect			getViewArea() const;

    void                        setBackgroundColor(const Color&);
    Color		        backgroundColor() const;
    void                        uisetBackgroundColor(const Color&);
    Color		        uibackgroundColor() const;
    void			setNoBackGround();
    void			rePaint(); 
    void			enableScrollZoom()  { enabscrollzoom_ = true; }
    void			disableScrollZoom() { enabscrollzoom_ = false; }
    bool			scrollZoomEnabled()
    				{ return enabscrollzoom_; }
    uiSize			scrollBarSize(bool horizontal) const;
    
    bool			isCtrlPressed() const	{return isctrlpressed_;}
    void			setCtrlPressed( bool yn )
    				{ isctrlpressed_ = yn; }

    MouseEventHandler&		getNavigationMouseEventHandler();
    				/*!<This eventhandler handles events related to
				    navigation (rubberbands & panning). For general
				    calls, use getMouseEventHandler(). */
    MouseEventHandler&		getMouseEventHandler();
    KeyboardEventHandler&	getKeyboardEventHandler();


    CNotifier<uiGraphicsViewBase,uiSize> reSize; //!< CallBacker is OLD size
    Notifier<uiGraphicsViewBase> rubberBandUsed;
    Notifier<uiGraphicsViewBase> reDrawNeeded;
    Notifier<uiGraphicsViewBase> reDrawn;
    				//!< In practice, this happens only after reSize
    Notifier<uiGraphicsViewBase> preDraw;
    Notifier<uiGraphicsViewBase> scrollBarUsed;


protected:

    uiGraphicsViewBody*		body_;
    uiGraphicsViewBody&		mkbody(uiParent*,const char*);

    uiRect*			selectedarea_;
    uiGraphicsScene*		scene_;
    int				sceneborder_;

    bool			isctrlpressed_;
    bool			enabscrollzoom_;
    bool			enabbgzoom_;
    void 			rubberBandCB(CallBacker*);

};

#endif

