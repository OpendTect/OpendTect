#ifndef uicanvas_h
#define uicanvas_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.h,v 1.20 2009-01-30 05:05:23 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidrawable.h"

class uiCanvasBody;

mClass uiCanvas : public uiDrawableObj
{
public:
				uiCanvas(uiParent*,const Color&,const char*);
    virtual			~uiCanvas()			{}

    void			update();
    void			setMouseTracking(bool);
    bool			hasMouseTracking() const;

    void			setBackgroundColor(const Color&);

    				//! Force activation in GUI thread
    void			activateMenu();
    Notifier<uiCanvas>		activatedone;

private:

    uiCanvasBody*		body_;
    uiCanvasBody&		mkbody(uiParent*,const char*);
};

#endif
