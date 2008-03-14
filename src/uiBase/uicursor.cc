/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.cc,v 1.8 2008-03-14 14:35:45 cvskris Exp $
________________________________________________________________________

-*/

#include "uicursor.h"
#include "pixmap.h"

#include "qcursor.h"
#include "qapplication.h"


void uiCursorManager::initClass()
{
    MouseCursorManager::setMgr( new uiCursorManager );
}

uiCursorManager::uiCursorManager()
{ } 


uiCursorManager::~uiCursorManager()
{}


void uiCursorManager::setOverrideShape( MouseCursor::Shape sh, bool replace )
{
    Qt::CursorShape qshape = (Qt::CursorShape)(int) sh;
    QCursor qcursor;
    qcursor.setShape( qshape );
    QApplication::setOverrideCursor( qcursor,  replace );
}


void uiCursorManager::setOverrideFile( const char* fn, int hotx, int hoty,
				       bool replace )
{
    ioPixmap pixmap( fn );
    QApplication::setOverrideCursor( QCursor( *pixmap.qpixmap(), hotx, hoty ),
	    			     replace );
}


void uiCursorManager::setOverrideCursor( const MouseCursor& mc, bool replace )
{
    if ( mc.shape_==MouseCursor::Bitmap )
	setOverrideFile( mc.filename_, mc.hotx_, mc.hoty_, replace );
    else
	setOverrideShape( mc.shape_, replace );
}



void uiCursorManager::restoreInternal()
{
    QApplication::restoreOverrideCursor();
}
