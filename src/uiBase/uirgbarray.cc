/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril & H. Huck
 Date:          08/09/2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uirgbarray.h"

#include "errh.h"

#include <QImage>
#include <QColor>
#include <QColormap>


uiRGBArray::uiRGBArray( bool walpha )
    : qimg_(new mQtclass(QImage))
    , withalpha_(walpha)
{
}


uiRGBArray::uiRGBArray( const OD::RGBImage& image )
{
    mDynamicCastGet( const uiRGBArray*, input, &image );
    if ( !input )
	pErrMsg( "Not supported. Go ahead and implement");
    
    qimg_ = new mQtclass(QImage)( input->qImage() );
    qimg_->detach();
}

uiRGBArray::~uiRGBArray()
{ 
    delete qimg_;
}


bool uiRGBArray::setSize( int d0, int d1 )
{
    if ( qimg_->width() == d0 && qimg_->height() == d1 )
	return true;

    delete qimg_;
    qimg_ = new mQtclass(QImage)( d0, d1,
	    			  withalpha_ ? mQtclass(QImage)::Format_ARGB32 
	    				     : mQtclass(QImage)::Format_RGB32 );

    return true;
}


Color uiRGBArray::get( int i0, int i1 ) const
{
    if ( qimg_->width()<=i0 || qimg_->height()<=i1 )
	return Color::NoColor();
    
    Color c; c.rgb() = qimg_->pixel( i0, i1 );
    return c;
}


bool uiRGBArray::set( int i0, int i1, const Color& c )
{
    if ( qimg_->width()<=i0 || qimg_->height()<=i1 )
	return false;

    if ( withalpha_ )
    {
	const Color newcol( c.r(), c.g(), c.b(), 255-c.t() );
	qimg_->setPixel( i0, i1, (mQtclass(QRgb))newcol.rgb() );
    }
    else
	qimg_->setPixel( i0, i1, (mQtclass(QRgb))c.rgb() );

    return true;
}


void uiRGBArray::clear( const Color& c )
{
    mQtclass(QColor) col( (mQtclass(QRgb))c.rgb() );
    mQtclass(QColormap) cmap = mQtclass(QColormap)::instance();
    uint pixel = cmap.pixel( col );
    qimg_->fill( pixel );
}


int uiRGBArray::getSize( bool isx ) const
{
    return isx ? qimg_->width() : qimg_->height();
}
