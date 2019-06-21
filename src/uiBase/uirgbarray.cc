/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert & H. Huck
 Date:          08/09/2006
________________________________________________________________________

-*/

#include "uirgbarray.h"

#include "file.h"
#include "uistrings.h"
#include <QImage>
#include <QColor>
#include <QColormap>

mUseQtnamespace

uiRGBArray::uiRGBArray( bool walpha )
    : qimg_(new QImage)
    , withalpha_(walpha)
{
}


uiRGBArray::uiRGBArray( const OD::RGBImage& image )
{
    mDynamicCastGet(const uiRGBArray*,input,&image);
    if ( !input )
    {
	pErrMsg( "Cannot copy image that's not uiRGBArray");
	qimg_ = new QImage;
	withalpha_ = true;
    }
    else
    {
	qimg_ = new QImage( input->qImage() );
	qimg_->detach();
	withalpha_ = input->withalpha_;
    }
}


uiRGBArray::uiRGBArray( const char* fnm )
    : qimg_(new QImage(fnm))
{
    withalpha_ = qimg_->hasAlphaChannel();
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
    qimg_ = new QImage( d0, d1,withalpha_ ? QImage::Format_ARGB32
					  : QImage::Format_RGB32 );

    return true;
}


bool uiRGBArray::reSize( int d0, int d1 )
{
    if ( qimg_->width() != d0 || qimg_->height() != d1 )
	*qimg_ = qimg_->scaled( d0, d1 );

    return qimg_->width() == d0 && qimg_->height() == d1;
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

    const QColor qcol( c.r(), c.g(), c.b(), 255-c.t() );
    qimg_->setPixel( i0, i1, withalpha_ ? qcol.rgba() : qcol.rgb() );
    return true;
}


void uiRGBArray::clear( const Color& c )
{
    const QColor qcol( c.r(), c.g(), c.b(), 255-c.t() );
    qimg_->fill( withalpha_ ? qcol.rgba() : qcol.rgb() );
}


int uiRGBArray::getSize( bool isx ) const
{
    return isx ? qimg_->width() : qimg_->height();
}


bool uiRGBArray::save(const char* fnm,const char* fmt,
		      int quality ) const
{
    return qimg_->save( fnm, fmt, quality );
}


unsigned char* uiRGBArray::getData()
{
    if ( !qimg_ || !bufferSize() )
	return 0;

    return qimg_->bits();
}


const unsigned char* uiRGBArray::getData() const
{
    if ( !qimg_ || !bufferSize() )
	return 0;

    return qimg_->bits();
}

// uiRGBImageLoader

void uiRGBImageLoader::initClass()
{
    OD::RGBImageLoader::setImageLoader( new uiRGBImageLoader );
}


uiRGBImageLoader::uiRGBImageLoader()
{
}


OD::RGBImage* uiRGBImageLoader::loadImage( const char* fnm,
					    uiString& errmsg )const
{
    if ( !File::exists(fnm) )
	{ errmsg = uiStrings::phrFileDoesNotExist( fnm ); return 0; }

    return new uiRGBArray( fnm );
}
