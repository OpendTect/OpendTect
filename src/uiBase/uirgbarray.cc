/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril & H. Huck
 Date:          08/09/2006
 RCS:           $Id: uirgbarray.cc,v 1.3 2006-09-27 15:06:24 cvshelene Exp $
________________________________________________________________________

-*/

#include "uirgbarray.h"
#include <qimage.h>


#ifdef USEQT4
# define mImgFmt QImage::Format_RGB32
#else
# define mImgFmt 32
#endif

uiRGBArray::uiRGBArray()
        : qimg_(new QImage)
{
}

uiRGBArray::uiRGBArray( const uiRGBArray& rgbarr )
{
    qimg_ = new QImage( rgbarr.Image() );
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
    qimg_ = new QImage( d0, d1, mImgFmt );
}


Color uiRGBArray::get( int i0, int i1 ) const
{
    if ( qimg_->width()<=i0 || qimg_->height()<=i1 )
	return Color::NoColor;
    
    Color c; c.rgb() = qimg_->pixel( i0, i1 );
    return c;
}


void uiRGBArray::set( int i0, int i1, const Color& c )
{
    if ( qimg_->width()<=i0 || qimg_->height()<=i1 )
	return;
    
    qimg_->setPixel( i0, i1, (QRgb)c.rgb() );
}
