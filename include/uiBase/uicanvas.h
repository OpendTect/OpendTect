#ifndef uicanvas_h
#define uicanvas_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.h,v 1.24 2012-08-03 13:00:50 cvskris Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uidrawable.h"

class uiCanvasBody;

mClass(uiBase) uiCanvas : public uiDrawableObj
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

};

#endif

