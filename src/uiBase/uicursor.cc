/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.cc,v 1.3 2004-09-14 06:37:01 kristofer Exp $
________________________________________________________________________

-*/

#include "uicursor.h"
#include "pixmap.h"

#include "qcursor.h"
#include "qapplication.h"
#include "qbitmap.h"

void uiCursor::setOverride( uiCursor::Shape shape, bool replace )
{
    QApplication::setOverrideCursor( QCursor((int)shape), replace );
}


void uiCursor::setOverride( const ioBitmap* shape, const ioBitmap* mask,
			    int hotX, int hotY, bool replace )
{
    if ( mask )
	QApplication::setOverrideCursor( QCursor(*shape->Bitmap(),
		  	*mask->Bitmap(),hotX,hotY), replace );
    else
	QApplication::setOverrideCursor(
		QCursor(*shape->Bitmap(),hotX,hotY), replace );

}


void uiCursor::restoreOverride()
{
    QApplication::restoreOverrideCursor();
}
