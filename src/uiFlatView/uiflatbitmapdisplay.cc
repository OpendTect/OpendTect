/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatbitmapdisplay.cc,v 1.1 2012-04-05 12:09:48 cvskris Exp $";

#include "uiflatbitmapdisplay.h"

#include "flatviewbitmapmgr.h"
#include "datapackbase.h"
#include "flatposdata.h"
#include "flatviewbmp2rgb.h"
#include "pixmap.h"
#include "uiflatviewer.h"
#include "uigraphicsitemimpl.h"
#include "uirgbarray.h"

namespace FlatView
{


uiBitMapDisplay::uiBitMapDisplay( uiFlatViewer& viewer )
    : viewer_( viewer )
    , display_( new uiDynamicPixmapItem )
    , baseimage_( new uiRGBArray( false ) )
    , basebitmap2baseimage_( new BitMap2RGB( viewer.appearance(), *baseimage_) )
    , wvabmpmgr_( 0 )
    , vdbmpmgr_( 0 )
{}


uiBitMapDisplay::~uiBitMapDisplay()
{
    delete baseimage_;
    delete basebitmap2baseimage_;
    delete wvabmpmgr_;
    delete vdbmpmgr_;
}


void uiBitMapDisplay::update()
{
    delete wvabmpmgr_;
    delete vdbmpmgr_;
    wvabmpmgr_ = 0;
    vdbmpmgr_ = 0;

    if ( viewer_.pack(false) )
    {
	const uiSize sz( viewer_.pack(false)->data().info().getSize(0),
		         viewer_.pack(false)->data().info().getSize(1) );

	const Interval<double> xrg = viewer_.pack(false)->posData().range(true);
	const Interval<double> yrg = viewer_.pack(false)->posData().range(false);

	const uiWorldRect wr( xrg.start, yrg.start, xrg.stop, yrg.stop );

	vdbmpmgr_ = new FlatView::BitMapMgr( viewer_, false );
	if ( !vdbmpmgr_->generate( wr, sz, sz ) )
	    return;

	basebitmap2baseimage_->draw( 0, vdbmpmgr_->bitMap(), uiPoint(0,0) ); 
	//ioPixmap pixmap( *baseimage_ );
	ioPixmap pixmap( "/Users/Shared/work/od/data/od.png" );

	display_->setBasePixmap( pixmap, wr );
    }
}



uiGraphicsItem* uiBitMapDisplay::getDisplay()
{
    return display_;
}


void uiBitMapDisplay::reGenerateCB(CallBacker*)
{
}




} //namespace
