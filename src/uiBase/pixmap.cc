/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.cc,v 1.11 2005-11-18 15:16:33 cvsarend Exp $
________________________________________________________________________

-*/

#include "pixmap.h"
#include "errh.h"

#include "color.h"
#include "arraynd.h"
#include "arrayndimpl.h"
#include "arrayrgb.h"

#include <qpixmap.h>
#include <qbitmap.h>
#include <qcolor.h>


ioPixmap::ioPixmap( const ioPixmap& pm )
{
    qpixmap = new QPixmap( *pm.qpixmap );
}


ioPixmap::ioPixmap( const ArrayRGB& anImage )
{
    qpixmap = new QPixmap;
    convertFromArrayRGB( anImage );
}


ioPixmap::ioPixmap( const char* xpm[] )
    { qpixmap = new QPixmap( xpm ); }


ioPixmap::ioPixmap( int w, int h )
    { qpixmap = new QPixmap( w, h ); }


ioPixmap::ioPixmap( const QPixmap& pm )
    { qpixmap = new QPixmap( pm ); }


ioPixmap::ioPixmap( const char* fileName, const char * format )
    { qpixmap = new QPixmap( fileName, format ); }


ioPixmap::~ioPixmap()
    { if(qpixmap) delete qpixmap; }


void ioPixmap::convertFromArrayRGB( const ArrayRGB & theImage )
{
    releaseDrawTool();

    if( !qpixmap ) qpixmap = new QPixmap;
#ifdef USEQT4
    *qpixmap = QPixmap::fromImage( theImage.Image(), Qt::OrderedAlphaDither);
#else
    qpixmap->convertFromImage( theImage.Image(), Qt::OrderedAlphaDither);
#endif
}    


QPaintDevice* ioPixmap::mQPaintDevice()
    { return qpixmap; }


int ioPixmap::width()
{
    return qpixmap->width();
}


int ioPixmap::height()
{
    return qpixmap->height();
}


void ioPixmap::fill( const Color& col )
{
    qpixmap->fill( QColor(col.r(),col.g(),col.b()) );
}


ioBitmap::ioBitmap( const char* fileName, const char * format )
    { qpixmap = new QBitmap( fileName, format ); }


