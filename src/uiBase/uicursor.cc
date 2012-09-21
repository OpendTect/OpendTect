/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          12/05/2004
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uicursor.h"
#include "pixmap.h"
#include "uirgbarray.h"

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
{ return uiPoint( mQtclass(QCursor)::pos().x(), mQtclass(QCursor)::pos().y() ); }


void uiCursorManager::fillQCursor( const MouseCursor& mc,
				   mQtclass(QCursor&) qcursor )
{
    if ( mc.shape_==MouseCursor::Bitmap )
    {
	if ( !mc.filename_.isEmpty() )
	{
	    ioPixmap pixmap( mc.filename_ );
	    qcursor = mQtclass(QCursor)( *pixmap.qpixmap(), mc.hotx_, mc.hoty_ );
	}
	else
	{
	    ioPixmap pixmap( uiRGBArray(*mc.image_)) ;
	    qcursor = mQtclass(QCursor)( *pixmap.qpixmap(), mc.hotx_, mc.hoty_ );
	}
    }
    else
    {
	const mQtclass(Qt)::CursorShape qshape =
	    			     (mQtclass(Qt)::CursorShape)(int) mc.shape_;
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


static void setOverrideQCursor( const mQtclass(QCursor&) qcursor, bool replace )
{
    overrideshape_ = (MouseCursor::Shape) qcursor.shape();

    mQtclass(QCursor) topcursor;
    const bool stackwasempty = !mQtclass(QApplication)::overrideCursor();
    if ( !stackwasempty )
	topcursor = *mQtclass(QApplication)::overrideCursor();

    if ( prioritycursoractive_ && !stackwasempty )
	mQtclass(QApplication)::restoreOverrideCursor();

    if ( replace )
	mQtclass(QApplication)::changeOverrideCursor( qcursor );
    else
	mQtclass(QApplication)::setOverrideCursor( qcursor );

    if ( prioritycursoractive_ && !stackwasempty )
	mQtclass(QApplication)::setOverrideCursor( topcursor );
}


void uiCursorManager::setOverrideShape( MouseCursor::Shape sh, bool replace )
{
    mQtclass(Qt)::CursorShape qshape = (mQtclass(Qt)::CursorShape)(int) sh;
    mQtclass(QCursor) qcursor;
    qcursor.setShape( qshape );
    setOverrideQCursor( qcursor, replace );
}


void uiCursorManager::setOverrideFile( const char* fn, int hotx, int hoty,
				       bool replace )
{
    ioPixmap pixmap( fn );
    mQtclass(QCursor) qcursor( *pixmap.qpixmap(), hotx, hoty );
    setOverrideQCursor( qcursor, replace );
}


void uiCursorManager::setOverrideCursor( const MouseCursor& mc, bool replace )
{
    mQtclass(QCursor) qcursor;
    fillQCursor( mc, qcursor );
    setOverrideQCursor( qcursor, replace );
}


#define mStoreOverrideShape() \
{ \
    overrideshape_ = MouseCursor::NotSet; \
    if ( mQtclass(QApplication)::overrideCursor() ) \
    { \
	const mQtclass(QCursor) overridecursor = \
					*QApplication::overrideCursor(); \
	overrideshape_ = (MouseCursor::Shape) overridecursor.shape(); \
    } \
}

void uiCursorManager::restoreInternal()
{
    if ( !mQtclass(QApplication)::overrideCursor() )
	return;

    const mQtclass(QCursor) topcursor =
				     *mQtclass(QApplication)::overrideCursor();
    mQtclass(QApplication)::restoreOverrideCursor();

    if ( !prioritycursoractive_ )
    {
	mStoreOverrideShape();
	return;
    }

    mQtclass(QApplication)::restoreOverrideCursor();
    mStoreOverrideShape();
    mQtclass(QApplication)::setOverrideCursor( topcursor );
}
