/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "pixmap.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "bufstringset.h"
#include "color.h"
#include "coltabindex.h"
#include "coltabsequence.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "separstr.h"
#include "settings.h"
#include "uirgbarray.h"
#include "uiicons.h"

#include <QPixmap>
#include <QBitmap>
#include <QColor>
#include <QImageWriter>


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


static bool fileExists( BufferString& fnm )
{
    const BufferString pngfnm( fnm, ".png" );
    if ( File::exists(pngfnm) )
    {
	fnm = pngfnm;
	return true;
    }

    return File::exists( fnm );
}


ioPixmap::ioPixmap( const char* fnm, const char* fmt )
    : qpixmap_(0)
    , srcname_(fnm)
{
    bool isnone = true;
    if ( srcname_.isEmpty() )
	pErrMsg("Empty icon name specified. "
		" (if this is intentional, use uiIcon::None())");
    if ( srcname_ != uiIcon::None() )
	isnone = false;
    if ( isnone )
	{ qpixmap_ = new QPixmap; return; }

    if ( fmt )
    {
	FileMultiString fms( fnm );
	fms += fmt;
	srcname_ = fms;
    }

    BufferString fname( srcname_ );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
    {
	BufferString icsetnm;
	Settings::common().get( "Icon set name", icsetnm );
	if ( icsetnm.isEmpty() )
	    icsetnm = "Default";
	const BufferString dirnm( "icons.", icsetnm );

	fp.setPath( GetSettingsFileName(dirnm) );
	fname = fp.fullPath();
	if ( !fileExists(fname) )
	{
	    fp.setPath( mGetSetupFileName(dirnm) );
	    fname = fp.fullPath();
	}

	// fallback to Default
	if ( !fileExists(fname) )
	{
	    fp.setPath( mGetSetupFileName("icons.Default") );
	    fname = fp.fullPath();
	}
    }

    // final fallback (icon simply missing)
    if ( !fileExists(fname) )
    {
	pErrMsg(BufferString("Icon not found: '",fnm,"'"));
	fname = FilePath(mGetSetupFileName("icons.Default"),
			"iconnotfound.png").fullPath();
    }

    qpixmap_ = new QPixmap( fname.buf(), fmt );
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
    releaseDrawTool();
    delete qpixmap_;
}


void ioPixmap::convertFromRGBArray( const uiRGBArray& rgbarr )
{
    releaseDrawTool();

    if ( !qpixmap_ ) qpixmap_ = new QPixmap;
    *qpixmap_ = QPixmap::fromImage( rgbarr.qImage(), Qt::OrderedAlphaDither );
}    


QPaintDevice* ioPixmap::qPaintDevice()
{ return qpixmap_; }

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


void ioPixmap::supportedImageFormats( BufferStringSet& list )
{
    QList<QByteArray> qlist = QImageWriter::supportedImageFormats();
    for ( int idx=0; idx<qlist.size(); idx++ )
	list.add( qlist[idx].constData() );
}


// ----- ioBitmap -----
ioBitmap::ioBitmap( const char* filenm, const char * format )
{
    qpixmap_ = new QBitmap( filenm, format );
    srcname_ = filenm;
}


QBitmap* ioBitmap::Bitmap() { return (QBitmap*)qpixmap_; }
const QBitmap* ioBitmap::Bitmap() const { return (const QBitmap*)qpixmap_; }


void supportedImageFormats( BufferStringSet& imageformats )
{
    QList<QByteArray> imgfrmts = QImageWriter::supportedImageFormats();
    for ( int idx=0; idx<imgfrmts.size(); idx++ )
	imageformats.add( imgfrmts[idx].data() );
}
