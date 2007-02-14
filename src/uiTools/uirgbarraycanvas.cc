/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uirgbarraycanvas.cc,v 1.2 2007-02-14 10:14:25 cvsbert Exp $
 ________________________________________________________________________

-*/

#include "uirgbarraycanvas.h"
#include "uirgbarray.h"
#include "iodrawtool.h"
#include "pixmap.h"

uiRGBArrayCanvas::uiRGBArrayCanvas( uiParent* p, uiRGBArray& a )
    	: uiCanvas(p)
	, rgbarr_(a)
	, newFillNeeded(this)
	, border_(0,0,0,0)
	, bgcolor_(Color::NoColor)
	, dodraw_(true)
	, pixmap_(0)
{
    setBGColor( Color::White );
    preDraw.notify( mCB(this,uiRGBArrayCanvas,beforeDraw) );
}


void uiRGBArrayCanvas::setBorders( const uiSize& tl, const uiSize& br )
{
    uiRect newborder;
    newborder.setLeft( tl.width() );
    newborder.setTop( tl.height() );
    newborder.setRight( br.width() );
    newborder.setLeft( br.height() );
    if ( border_ != newborder )
    {
	border_ = newborder;
	setupChg();
    }
}


void uiRGBArrayCanvas::setBGColor( const Color& c )
{
    if ( bgcolor_ != c )
    {
	bgcolor_ = c;
	setBackgroundColor( bgcolor_ );
	setupChg();
    }
}


void uiRGBArrayCanvas::setDrawArr( bool yn )
{
    if ( dodraw_ != yn )
    {
	dodraw_ = yn;
	setupChg();
    }
}


void uiRGBArrayCanvas::forceNewFill()
{
    delete pixmap_; pixmap_ = 0;
}


void uiRGBArrayCanvas::setupChg()
{
    update(); // Force redraw
}


void uiRGBArrayCanvas::beforeDraw( CallBacker* )
{
    const uiSize totsz( width(), height() );
    arrarea_.setLeft( border_.left() );
    arrarea_.setTop( border_.top() );
    arrarea_.setRight( totsz.width() - border_.right() - 1 );
    arrarea_.setBottom( totsz.height() - border_.bottom() - 1 );

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
	forceNewFill();
    }
}


void uiRGBArrayCanvas::reDrawHandler( uiRect updarea )
{
    if ( !dodraw_ )
	return;

    if ( !pixmap_ )
    {
	rgbarr_.clear( bgcolor_ );
	newFillNeeded.trigger();
	mkNewFill();
	if ( !createPixmap() )
	    return;
    }

    if ( pixmap_->width() < 2 && pixmap_->height() < 2 )
	return;

    updarea_ = updarea;
    uiRect updpart( updarea_ );
    updpart -= border_.topLeft();
    if ( updpart.bottom() < 0 || updpart.right() < 0 )
	return;
    if ( updpart.left() < 0 ) updpart.setLeft( 0 );
    if ( updpart.top() < 0 ) updpart.setTop( 0 );

    drawTool()->drawPixmap( arrarea_.topLeft(), pixmap_, updpart );
    drawTool()->setBackgroundColor( bgcolor_ );
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
