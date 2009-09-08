#ifndef uicanvas_h
#define uicanvas_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.h,v 1.22 2009-09-08 15:17:08 cvsbert Exp $
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


private:

    uiCanvasBody*		body_;
    uiCanvasBody&		mkbody(uiParent*,const char*);

public:
				//! Force activation in GUI thread
				//! Not for casual use
    void			activateMenu();
    Notifier<uiCanvas>		activatedone;
};

#endif
