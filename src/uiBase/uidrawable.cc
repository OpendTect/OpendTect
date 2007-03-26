/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          03/03/2000
 RCS:           $Id: uidrawable.cc,v 1.6 2007-03-26 16:56:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidrawable.h"
#include "i_uidrwbody.h"
#include "iodrawimpl.h"
#include "iodrawtool.h"
#include "uiobjbody.h"
#include "i_layout.h"

#include <qwidget.h>


uiDrawableObj::uiDrawableObj( uiParent* p, const char* nm, uiObjectBody& b )
    : uiObject(p,nm,b)
    , preDraw(this), postDraw(this), reSized(this)
    , rubberbandbutton_(OD::LeftButton)
    , rubberbandon_(false)
{}


ioDrawTool* uiDrawableObj::drawTool_( int x0, int y0 )
{
    mDynamicCastGet( ioDrawArea*, drwbl, body() );
    return drwbl ? drwbl->drawTool( x0, y0 ) : 0;
}


int uiDrawableObj::width() const
{
    const ioDrawTool* dt = const_cast<uiDrawableObj*>(this)->drawTool_( 0, 0 );
    return dt ? dt->getDevWidth() : uiObject::width();
}


int uiDrawableObj::height() const
{
    const ioDrawTool* dt = const_cast<uiDrawableObj*>(this)->drawTool_( 0, 0 );
    return dt ? dt->getDevHeight() : uiObject::height();
}


MouseEventHandler& uiDrawableObj::getMouseEventHandler()
{
    return mousehandler_;
}
