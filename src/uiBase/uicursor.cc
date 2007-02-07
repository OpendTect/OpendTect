/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.cc,v 1.5 2007-02-07 16:44:02 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uicursor.h"

#include "qcursor.h"
#include "qapplication.h"


void uiCursor::setOverride( uiCursor::Shape shape, bool replace )
{
    Qt::CursorShape qshape = (Qt::CursorShape)(int)shape;
    QApplication::setOverrideCursor( QCursor(qshape), replace );
}


void uiCursor::restoreOverride()
{
    QApplication::restoreOverrideCursor();
}
