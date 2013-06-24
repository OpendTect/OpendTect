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

namespace FlatView
{

class uiBitMapDisplayTask : public Task
{
public:
    uiBitMapDisplayTask( Viewer& viewer, 
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
	if ( wvabmpmgr_ ) wvabmpmgr_->setupChg();
	if ( vdbmpmgr_ ) vdbmpmgr_->setupChg();
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
	    rg = mgr->bitMapGen()->data().scale( mapper.cliprate_, mUdf(float));

	return rg;
    }




    bool			isdynamic_;

    Viewer&			viewer_;
    BitMapMgr*			wvabmpmgr_;
    BitMapMgr*			vdbmpmgr_;
    uiRGBArray*			image_;
    BitMap2RGB*			bitmap2image_;
    uiSize			sz_;
    uiWorldRect			wr_;
    uiDynamicImageItem*		display_;
    Threads::Lock		lock_;
};



uiBitMapDisplay::uiBitMapDisplay( Viewer& viewer )
    : viewer_( viewer )
    , extfac_(0.0f)
    , display_( new uiDynamicImageItem )
    , basetask_( new uiBitMapDisplayTask( viewer, display_, false ) )
    , finishedcb_( mCB( this, uiBitMapDisplay, dynamicTaskFinishCB ) )
{
    display_->wantsData().notify( mCB( this, uiBitMapDisplay, reGenerateCB) );
    workqueueid_ = Threads::WorkManager::twm().addQueue( 
	    			Threads::WorkManager::MultiThread );
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
    display_->clearImages( true );
    basetask_->reset();

    if ( !viewer_.isVisible(true) && !viewer_.isVisible(false) )
    {
	display_->setVisible( false );
	return;
    }

    StepInterval<double> xrg, yrg;

    if ( viewer_.isVisible(true) )
    {
	xrg = viewer_.pack(true)->posData().range(true);
	yrg = viewer_.pack(true)->posData().range(false);

	if ( viewer_.isVisible( false ) )
	{
	    const StepInterval<double> vdxrg =
		viewer_.pack(false)->posData().range(true);
	    const StepInterval<double> vdyrg =
		viewer_.pack(false)->posData().range(false);

	    xrg.include( vdxrg ); xrg.step = mMIN(vdxrg.step, xrg.step );
	    yrg.include( vdyrg ); yrg.step = mMIN(vdyrg.step, yrg.step );
	}
    }
    else
    {
	xrg = viewer_.pack(false)->posData().range(true);
	yrg = viewer_.pack(false)->posData().range(false);
	yrg.widen( extfac_ * yrg.step, true );
    }

    xrg.widen( extfac_ * xrg.step, true );
    const uiWorldRect wr( xrg.start, yrg.start, xrg.stop, yrg.stop );
    const uiSize sz( viewrect_.width(), viewrect_.height() );

    basetask_->setScope( wr, sz );
    if ( !basetask_->execute() )
	return;

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
    const uiWorldRect wr = display_->wantedWorldRect();
    const uiSize sz = display_->wantedScreenSize();
    if ( sz.width()<=0 || sz.height()<=0 ) return;

    uiBitMapDisplayTask* dynamictask =
	new uiBitMapDisplayTask( viewer_, display_, true );

    dynamictask->setScope( wr, sz );

    Threads::WorkManager::twm().emptyQueue( workqueueid_, false );
    Threads::WorkManager::twm().addWork( 
	Threads::Work(*dynamictask,true), &finishedcb_, workqueueid_, true);
}


void uiBitMapDisplay::dynamicTaskFinishCB( CallBacker* )
{
}

} //namespace
