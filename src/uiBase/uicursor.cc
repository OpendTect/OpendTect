/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          12/05/2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicursor.cc,v 1.17 2010/11/18 17:16:53 cvsjaap Exp $";

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


uiPoint uiCursorManager::cursorPos()
{ return uiPoint( QCursor::pos().x(), QCursor::pos().y() ); }


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


static bool prioritycursoractive_ = false;
static MouseCursor::Shape overrideshape_ = MouseCursor::NotSet;


void uiCursorManager::setPriorityCursor( MouseCursor::Shape mcshape )
{
    if ( prioritycursoractive_ )
	unsetPriorityCursor();

    setOverride( mcshape );
    prioritycursoractive_ = true;
}


void uiCursorManager::unsetPriorityCursor()
{
    if ( !prioritycursoractive_ )
	return;

    prioritycursoractive_ = false;
    restoreOverride();
}


MouseCursor::Shape uiCursorManager::overrideCursorShape()
{ return overrideshape_; }


static void setOverrideQCursor( const QCursor& qcursor, bool replace )
{
    overrideshape_ = (MouseCursor::Shape) qcursor.shape();

    QCursor topcursor;
    const bool stackwasempty = !QApplication::overrideCursor();
    if ( !stackwasempty )
	topcursor = *QApplication::overrideCursor();

    if ( prioritycursoractive_ && !stackwasempty )
	QApplication::restoreOverrideCursor();

    if ( replace )
	QApplication::changeOverrideCursor( qcursor );
    else
	QApplication::setOverrideCursor( qcursor );

    if ( prioritycursoractive_ && !stackwasempty )
	QApplication::setOverrideCursor( topcursor );
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


#define mStoreOverrideShape() \
{ \
    overrideshape_ = MouseCursor::NotSet; \
    if ( QApplication::overrideCursor() ) \
    { \
	const QCursor overridecursor = *QApplication::overrideCursor(); \
	overrideshape_ = (MouseCursor::Shape) overridecursor.shape(); \
    } \
}

void uiCursorManager::restoreInternal()
{
    if ( !QApplication::overrideCursor() )
	return;

    const QCursor topcursor = *QApplication::overrideCursor();
    QApplication::restoreOverrideCursor();

    if ( !prioritycursoractive_ )
    {
	mStoreOverrideShape();
	return;
    }

    QApplication::restoreOverrideCursor();
    mStoreOverrideShape();
    QApplication::setOverrideCursor( topcursor );
}
