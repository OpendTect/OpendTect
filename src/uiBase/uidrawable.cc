/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          03/03/2000
 RCS:           $Id: uidrawable.cc,v 1.5 2007-02-14 12:38:00 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidrawable.h"
#include "i_uidrwbody.h"
#include "iodrawimpl.h"
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


MouseEventHandler& uiDrawableObj::getMouseEventHandler()
{
    return mousehandler_;
}
