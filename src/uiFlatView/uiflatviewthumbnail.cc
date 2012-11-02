/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2007
 ________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uirgbarraycanvas.h"
#include "uiflatviewthumbnail.h"
#include "uiflatviewcontrol.h"
#include "uiflatviewer.h"
#include "uiworld2ui.h"

uiFlatViewThumbnail::uiFlatViewThumbnail( uiParent* p, uiFlatViewer& fv )
    	: uiGraphicsView(p,"Flatview thumbnail canvas")
	, viewer_(fv)
	, bgrectitem_(0)
	, fgrectitem_(0)
	, mousehandler_(getMouseEventHandler())
	, feedbackwr_( 0 )
{
    setColors( Color(0,0,200), Color(255,255,200) );
    viewer_.viewChanged.notify( mCB(this,uiFlatViewThumbnail,vwChg) );
    mousehandler_.buttonReleased.notify(
	    		mCB(this,uiFlatViewThumbnail,mouseRelCB) );
    mousehandler_.buttonPressed.notify(
	    		mCB(this,uiFlatViewThumbnail,mousePressCB) );
    mousehandler_.movement.notify(
	    		mCB(this,uiFlatViewThumbnail,mouseMoveCB) );

    setPrefWidth( 45 ); setPrefHeight( 30 ); setStretch( 0, 0 );
}


uiFlatViewThumbnail::~uiFlatViewThumbnail()
{
    delete feedbackwr_;
    mousehandler_.buttonReleased.remove(
	    		mCB(this,uiFlatViewThumbnail,mouseRelCB) );
    mousehandler_.buttonPressed.remove(
	    		mCB(this,uiFlatViewThumbnail,mousePressCB) );
    mousehandler_.movement.remove(
	    		mCB(this,uiFlatViewThumbnail,mouseMoveCB) );

    delete bgrectitem_;
    delete fgrectitem_;
}


void uiFlatViewThumbnail::setColors( Color fg, Color bg )
{
    fgcolor_ = fg; bgcolor_ = bg;
    setBackgroundColor( bgcolor_ );
}


#define mDeclW2UVars( wrsource ) \
    uiWorldRect br = viewer_.boundingBox(); \
    const uiWorldRect wr = wrsource; \
    if ( wr.left() > wr.right() ) \
	{ double tmp = br.left(); br.setLeft(br.right()); br.setRight(tmp); } \
    if ( wr.bottom() > wr.top() ) \
	{ double tmp = br.top(); br.setTop(br.bottom()); br.setBottom(tmp); } \
    uiWorld2Ui w2u( br, uiSize(width(),height()) );

void uiFlatViewThumbnail::draw()
{
    draw( viewer_.curView() );
}


void uiFlatViewThumbnail::draw( const uiWorldRect& viewarea )
{
    mDeclW2UVars( viewarea );
    const uiRect uibr( w2u.transform(br) );
    if ( !bgrectitem_ )
	bgrectitem_ = scene().addRect( mCast(float,uibr.left()), 
				       mCast(float,uibr.top()), 
				       mCast(float,uibr.width()),
				       mCast(float,uibr.height()) );
    else
	bgrectitem_->setRect( uibr.left(),uibr.top(),uibr.width(),uibr.height() );

    uiRect uiwr;
    getUiRect( feedbackwr_ ? *feedbackwr_ : wr , uiwr );
    if ( !fgrectitem_ )
	fgrectitem_ = scene().addRect( mCast(float,uiwr.left()), 
				       mCast(float,uiwr.top()), 
				       mCast(float,uiwr.width()),
				       mCast(float,uiwr.height()) );
    else
	fgrectitem_->setRect( uiwr.left(), uiwr.top(),
			      uiwr.width(), uiwr.height() );
    fgrectitem_->setPenColor( fgcolor_ );
    fgrectitem_->setZValue( 2 );
}


void uiFlatViewThumbnail::getUiRect( const uiWorldRect& inputwr,
				     uiRect& uiwr ) const
{
    mDeclW2UVars( inputwr );

    uiwr = w2u.transform(wr);
    uiSize sz( uiwr.size() );

    const uiRect uibr( w2u.transform(br) );

    if ( uiwr.width() < 2 || uiwr.height() < 2 )
    {
	const int toadd = 2 - mMIN(sz.width(),sz.height());
	int addl = 1; int addr = toadd - 1;
	if ( uiwr.left() < 1 )
	    { addl = 0; addr = toadd; }
	else if ( uiwr.right() >= uibr.right() )
	    { addl = toadd; addr = 0; }
	uiwr.setLeft( uiwr.left() - addl );
	uiwr.setRight( uiwr.right() + addr );
	int addt = 1; int addb = toadd - 1;
	if ( uiwr.top() < 1 )
	    { addt = 0; addb = toadd; }
	else if ( uiwr.bottom() >= uibr.bottom() )
	    { addt = toadd; addb = 0; }
	uiwr.setTop( uiwr.top() - addt );
	uiwr.setBottom( uiwr.bottom() + addb );
    }
}


void uiFlatViewThumbnail::vwChg( CallBacker* )
{ draw(); }


void uiFlatViewThumbnail::vwChging( CallBacker* cb )
{
    mCBCapsuleUnpack(uiWorldRect,viewarea,cb);
    draw( viewarea );
}


void uiFlatViewThumbnail::mousePressCB( CallBacker* )
{
    if ( !mousehandler_.hasEvent() ||
	 mousehandler_.isHandled() || !viewer_.control() ) return;

    const uiWorldRect wr = viewer_.curView();

    if ( feedbackwr_ ) *feedbackwr_ = wr;
    else feedbackwr_ = new uiWorldRect( wr );

    mousehandler_.setHandled( true );
}



void uiFlatViewThumbnail::mouseMoveCB( CallBacker* )
{
    if ( !mousehandler_.hasEvent() || !feedbackwr_ ||
	 mousehandler_.isHandled() || !viewer_.control() ) return;

    mDeclW2UVars( viewer_.curView() );
    const MouseEvent& ev = mousehandler_.event();
    Geom::Point2D<double> wpt = w2u.transform( ev.pos() );
    Geom::Size2D<double> wsz = wr.size();
    *feedbackwr_ = viewer_.control()->getNewWorldRect(
	    wpt, wsz, viewer_.curView(), br );

    mousehandler_.setHandled( true );
    draw();
}


void uiFlatViewThumbnail::mouseRelCB( CallBacker* )
{
    if ( !mousehandler_.hasEvent() ||
	 mousehandler_.isHandled() || !viewer_.control() ) return;

    delete feedbackwr_; feedbackwr_ = 0;

    mDeclW2UVars( viewer_.curView() );
    const MouseEvent& ev = mousehandler_.event();
    Geom::Point2D<double> wpt = w2u.transform( ev.pos() );
    Geom::Size2D<double> wsz = wr.size();
    viewer_.control()->setNewView( wpt, wsz );
    mousehandler_.setHandled( true );
}
