/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiflatbitmapdisplay.h"

#include "datapackbase.h"
#include "flatposdata.h"
#include "flatviewbitmapmgr.h"
#include "flatviewbmp2rgb.h"
#include "pixmap.h"
#include "threadwork.h"
#include "threadlock.h"
#include "uigraphicsitemimpl.h"
#include "uirgbarray.h"
#include "thread.h"

namespace FlatView
{

class uiBitMapDisplayTask : public Task
{
public:
    uiBitMapDisplayTask( uiFlatViewer& viewer, 
			 uiDynamicImageItem* display, bool isdynamic )
	: image_( new uiRGBArray( false ) )
	, bitmap2image_( new BitMap2RGB( viewer.appearance(), *image_) )
	, display_( display )
	, isdynamic_( isdynamic )
	, viewer_( viewer )
	, wvabmpmgr_( 0 )
	, vdbmpmgr_( 0 )
    {}

    ~uiBitMapDisplayTask()
    {
	delete image_;
	delete bitmap2image_;
	delete wvabmpmgr_;
	wvabmpmgr_ = 0;
	delete vdbmpmgr_;
	vdbmpmgr_ = 0;
    }

    void setScope( const uiWorldRect& wr, const uiSize& sz )
    { wr_ = wr; sz_ = sz; }

    void reset()
    {
	Threads::Locker lckr( lock_ );
	if ( wvabmpmgr_ ) wvabmpmgr_->clearAll();
	if ( vdbmpmgr_ ) vdbmpmgr_->clearAll();
    }

    bool execute()
    {
	image_->setSize( sz_.width(), sz_.height() );

	Threads::Locker lckr( lock_ );

	if ( !wvabmpmgr_ )
	    wvabmpmgr_ = new FlatView::BitMapMgr( viewer_, true );

	if ( !vdbmpmgr_ )
	    vdbmpmgr_ = new FlatView::BitMapMgr( viewer_, false );

	FlatView::BitMapGenTask wvatask( *wvabmpmgr_, wr_, sz_, sz_ );
	FlatView::BitMapGenTask vdtask( *vdbmpmgr_, wr_, sz_, sz_ );

	TypeSet<Threads::Work> tasks;
	tasks += Threads::Work( wvatask, false );
	tasks += Threads::Work( vdtask, false );

	if ( !Threads::WorkManager::twm().addWork( tasks, 
		   Threads::WorkManager::cDefaultQueueID() ) )
	{
	     return false;
	}

	bitmap2image_->draw( wvabmpmgr_->bitMap(), vdbmpmgr_->bitMap(),
			     uiPoint(0,0), true ); 

	display_->setImage( isdynamic_, *image_, wr_ );
	return true;
    }

    Interval<float> getBitmapDataRange( bool iswva ) const
    {
	Interval<float> rg( mUdf(float), mUdf(float) );
	const ColTab::MapperSetup mapper =
	    iswva ? viewer_.appearance().ddpars_.wva_.mappersetup_
		  : viewer_.appearance().ddpars_.vd_.mappersetup_;

	Interval<float> mapperrange = mapper.range_;
	if ( mapper.type_ == ColTab::MapperSetup::Fixed )
	    return mapperrange;

	FlatView::BitMapMgr* mgr = iswva ? wvabmpmgr_ : vdbmpmgr_;
	if ( mgr && mgr->bitMapGen() )
	    rg = mgr->bitMapGen()->data().scale(
		    mapper.cliprate_, mapper.symmidval_ );

	return rg;
    }

    bool			isdynamic_;

    uiFlatViewer&		viewer_;
    BitMapMgr*			wvabmpmgr_;
    BitMapMgr*			vdbmpmgr_;
    uiRGBArray*			image_;
    BitMap2RGB*			bitmap2image_;
    uiSize			sz_;
    uiWorldRect			wr_;
    uiDynamicImageItem*		display_;
    Threads::Lock		lock_;
};



uiBitMapDisplay::uiBitMapDisplay( uiFlatViewer& viewer )
    : viewer_( viewer )
    , display_( new uiDynamicImageItem )
    , basetask_( new uiBitMapDisplayTask( viewer, display_, false ) )
    , finishedcb_( mCB( this, uiBitMapDisplay, dynamicTaskFinishCB ) )
    , overlap_( 0.5f )
{
    const int nrcpu = Threads::getNrProcessors();
    if ( nrcpu<4 )
	overlap_ = 0.25f;
    else if ( nrcpu<2 )
	overlap_ = 0.1f;

    display_->wantsData().notify( mCB( this, uiBitMapDisplay, reGenerateCB) );
    workqueueid_ = Threads::WorkManager::twm().addQueue( 
	    			Threads::WorkManager::SingleThread,
				"BitmapDisplay");
}


uiBitMapDisplay::~uiBitMapDisplay()
{
    Threads::WorkManager::twm().removeQueue( workqueueid_, false );

    delete basetask_;
    delete display_;
}



void uiBitMapDisplay::removeDisplay()
{
    if ( display_ )
	display_->wantsData().remove( mCB(this,uiBitMapDisplay,reGenerateCB) );

    display_ = 0;
}


void uiBitMapDisplay::update()
{
    basetask_->reset();

    if ( !viewer_.isVisible(true) && !viewer_.isVisible(false) )
    {
	display_->setVisible( false );
	return;
    }

    TypeSet<Threads::Work> updatework;

    PtrMan<Task> dynamictask = createDynamicTask();
    if ( dynamictask )
	updatework += Threads::Work( *dynamictask, false );

    uiWorldRect wr( viewer_.boundingBox() );
    wr.swapVer();
    const uiSize sz( viewer_.getDataPackRange(true).nrSteps()+1,
		     viewer_.getDataPackRange(false).nrSteps()+1 );
    basetask_->setScope( wr, sz );

    updatework += Threads::Work( *basetask_, false );

    if ( !Threads::WorkManager::twm().addWork( updatework ) )
	return;

    viewer_.appearance().ddpars_.wva_.mappersetup_.range_ = getDataRange( true);
    viewer_.appearance().ddpars_.vd_.mappersetup_.range_ = getDataRange( false);
    display_->setVisible( true );
}



uiGraphicsItem* uiBitMapDisplay::getDisplay()
{
    return display_;
}



Interval<float> uiBitMapDisplay::getDataRange( bool iswva ) const
{
    return basetask_->getBitmapDataRange( iswva );
}


void uiBitMapDisplay::reGenerateCB(CallBacker*)
{
    Task* dynamictask = createDynamicTask();

    if ( !dynamictask )
	return;

    Threads::WorkManager::twm().emptyQueue( workqueueid_, false );
    Threads::WorkManager::twm().addWork( 
	Threads::Work(*dynamictask,true), &finishedcb_, workqueueid_, true);
}


Task* uiBitMapDisplay::createDynamicTask()
{
    if ( !display_ )
	return 0;

    const uiWorldRect wr = display_->wantedWorldRect();
    const uiSize sz = display_->wantedScreenSize();
    if ( sz.width()<=0 || sz.height()<=0 )
	return 0;

    uiBitMapDisplayTask* dynamictask =
	new uiBitMapDisplayTask( viewer_, display_, true );

    const bool revx = wr.left() > wr.right();
    const bool revy = wr.bottom() > wr.top();

    const double expandx = wr.width()*overlap_ * (revx ? -1 : 1 );
    const double expandy = wr.height()*overlap_ * (revy ? 1 : -1 );

    uiWorldRect computewr( wr.left()-expandx,
			   wr.top()-expandy,
			   wr.right()+expandx,
			   wr.bottom()+expandy );
    computewr.limitTo( viewer_.boundingBox() );
    const uiSize computesz(
	    mNINT32(sz.width()/wr.width()*computewr.width()),
	    mNINT32(sz.height()/wr.height()*computewr.height()) );


    dynamictask->setScope( computewr, computesz );

    return dynamictask;
}


void uiBitMapDisplay::dynamicTaskFinishCB( CallBacker* )
{
}

} //namespace
