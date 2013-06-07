#ifndef uidrawable_h
#define uidrawable_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          01/02/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiobj.h"
#include "iodrawtool.h"
#include "mouseevent.h"
#include "keyboardevent.h"

mClass uiDrawableObj : public uiObject, public ioDrawArea
{
    mTTFriend(C,T,uiDrawableObjBody);

public:
			uiDrawableObj(uiParent* parnt,const char* nm, 
				      uiObjectBody&);
    virtual		~uiDrawableObj()	{}

    virtual ioDrawTool& drawTool();

    int			width() const	{ return cDT().getDevWidth(); }
    int			height() const	{ return cDT().getDevHeight(); }

    void		setRubberBandingOn( bool yn )	{ rubberbandon_ = yn; }
    bool		isRubberBandingOn() const	{ return rubberbandon_;}
    void		setRubberBandButton( OD::ButtonState bs )
			{ rubberbandbutton_ = bs; }
    OD::ButtonState	rubberBandButton()
    			{ return rubberbandbutton_; }

    MouseEventHandler&		getMouseEventHandler();
    KeyboardEventHandler&	getKeyboardEventHandler();

    Notifier<uiDrawableObj>	preDraw;
    Notifier<uiDrawableObj>	postDraw;
    Notifier<uiDrawableObj>	reSized;

mProtected:

/*! \brief handler for additional redrawing stuff

reDrawHandler() is called from uiDrawableObjBody::handlePaintEvent, which

1) triggers preDraw on associated uiDrawableObj (i.e. this)

2) calls Qt's paintEvent handler on associated widget

3) calls reDrawHandler() on associated uiDrawableObj (i.e. this)

4) triggers postDraw on associated uiDrawableObj (i.e. this)

Subclasses can override this method to do some additional drawing.

\sa uiDrawableObjBody::handlePaintEvent( uiRect r, QPaintEvent* QPEv=0 )

*/
    virtual void        reDrawHandler(uiRect updateArea)	{}
    virtual void        reSizeHandler(uiSize,uiSize)		{}
    virtual void	rubberBandHandler(uiRect)		{}

    inline const ioDrawTool& cDT() const
    			{ return const_cast<uiDrawableObj*>(this)->drawTool(); }

    OD::ButtonState	rubberbandbutton_;
    bool		rubberbandon_;

private:

    MouseEventHandler		mousehandler_;
    KeyboardEventHandler 	keyboardhandler_;

};


#endif
