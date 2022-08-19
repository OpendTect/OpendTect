/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicursor.h"
#include "uipixmap.h"
#include "uirgbarray.h"

#include <QCursor>
#include <QApplication>

mUseQtnamespace

void uiCursorManager::initClass()
{
    mDefineStaticLocalObject( uiCursorManager, uimgr, );
    MouseCursorManager::setMgr( &uimgr );
}


uiCursorManager::uiCursorManager()
{}


uiCursorManager::~uiCursorManager()
{}


uiPoint uiCursorManager::cursorPos()
{ return uiPoint( QCursor::pos().x(), QCursor::pos().y() ); }


#define ROTATE_WIDTH 16
#define ROTATE_HEIGHT 16
#define ROTATE_BYTES ((ROTATE_WIDTH + 7) / 8) * ROTATE_HEIGHT
#define ROTATE_HOT_X 6
#define ROTATE_HOT_Y 8

static unsigned char rotate_bitmap[ROTATE_BYTES] = {
    0xf0, 0xef, 0x18, 0xb8, 0x0c, 0x90, 0xe4, 0x83,
    0x34, 0x86, 0x1c, 0x83, 0x00, 0x81, 0x00, 0xff,
    0xff, 0x00, 0x81, 0x00, 0xc1, 0x38, 0x61, 0x2c,
    0xc1, 0x27, 0x09, 0x30, 0x1d, 0x18, 0xf7, 0x0f
};

static unsigned char rotate_mask_bitmap[ROTATE_BYTES] = {
    0xf0, 0xef, 0xf8, 0xff, 0xfc, 0xff, 0xfc, 0xff,
    0x3c, 0xfe, 0x1c, 0xff, 0x00, 0xff, 0x00, 0xff,
    0xff, 0x00, 0xff, 0x00, 0xff, 0x38, 0x7f, 0x3c,
    0xff, 0x3f, 0xff, 0x3f, 0xff, 0x1f, 0xf7, 0x0f
};


void uiCursorManager::fillQCursor( const MouseCursor& mc,
				   QCursor& qcursor )
{
    if ( mc.shape_==MouseCursor::Bitmap )
    {
	if ( !mc.filename_.isEmpty() )
	{
	    uiPixmap pixmap( mc.filename_ );
	    qcursor = QCursor( *pixmap.qpixmap(), mc.hotx_, mc.hoty_ );
	}
	else
	{
	    uiPixmap pixmap( uiRGBArray(*mc.image_)) ;
	    qcursor = QCursor( *pixmap.qpixmap(), mc.hotx_, mc.hoty_ );
	}
    }
    else if ( mc.shape_==MouseCursor::GreenArrow )
    {
	uiPixmap pixmap( "greenarrowcursor" );
	qcursor = QCursor( *pixmap.qpixmap(), 0, 0 );
    }
    else if ( mc.shape_==MouseCursor::Pencil )
    {
	uiPixmap pixmap( "pencil" );
	qcursor = QCursor( *pixmap.qpixmap(), 0, 0 );
    }
    else if ( mc.shape_==MouseCursor::Rotator )
    {
	uiRGBArray cursorimage( true );
	cursorimage.setSize( ROTATE_WIDTH, ROTATE_HEIGHT );
	cursorimage.putFromBitmap( rotate_bitmap, rotate_mask_bitmap );
	uiPixmap pixmap( cursorimage );
	qcursor = QCursor( *pixmap.qpixmap(), ROTATE_HOT_X, ROTATE_HOT_Y );
    }
    else
    {
	const Qt::CursorShape qshape =
	    			     (Qt::CursorShape)(int) mc.shape_;
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
    uiPixmap pixmap( fn );
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
	const QCursor overridecursor = \
					*QApplication::overrideCursor(); \
	overrideshape_ = (MouseCursor::Shape) overridecursor.shape(); \
    } \
}

void uiCursorManager::restoreInternal()
{
    if ( !QApplication::overrideCursor() )
	return;

    const QCursor topcursor =
				     *QApplication::overrideCursor();
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
