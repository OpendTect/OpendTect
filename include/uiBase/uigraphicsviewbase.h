#ifndef uigraphicsviewbase_h
#define uigraphicsviewbase_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		March 2009
 RCS:		$Id: uigraphicsviewbase.h,v 1.1 2009-03-26 08:17:42 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uigeom.h"
#include "mouseevent.h"

class Color;
class uiGraphicsScene;
class uiGraphicsViewBody;
class uiRect;


mClass uiGraphicsViewBase : public uiObject
{
friend class uiGraphicsViewBody;
public:
				uiGraphicsViewBase(uiParent*,const char*);
				~uiGraphicsViewBase();

    void			setScene(uiGraphicsScene&);
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

    uiPoint			getCursorPos() const; 
    uiPoint			getScenePos(float,float) const; 
    const uiRect*		getSelectedArea() const	{return selectedarea_;}
    void			setViewArea(double x,double y,
	    				    double w,double h);
    uiRect			getViewArea() const;

    void                        setBackgroundColor(const Color&);
    Color		        backgroundColor() const;
    void			rePaintRect(const uiRect*); 
    void			setZoomOnCtrlScroll( bool yn )	
				{ zoomonctrlscroll_ = yn; }
    bool			zoomOnCtrlScroll()
    				{ return zoomonctrlscroll_; }

    MouseEventHandler&		getMouseEventHandler();

    Notifier<uiGraphicsViewBase>	reSize;
    Notifier<uiGraphicsViewBase>	rubberBandUsed;
    Notifier<uiGraphicsViewBase>	reDrawNeeded;
    Notifier<uiGraphicsViewBase>	activatedone;

    				//! Force activation in GUI thread
    void			activateMenu();

protected:

    uiGraphicsViewBody*		body_;
    uiGraphicsViewBody&		mkbody(uiParent*,const char*);

    uiRect*			selectedarea_;
    uiGraphicsScene*		scene_;

    bool			zoomonctrlscroll_;
    void 			rubberBandCB(CallBacker*);
};

#endif
