/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril & H. Huck
 Date:          08/09/2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uirgbarray.cc,v 1.11 2008-12-24 05:55:22 cvsnanne Exp $";

#include "uirgbarray.h"

#include <QImage>
#include <QColor>
#include <QColormap>


uiRGBArray::uiRGBArray( bool walpha )
    : qimg_(new QImage)
    , withalpha_(walpha)
{
}


uiRGBArray::uiRGBArray( const uiRGBArray& rgbarr )
{
    qimg_ = new QImage( rgbarr.qImage() );
    qimg_->detach();
}


uiRGBArray::~uiRGBArray() 
{ 
    delete qimg_;
}


void uiRGBArray::setSize( int d0, int d1 )
{
    if ( qimg_->width() == d0 && qimg_->height() == d1 )
	return;

    delete qimg_;
    qimg_ = new QImage( d0, d1, withalpha_ ? QImage::Format_ARGB32 
	    				   : QImage::Format_RGB32 );
}


Color uiRGBArray::get( int i0, int i1 ) const
{
    if ( qimg_->width()<=i0 || qimg_->height()<=i1 )
	return Color::NoColor();
    
    Color c; c.rgb() = qimg_->pixel( i0, i1 );
    return c;
}


void uiRGBArray::set( int i0, int i1, const Color& c )
{
    if ( qimg_->width()<=i0 || qimg_->height()<=i1 )
	return;

    if ( withalpha_ )
    {
	const Color newcol( c.r(), c.g(), c.b(), 255-c.t() );
	qimg_->setPixel( i0, i1, (QRgb)newcol.rgb() );
    }
    else
	qimg_->setPixel( i0, i1, (QRgb)c.rgb() );
}


void uiRGBArray::clear( const Color& c )
{
    QColor col( (QRgb)c.rgb() );
    QColormap cmap = QColormap::instance();
    uint pixel = cmap.pixel( col );
    qimg_->fill( pixel );
}


int uiRGBArray::getSize( bool isx ) const
{
    return isx ? qimg_->width() : qimg_->height();
}
