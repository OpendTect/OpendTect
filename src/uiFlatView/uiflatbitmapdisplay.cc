/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatbitmapdisplay.cc,v 1.3 2012-04-06 13:03:31 cvskris Exp $";

#include "uiflatbitmapdisplay.h"

#include "flatviewbitmapmgr.h"
#include "datapackbase.h"
#include "flatposdata.h"
#include "flatviewbmp2rgb.h"
#include "pixmap.h"
#include "threadwork.h"
#include "uigraphicsitemimpl.h"
#include "uirgbarray.h"

namespace FlatView
{

class uiBitMapDisplayTask : public Task
{
public:
    uiBitMapDisplayTask( Viewer& viewer, bool wva,
			 uiDynamicImageItem* display, bool isdynamic )
	: image_( new uiRGBArray( true ) )
	, bitmap2image_( new BitMap2RGB( viewer.appearance(), *image_) )
	, display_( display )
	, isdynamic_( isdynamic )
	, wva_( wva )
	, viewer_( viewer )
	, bmpmgr_( 0 )
    {}

    ~uiBitMapDisplayTask()
    { delete image_; delete bitmap2image_; }

    void setScope( const uiWorldRect& wr, const uiSize& sz )
    { wr_ = wr; sz_ = sz; }

    void reset()
    {
	delete bmpmgr_;
	bmpmgr_ = 0;
    }

    bool execute()
    {
	image_->setSize( sz_.width(), sz_.height() );

	if ( !bmpmgr_ )
	    bmpmgr_ = new FlatView::BitMapMgr( viewer_, wva_ );

	if ( !bmpmgr_->generate( wr_, sz_, sz_ ) )
	    return false;

	bitmap2image_->draw( wva_ ? bmpmgr_->bitMap() : 0,
			     wva_ ? 0 : bmpmgr_->bitMap(), uiPoint(0,0) ); 
	display_->setImage( isdynamic_, *image_, wr_ );
	return true;
    }

    bool			wva_;
    bool			isdynamic_;

    Viewer&			viewer_;
    BitMapMgr*			bmpmgr_;
    uiRGBArray*			image_;
    BitMap2RGB*			bitmap2image_;
    uiSize			sz_;
    uiWorldRect			wr_;
    uiDynamicImageItem*		display_;
};



uiBitMapDisplay::uiBitMapDisplay( Viewer& viewer, bool wva )
    : viewer_( viewer )
    , wva_( wva )
    , display_( new uiDynamicImageItem )
    , basetask_( new uiBitMapDisplayTask( viewer, wva, display_, false ) )
    , dynamictask_( 0 )
    , isworking_( false )
    , finishedcb_( mCB( this, uiBitMapDisplay, dymamicTaskFinishCB ) )
{
    display_->wantsData().notify( mCB( this, uiBitMapDisplay, reGenerateCB) );
}


uiBitMapDisplay::~uiBitMapDisplay()
{
    if ( isworking_ )
	pErrMsg("Deleting with active job" );

    delete basetask_;
    delete display_;
}



void uiBitMapDisplay::removeDisplay()
{
    if ( display_ )
	display_->wantsData().remove( mCB( this, uiBitMapDisplay, reGenerateCB) );

    display_ = 0;
}


void uiBitMapDisplay::update()
{
    basetask_->reset();

    if ( !viewer_.pack(wva_) )
    {
	//display_->setVisible( false );
	return;
    }

    const uiSize sz( viewer_.pack(wva_)->data().info().getSize(0),
		     viewer_.pack(wva_)->data().info().getSize(1) );

    const Interval<double> xrg =viewer_.pack(wva_)->posData().range(true);
    const Interval<double> yrg =viewer_.pack(wva_)->posData().range(false);

    const uiWorldRect wr( xrg.start, yrg.start, xrg.stop, yrg.stop );

    basetask_->setScope( wr, sz );
    if ( !basetask_->execute() )
	return;

    //display_->setVisible( true );
}



uiGraphicsItem* uiBitMapDisplay::getDisplay()
{
    return display_;
}


void uiBitMapDisplay::reGenerateCB(CallBacker*)
{
    if ( isworking_ )
	return;

    isworking_ = true;

    const uiWorldRect wr = display_->wantedWorldRect();
    const uiSize sz = display_->wantedScreenSize();

    if ( !dynamictask_ )
	dynamictask_ = new uiBitMapDisplayTask( viewer_, wva_, display_, true );

    dynamictask_->setScope( wr, sz );

    //TODO: Send to queue
    Threads::WorkManager::twm().addWork( Threads::Work(*dynamictask_,false), &finishedcb_,
		     Threads::WorkManager::cDefaultQueueID(), true );
}


void uiBitMapDisplay::dymamicTaskFinishCB( CallBacker* )
{
    isworking_ = false;
}


} //namespace
