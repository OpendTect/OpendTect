/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.cc,v 1.7 2003-09-08 13:03:04 nanne Exp $
________________________________________________________________________

-*/

#include "pixmap.h"
#include "errh.h"

#include "color.h"
#include "arraynd.h"
#include "arrayndimpl.h"
#include "arrayrgb.h"

#include <qpixmap.h>
#include <qcolor.h>

ioPixmap::ioPixmap( const ArrayRGB& anImage )
{
    qpixmap = new QPixmap;
    if( !convertFromArrayRGB( anImage ) )
    {
        ErrMsg( "Could not create Pixmap" );
        pErrMsg( "Qt's QPixmap::convertFromImage returned with error" );
    }
}


ioPixmap::ioPixmap( const char* xpm[] )
    { qpixmap = new QPixmap( xpm ); }


ioPixmap::ioPixmap( int w, int h, int depth)
    { qpixmap = new QPixmap( w, h, depth ); }


ioPixmap::ioPixmap( const QPixmap& pm )
    { qpixmap = new QPixmap( pm ); }


ioPixmap::ioPixmap( const char* fileName, const char * format )
    { qpixmap = new QPixmap( fileName, format ); }


ioPixmap::~ioPixmap()
    { if(qpixmap) delete qpixmap; }


bool ioPixmap::convertFromArrayRGB( const ArrayRGB & theImage )
{
    releaseDrawTool();

    if( !qpixmap ) qpixmap = new QPixmap;
    return qpixmap->convertFromImage( theImage.Image(), Qt::OrderedAlphaDither);
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
