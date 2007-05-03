/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.cc,v 1.6 2007-05-03 16:24:42 cvskris Exp $
________________________________________________________________________

-*/

#include "uicursor.h"
#include "pixmap.h"

#include "qcursor.h"
#include "qapplication.h"


void uiCursor::setOverride( uiCursor::Shape shape, bool replace )
{
    Qt::CursorShape qshape = (Qt::CursorShape)(int)shape;
    QApplication::setOverrideCursor( QCursor(qshape), replace );
}


void uiCursor::setOverride( const ioPixmap& pm, int hotx, int hoty,
			    bool replace )
{
    QApplication::setOverrideCursor( QCursor( *pm.qpixmap(), hotx, hoty ),
	    			     replace );
}


void uiCursor::restoreOverride()
{
    QApplication::restoreOverrideCursor();
}
