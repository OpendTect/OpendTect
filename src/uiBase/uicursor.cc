/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.cc,v 1.2 2004-05-17 13:56:37 bert Exp $
________________________________________________________________________

-*/

#include "uicursor.h"
#include "qcursor.h"
#include "qapplication.h"

void uiCursor::setOverride( uiCursor::Shape shape, bool replace )
{
    QApplication::setOverrideCursor( QCursor((int)shape), replace );
}


void uiCursor::restoreOverride()
{
    QApplication::restoreOverrideCursor();
}
