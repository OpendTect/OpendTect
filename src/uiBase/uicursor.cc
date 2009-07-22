/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          12/05/2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicursor.cc,v 1.13 2009-07-22 16:01:38 cvsbert Exp $";

#include "uicursor.h"
#include "pixmap.h"

#include <QCursor>
#include <QApplication>


void uiCursorManager::initClass()
{
    static uiCursorManager uimgr;
    MouseCursorManager::setMgr( &uimgr );
}


uiCursorManager::uiCursorManager()
{} 


uiCursorManager::~uiCursorManager()
{}


void uiCursorManager::fillQCursor( const MouseCursor& mc, QCursor& qcursor )
{
    if ( mc.shape_==MouseCursor::Bitmap )
    {
	ioPixmap pixmap( mc.filename_ );
	qcursor = QCursor( *pixmap.qpixmap(), mc.hotx_, mc.hoty_ );
    }
    else
    {
	const Qt::CursorShape qshape = (Qt::CursorShape)(int) mc.shape_;
	qcursor.setShape( qshape );
    }
}


static void setOverrideQCursor( const QCursor& qcursor, bool replace )
{
    if ( replace )
	QApplication::changeOverrideCursor( qcursor );
    else
	QApplication::setOverrideCursor( qcursor );
}


void uiCursorManager::setOverrideShape( MouseCursor::Shape sh, bool replace )
{
    Qt::CursorShape qshape = (Qt::CursorShape)(int) sh;
    QCursor qcursor;
    qcursor.setShape( qshape );
    setOverrideQCursor( qcursor, replace );
}


void uiCursorManager::setOverrideFile( const char* fn, int hotx, int hoty,
				       bool replace )
{
    ioPixmap pixmap( fn );
    QCursor qcursor( *pixmap.qpixmap(), hotx, hoty );
    setOverrideQCursor( qcursor, replace );
}


void uiCursorManager::setOverrideCursor( const MouseCursor& mc, bool replace )
{
    QCursor qcursor;
    fillQCursor( mc, qcursor );
    setOverrideQCursor( qcursor, replace );
}


void uiCursorManager::restoreInternal()
{
    QApplication::restoreOverrideCursor();
}
