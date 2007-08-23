/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Aug 2007
 RCS:           $Id: uiflatviewthumbnail.cc,v 1.1 2007-08-23 15:27:31 cvsbert Exp $
 ________________________________________________________________________

-*/

#include "uiflatviewthumbnail.h"
#include "uiflatviewer.h"
#include "uiworld2ui.h"
#include "iodrawtool.h"

uiFlatViewThumbnail::uiFlatViewThumbnail( uiParent* p, uiFlatViewer& fv )
    	: uiCanvas(p,"Flatview thumbnail canvas")
	, viewer_(fv)
	, mousehandler_(getMouseEventHandler())
{
    setColors( Color::Black, Color::White );
    viewer_.viewChanged.notify( mCB(this,uiFlatViewThumbnail,vwChg) );
    mousehandler_.buttonReleased.notify(
	    		mCB(this,uiFlatViewThumbnail,mouseRelCB) );

    setPrefWidth( 50 ); setPrefHeight( 30 ); setStretch( 0, 0 );
}


void uiFlatViewThumbnail::setColors( Color fg, Color bg )
{
    fgcolor_ = fg; bgcolor_ = bg;
    setBackgroundColor( bgcolor_ );
}


void uiFlatViewThumbnail::reDrawHandler( uiRect updarea )
{
    ioDrawTool& dt = drawTool();
    dt.setBackgroundColor( bgcolor_ );
    dt.clear();

    const uiWorldRect bb = viewer_.boundingBox();
    const uiWorldRect wr = viewer_.curView();
    uiWorld2Ui w2u( bb, uiSize(width(),height()) );
    dt.setPenColor( Color::Black );
    dt.drawRect( w2u.transform(bb) );
    dt.setPenColor( fgcolor_ );
    dt.drawRect( w2u.transform(wr) );
}


void uiFlatViewThumbnail::vwChg( CallBacker* )
{
    update();
}


void uiFlatViewThumbnail::mouseRelCB( CallBacker* )
{
}
