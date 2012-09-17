/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: uirgbarraycanvas.cc,v 1.23 2010/10/28 07:28:36 cvsbert Exp $";

#include "uirgbarraycanvas.h"
#include "uirgbarray.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "iodrawtool.h"
#include "pixmap.h"

uiRGBArrayCanvas::uiRGBArrayCanvas( uiParent* p, uiRGBArray& a )
    	: uiGraphicsView(p,"RGB Array view")
	, rgbarr_(a) 
	, bgcolor_(Color::NoColor()) 
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


void uiRGBArrayCanvas::setBGColor( const Color& c )
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
    beforeDraw( width(), height() );
}


void uiRGBArrayCanvas::beforeDraw( int w, int h )
{
    const uiSize totsz( w, h ); 
    const int unusedpix = 1;
    arrarea_ = border_.getRect( totsz, unusedpix );
    const int xsz = arrarea_.width() + 1;
    const int ysz = arrarea_.height() + 1;
    if ( xsz < 1 || ysz < 1 )
    {
	rgbarr_.setSize( 1, 1 );
	pixmap_ = new ioPixmap( 1, 1 );
	return;
    }

    if ( rgbarr_.getSize(true) != xsz || rgbarr_.getSize(false) != ysz )
    {
	rgbarr_.setSize( xsz, ysz );
	delete pixmap_; pixmap_ = 0;
    }
}


void uiRGBArrayCanvas::setPixmap( const ioPixmap& pixmap )
{
    if ( pixmap_ )
	delete pixmap_;
    pixmap_ = new ioPixmap( pixmap );
}


void uiRGBArrayCanvas::removePixmap()
{
    delete pixmapitm_; pixmapitm_ = 0;
}


void uiRGBArrayCanvas::draw()
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

    pixmap_ = new ioPixmap( rgbarr_ );
    return true;
}
