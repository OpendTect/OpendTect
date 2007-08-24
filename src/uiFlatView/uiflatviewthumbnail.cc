/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Aug 2007
 RCS:           $Id: uiflatviewthumbnail.cc,v 1.3 2007-08-24 11:25:37 cvsbert Exp $
 ________________________________________________________________________

-*/

#include "uiflatviewthumbnail.h"
#include "uiflatviewcontrol.h"
#include "uiflatviewer.h"
#include "uiworld2ui.h"
#include "iodrawtool.h"

uiFlatViewThumbnail::uiFlatViewThumbnail( uiParent* p, uiFlatViewer& fv )
    	: uiCanvas(p,"Flatview thumbnail canvas")
	, viewer_(fv)
	, mousehandler_(getMouseEventHandler())
{
    setColors( Color(0,0,200), Color(255,255,200) );
    viewer_.viewChanged.notify( mCB(this,uiFlatViewThumbnail,vwChg) );
    mousehandler_.buttonReleased.notify(
	    		mCB(this,uiFlatViewThumbnail,mouseRelCB) );

    setPrefWidth( 45 ); setPrefHeight( 30 ); setStretch( 0, 0 );
}


void uiFlatViewThumbnail::setColors( Color fg, Color bg )
{
    fgcolor_ = fg; bgcolor_ = bg;
    setBackgroundColor( bgcolor_ );
}


#define mDeclW2UVars \
    uiWorldRect br = viewer_.boundingBox(); \
    const uiWorldRect wr = viewer_.curView(); \
    if ( wr.left() > wr.right() ) \
	{ double tmp = br.left(); br.setLeft(br.right()); br.setRight(tmp); } \
    if ( wr.bottom() > wr.top() ) \
	{ double tmp = br.top(); br.setTop(br.bottom()); br.setBottom(tmp); } \
    uiWorld2Ui w2u( br, uiSize(width(),height()) )

void uiFlatViewThumbnail::reDrawHandler( uiRect updarea )
{
    ioDrawTool& dt = drawTool();
    dt.setBackgroundColor( bgcolor_ );
    dt.clear();

    mDeclW2UVars;
    dt.setPenColor( Color::Black );
    dt.drawRect( w2u.transform(br) );
    dt.setPenColor( fgcolor_ );
    dt.drawRect( w2u.transform(wr) );
}


void uiFlatViewThumbnail::vwChg( CallBacker* )
{
    update();
}


void uiFlatViewThumbnail::mouseRelCB( CallBacker* )
{
    if ( !mousehandler_.hasEvent() || !viewer_.control() ) return;

    mDeclW2UVars;
    const MouseEvent& ev = mousehandler_.event();
    Geom::Point2D<double> wpt = w2u.transform( ev.pos() );
    Geom::Size2D<double> wsz = wr.size();
    viewer_.control()->setNewView( wpt, wsz );
}
