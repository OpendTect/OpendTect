/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipixmap.h"

#include "arraynd.h"
#include "coltabindex.h"
#include "coltabsequence.h"
#include "file.h"
#include "filepath.h"
#include "odiconfile.h"
#include "settings.h"

#include "uirgbarray.h"
#include "uiicons.h"

#include <QPixmap>
#include <QBitmap>
#include <QColor>
#include <QImageWriter>

mUseQtnamespace


ioPixmap::ioPixmap( const ioPixmap& pm )
    : qpixmap_(new QPixmap(*pm.qpixmap_))
    , srcname_(pm.srcname_)
{
}


ioPixmap::ioPixmap( const uiRGBArray& rgbarr )
    : qpixmap_(new QPixmap)
    , srcname_("[uiRGBArray]")
{
    convertFromRGBArray( rgbarr );
}


ioPixmap::ioPixmap( const char* xpm[] )
    : qpixmap_(new QPixmap(xpm))
    , srcname_("[xpm]")
{
}


ioPixmap::ioPixmap( int w, int h )
    : qpixmap_(new QPixmap( w<2 ? 1 : w, h<2 ? 2 : h ))
    , srcname_("[created]")
{
    qpixmap_->fill( QColor(0,0,0,0) );
}


ioPixmap::ioPixmap( const QPixmap& pm )
    : qpixmap_(new QPixmap(pm))
{
}


ioPixmap::ioPixmap( const char* icnm, bool small )
    : qpixmap_(0)
    , srcname_(icnm)
{
    bool isnone = true;
    if ( srcname_.isEmpty() )
	{ pErrMsg("Empty icon name specified. "
		    " (if this is intentional, use uiIcon::None())"); }

    if ( srcname_ != uiIcon::None() )
	isnone = false;
    if ( isnone )
	{ qpixmap_ = new QPixmap; return; }

    OD::IconFile icfile( icnm );
    qpixmap_ = new QPixmap( icfile.fullFileName(small).str(), 0 );
}


ioPixmap::ioPixmap( const ColTab::Sequence& ctabin, int w, int h, bool hor )
    : qpixmap_(0)
    , srcname_("[colortable]")
{
    bool validsz = true;
    if ( w < 2 ) { w = 1; validsz = false; }
    if ( h < 2 ) { h = 1; validsz = false; }

    if ( ctabin.size() == 0 || !validsz )
    {
        qpixmap_ = new QPixmap( w, h );
        qpixmap_->fill( QColor(0,0,0,0) );
	return;
    }

    uiRGBArray rgbarr( false );
    rgbarr.setSize( w, h );
    if ( hor )
    {
	ColTab::IndexedLookUpTable table( ctabin, w );
	for ( int idx1=0; idx1<rgbarr.getSize(true); idx1++ )
	{
	    const Color color = table.colorForIndex( idx1 );
	    for ( int idx2=0; idx2<rgbarr.getSize(false); idx2++ )
		rgbarr.set( idx1, idx2, color );
	}
    }
    else // vertical colorbar
    {
	ColTab::IndexedLookUpTable table( ctabin, h );
	for ( int idx1=0; idx1<rgbarr.getSize(false); idx1++ )
	{
	    const Color color = table.colorForIndex( idx1 );
	    for ( int idx2=0; idx2<rgbarr.getSize(true); idx2++ )
		rgbarr.set( idx2, idx1, color );
	}
    }

    qpixmap_ = new QPixmap;
    convertFromRGBArray( rgbarr );
}


ioPixmap::~ioPixmap()
{
    delete qpixmap_;
}


void ioPixmap::convertFromRGBArray( const uiRGBArray& rgbarr )
{
    if ( !qpixmap_ ) qpixmap_ = new QPixmap;
    *qpixmap_ = QPixmap::fromImage( rgbarr.qImage(),
                                    Qt::OrderedAlphaDither );
}


int ioPixmap::width() const
{ return qpixmap_->width(); }

int ioPixmap::height() const
{ return qpixmap_->height(); }

bool ioPixmap::isEmpty() const
{ return !qpixmap_ || qpixmap_->isNull(); }

void ioPixmap::fill( const Color& col )
{ qpixmap_->fill( QColor(col.r(),col.g(),col.b()) ); }


bool ioPixmap::save( const char* fnm, const char* fmt, int quality ) const
{ return qpixmap_ ? qpixmap_->save( fnm, fmt, quality ) : false; }


bool ioPixmap::isPresent( const char* icnm )
{
    return OD::IconFile::isPresent( icnm );
}


void ioPixmap::supportedImageFormats( BufferStringSet& list )
{
    return uiRGBArray::supportedImageFormats( list );
}


void supportedImageFormats( BufferStringSet& imageformats )
{
    QList<QByteArray> imgfrmts = QImageWriter::supportedImageFormats();
    for ( int idx=0; idx<imgfrmts.size(); idx++ )
	imageformats.add( imgfrmts[idx].data() );
}


// ----- ioBitmap -----
ioBitmap::ioBitmap( const char* filenm, const char * format )
{
    qpixmap_ = new QBitmap( filenm, format );
    srcname_ = filenm;
}
