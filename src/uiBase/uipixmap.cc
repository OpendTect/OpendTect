/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
________________________________________________________________________

-*/

#include "uipixmap.h"

#include "arraynd.h"
#include "file.h"
#include "fileformat.h"
#include "odiconfile.h"

#include "uirgbarray.h"

#include <QBitmap>
#include <QColor>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QPixmap>

mUseQtnamespace


uiPixmap::uiPixmap()
    : qpixmap_(0)
{}


uiPixmap::uiPixmap( const uiPixmap& pm )
    : qpixmap_(new QPixmap(*pm.qpixmap_))
    , srcname_(pm.srcname_)
{
}


uiPixmap::uiPixmap( const uiRGBArray& rgbarr )
    : qpixmap_(new QPixmap)
    , srcname_("[uiRGBArray]")
{
    convertFromRGBArray( rgbarr );
}


uiPixmap::uiPixmap( const char* xpm[] )
    : qpixmap_(new QPixmap(xpm))
    , srcname_("[xpm]")
{
}


uiPixmap::uiPixmap( int w, int h, Color col )
    : qpixmap_(new QPixmap( w<2 ? 1 : w, h<2 ? 2 : h ))
    , srcname_("[created]")
{
    if ( col != Color::NoColor() )
	fill( col );
}


uiPixmap::uiPixmap( const QPixmap& pm )
    : qpixmap_(new QPixmap(pm))
{
}


uiPixmap::uiPixmap( const char* icnm )
    : qpixmap_(0)
    , srcname_(icnm)
{
    OD::IconFile icfile( icnm );
    if ( !icfile.haveData() )
	{ qpixmap_ = new QPixmap; return; }

    qpixmap_ = new QPixmap( icfile.fileNames().get(0).str(), 0 );
}


uiPixmap::~uiPixmap()
{
    delete qpixmap_;
}


void uiPixmap::convertFromRGBArray( const uiRGBArray& rgbarr )
{
    if ( !qpixmap_ ) qpixmap_ = new QPixmap;
    *qpixmap_ = QPixmap::fromImage( rgbarr.qImage(),
				    Qt::OrderedAlphaDither );
}


int uiPixmap::width() const
{ return qpixmap_->width(); }

int uiPixmap::height() const
{ return qpixmap_->height(); }

bool uiPixmap::isEmpty() const
{ return !qpixmap_ || qpixmap_->isNull(); }


void uiPixmap::fill( const Color& col )
{
    qpixmap_->fill( QColor(col.r(),col.g(),col.b()) );
    QPainter painter( qpixmap_ );
    painter.drawRect( 0, 0, qpixmap_->width()-1, qpixmap_->height()-1 );
}


bool uiPixmap::save( const char* fnm, const char* fmt, int quality ) const
{ return qpixmap_ ? qpixmap_->save( fnm, fmt, quality ) : false; }


bool uiPixmap::isPresent( const char* icnm )
{
    return OD::IconFile::isPresent( icnm );
}

static File::FormatList* readimgfmtsinst_ = 0;
static File::FormatList* writeimgfmtsinst_ = 0;
static File::FormatList* printimgfmtsinst_ = 0;

static void ensureImgFmtsLists()
{
    if ( readimgfmtsinst_ )
	return;

    readimgfmtsinst_ = new File::FormatList;
    writeimgfmtsinst_ = new File::FormatList;
    printimgfmtsinst_ = new File::FormatList;
    const uiString fmtnm( od_static_tr("ImgFmtsLists","Image files") );

    File::Format rdfmt( fmtnm );
    QList<QByteArray> imgfrmts = QImageReader::supportedImageFormats();
    for ( int idx=0; idx<imgfrmts.size(); idx++ )
	rdfmt.addExtension( imgfrmts[idx].data() );
    readimgfmtsinst_->addFormat( rdfmt );

    imgfrmts = QImageWriter::supportedImageFormats();
    File::Format wrfmt( fmtnm );
    for ( int idx=0; idx<imgfrmts.size(); idx++ )
	wrfmt.addExtension( imgfrmts[idx].data() );
    writeimgfmtsinst_->addFormat( wrfmt );

    File::Format prfmt( od_static_tr("ImgFmtsLists","Printable files"),
		        "pdf", "ps", "eps" );
    printimgfmtsinst_->addFormat( prfmt );
}


void OD::GetSupportedImageFormats( File::FormatList& formats, bool forread,
				   bool withprint )
{
    ensureImgFmtsLists();

    formats = forread ? *readimgfmtsinst_ : *writeimgfmtsinst_;
    if ( withprint )
	formats.addFormats( *printimgfmtsinst_ );
}
