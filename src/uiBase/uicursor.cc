/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.cc,v 1.1 2004-05-12 12:11:19 arend Exp $
________________________________________________________________________

-*/

#include "uicursor.h"
#include "qcursor.h"
#include "qapplication.h"

void uiCursor::setOverrideCursor( CursorShape shape, bool replace )
{
    QApplication::setOverrideCursor( QCursor((int) shape), replace );
}


void uiCursor::restoreOverrideCursor()
{
    QApplication::restoreOverrideCursor();
}


