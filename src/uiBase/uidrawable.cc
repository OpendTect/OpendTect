/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          03/03/2000
 RCS:           $Id: uidrawable.cc,v 1.2 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include <uidrawable.h>
#include <i_uidrwbody.h>
#include <iodrawimpl.h>
#include <uiobjbody.h>
#include <i_layout.h>


//#include <qpaintdevice.h>
#include <qwidget.h>

uiDrawableObj::uiDrawableObj( uiParent* parnt, const char* nm,  
			      uiObjectBody& b)
    : uiObject( parnt, nm, b ) , preDraw(this) , postDraw(this)
    , reSized(this)        					{}


ioDrawTool* uiDrawableObj::drawTool_( int x0, int y0 )
{
    mDynamicCastGet( ioDrawArea*, drwbl, body() );
    return drwbl ? drwbl->drawTool( x0, y0 ) : 0;
}
