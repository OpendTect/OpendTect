/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          03/03/2000
 RCS:           $Id: uidrawable.cc,v 1.1 2000-11-27 10:20:35 bert Exp $
________________________________________________________________________

-*/

#include <uidrawable.h>
#include <i_layout.h>

#include <qwidget.h>

uiDrawableObj::~uiDrawableObj()
{
}

QPaintDevice* uiDrawableObj::mQPaintDevice() 
{ 
    return &qWidget(); 
}
