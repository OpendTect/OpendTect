/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uirgbarraycanvas.h"
#include "uirgbarray.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uipixmap.h"

uiRGBArrayCanvas::uiRGBArrayCanvas( uiParent* p, uiRGBArray& a )
    	: uiGraphicsView(p,"RGB Array view")
	, rgbarr_(a) 
	, bgcolor_(OD::Color::NoColor())
	, dodraw_(true)
	, pixmapitm_(0)
	, pixmap_(0)
{
    disableScrollZoom();

    setStretch( 2, 2 );
}


uiRGBArrayCanvas::~uiRGBArrayCanvas()
{
    delete pixmap_;
    removePixmap();
}


void uiRGBArrayCanvas::setBorder( const uiBorder& newborder )
{
    if ( border_ != newborder )
	border_ = newborder;
}


void uiRGBArrayCanvas::setBGColor( const OD::Color& c )
{
    if ( bgcolor_ != c )
    {
	bgcolor_ = c;
	setBackgroundColor( bgcolor_ );
    }
}


void uiRGBArrayCanvas::setDrawArr( bool yn )
{
    if ( dodraw_ != yn )
	dodraw_ = yn;
}


void uiRGBArrayCanvas::beforeDraw()
{
    beforeDraw( mNINT32(scene().width()), mNINT32(scene().height()) );
}


void uiRGBArrayCanvas::beforeDraw( int w, int h )
{
    const uiSize totsz( w, h ); 
    const int unusedpix = 0;
    arrarea_ = border_.getRect( totsz, unusedpix );
    const int xsz = arrarea_.width() + 1;
    const int ysz = arrarea_.height() + 1;
    if ( xsz < 1 || ysz < 1 )
    {
	rgbarr_.setSize( 1, 1 );
	pixmap_ = new uiPixmap( 1, 1 );
	return;
    }

    if ( rgbarr_.getSize(true) != xsz || rgbarr_.getSize(false) != ysz )
    {
	rgbarr_.setSize( xsz, ysz );
	delete pixmap_; pixmap_ = 0;
    }
}


void uiRGBArrayCanvas::setPixmap( const uiPixmap& pixmap )
{
    if ( pixmap_ )
	delete pixmap_;
    pixmap_ = new uiPixmap( pixmap );
}


void uiRGBArrayCanvas::removePixmap()
{
    delete pixmapitm_; pixmapitm_ = 0;
}


void uiRGBArrayCanvas::updatePixmap()
{
    if ( !dodraw_ )
	return;

    if ( !pixmap_ )
    {
	rgbarr_.clear( bgcolor_ );
	mkNewFill();
	if ( !createPixmap() )
	    return;
    }

    const uiRect pixrect( 0, 0, pixmap_->width()-1, pixmap_->height()-1 );
    if ( pixrect.right() < 1 && pixrect.bottom() < 1 )
	return;

    if ( !pixmapitm_ )
	pixmapitm_ = scene().addItem( new uiPixmapItem(*pixmap_) );
    else
	pixmapitm_->setPixmap( *pixmap_ );
    pixmapitm_->setOffset( arrarea_.left(), arrarea_.top() );
}


void uiRGBArrayCanvas::setPixMapPos( int x, int y )
{
    if ( pixmapitm_ )
	pixmapitm_->setOffset( x, y );
}


bool uiRGBArrayCanvas::createPixmap()
{
    const int xsz = rgbarr_.getSize( true );
    const int ysz = rgbarr_.getSize( false );
    delete pixmap_; pixmap_ = 0;
    if ( xsz < 1 || ysz < 1 )
	return false;

    pixmap_ = new uiPixmap( rgbarr_ );
    return true;
}
