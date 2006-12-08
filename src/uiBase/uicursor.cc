/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.cc,v 1.4 2006-12-08 15:58:03 cvsnanne Exp $
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
