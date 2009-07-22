#ifndef uicanvas_h
#define uicanvas_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.h,v 1.21 2009-07-22 16:01:20 cvsbert Exp $
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
