/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          03/03/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidrawable.cc,v 1.10 2009/07/22 16:01:38 cvsbert Exp $";

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


ioDrawTool& uiDrawableObj::drawTool()
{
    mDynamicCastGet(ioDrawArea*,drwbl,body());
    if ( !drwbl ) pErrMsg("body() is not ioDrawArea. Crash follows");
    return drwbl->drawTool();
}


MouseEventHandler& uiDrawableObj::getMouseEventHandler()
{ return mousehandler_; }


KeyboardEventHandler& uiDrawableObj::getKeyboardEventHandler()
{ return keyboardhandler_; }
