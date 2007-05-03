/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.cc,v 1.7 2007-05-03 18:11:44 cvskris Exp $
________________________________________________________________________

-*/

#include "uicursor.h"
#include "pixmap.h"

#include "qcursor.h"
#include "qapplication.h"


uiCursor::uiCursor( Shape sh )
    : qcursor_( *new QCursor )
{
    setShape( sh );
}


uiCursor::uiCursor( const ioPixmap& pm, int hotx, int hoty )
    : qcursor_( *new QCursor )
{
    setPixmap( pm, hotx, hoty );
}


uiCursor::~uiCursor()
{ delete &qcursor_; }


void uiCursor::setShape( Shape sh )
{
    Qt::CursorShape qshape = (Qt::CursorShape)(int) sh;
    qcursor_.setShape( qshape );
}


void uiCursor::setPixmap( const ioPixmap& pm, int hotx, int hoty )
{
    qcursor_ = QCursor( *pm.qpixmap(), hotx, hoty );
}


void uiCursor::setOverride( const uiCursor& cursor, bool replace )
{
    QApplication::setOverrideCursor( cursor.qcursor_,  replace );
}


void uiCursor::restoreOverride()
{
    QApplication::restoreOverrideCursor();
}


const QCursor& uiCursor::qcursor() const { return qcursor_; }
