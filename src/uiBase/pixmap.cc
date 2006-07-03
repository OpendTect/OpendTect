/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.cc,v 1.14 2006-07-03 16:39:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "pixmap.h"
#include "errh.h"

#include "color.h"
#include "arraynd.h"
#include "arrayndimpl.h"
#include "arrayrgb.h"
#include "colortab.h"
#include "separstr.h"

#include <qpixmap.h>
#include <qbitmap.h>
#include <qcolor.h>


ioPixmap::ioPixmap( const ioPixmap& pm )
    : qpixmap_(new QPixmap(*pm.qpixmap_))
    , srcname_(pm.srcname_)
{
}


ioPixmap::ioPixmap( const ArrayRGB& anImage )
    : qpixmap_(new QPixmap)
    , srcname_("[ArrayRGB]")
{
    convertFromArrayRGB( anImage );
}


ioPixmap::ioPixmap( const char* xpm[] )
    : qpixmap_(new QPixmap(xpm))
    , srcname_("[xpm]")
{
}


ioPixmap::ioPixmap( int w, int h )
    : qpixmap_(new QPixmap(w,h))
    , srcname_("[created]")
{
}


ioPixmap::ioPixmap( const QPixmap& pm )
    : qpixmap_(new QPixmap(pm))
{
}


ioPixmap::ioPixmap( const char* filenm, const char* fmt )
    : qpixmap_(new QPixmap(filenm,fmt))
    , srcname_(filenm)
{
    if ( fmt )
    {
	FileMultiString fms( filenm );
	fms += fmt;
	srcname_ = fms;
    }
}

    
ioPixmap::ioPixmap( const char* tablename, int width, int height )
    : qpixmap_(0)
    , srcname_("[colortable]")
{
    if ( !tablename || !*tablename )
    {
	qpixmap_ = new QPixmap( width, height );
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
    qpixmap_ = new QPixmap;
    convertFromArrayRGB( rgb );
}


ioPixmap::~ioPixmap()
{
    if ( qpixmap_ )
	delete qpixmap_;
}


void ioPixmap::convertFromArrayRGB( const ArrayRGB& theImage )
{
    releaseDrawTool();

    if( !qpixmap_ ) qpixmap_ = new QPixmap;
#ifdef USEQT4
    *qpixmap_ = QPixmap::fromImage( theImage.Image(), Qt::OrderedAlphaDither);
#else
    qpixmap_->convertFromImage( theImage.Image(), Qt::OrderedAlphaDither);
#endif
}    


QPaintDevice* ioPixmap::mQPaintDevice()
{
    return qpixmap_;
}


int ioPixmap::width()
{
    return qpixmap_->width();
}


int ioPixmap::height()
{
    return qpixmap_->height();
}


void ioPixmap::fill( const Color& col )
{
    qpixmap_->fill( QColor(col.r(),col.g(),col.b()) );
}


ioBitmap::ioBitmap( const char* filenm, const char * format )
{
    qpixmap_ = new QBitmap( filenm, format );
    srcname_ = filenm;
}
