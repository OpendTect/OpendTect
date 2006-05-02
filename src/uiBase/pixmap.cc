/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.cc,v 1.13 2006-05-02 19:40:00 cvsnanne Exp $
________________________________________________________________________

-*/

#include "pixmap.h"
#include "errh.h"

#include "color.h"
#include "arraynd.h"
#include "arrayndimpl.h"
#include "arrayrgb.h"
#include "colortab.h"

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

    
ioPixmap::ioPixmap( const char* tablename, int width, int height )
{
    if ( !tablename || !*tablename )
    {
	qpixmap = new QPixmap( width, height );
	fill( Color(255,255,255) );
	return;
    }

    ArrayRGB rgb( height, width );
    ColorTable ctab;
    ColorTable::get( tablename, ctab );
    ctab.calcList(width);
    
    Color color;
    for ( int idx1=0; idx1<rgb.info().getSize(1); idx1++ )
    {
	color = ctab.tableColor(idx1);
	
	for ( int idx2=0; idx2<rgb.info().getSize(0); idx2++ )
	    rgb.set( idx2, idx1, color );
    }
    qpixmap = new QPixmap;
    convertFromArrayRGB( rgb );
}


ioPixmap::~ioPixmap()
    { if ( qpixmap ) delete qpixmap; }


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


