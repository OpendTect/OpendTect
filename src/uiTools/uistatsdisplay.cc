/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Mid 2005
 RCS:           $Id: uistatsdisplay.cc,v 1.1 2008-03-26 13:21:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"

#include "uicanvas.h"
#include "uihistogramdisplay.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uistatusbar.h"

#include "arraynd.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "samplingdata.h"
#include "statruncalc.h"
#include "errh.h"

static const int sCanvasHeight = 250;
static const int sCanvasWidth = 400;
static const int cBoundarySz = 20;

uiStatsDisplay::uiStatsDisplay( uiParent* p, bool withplot, bool withtext )
    : uiGroup( p, "Statistics display group" )
    , canvas_(0)
    , countfld_(0)
{
    if ( withplot )
    {
	canvas_ = new uiCanvas( this );
	canvas_->setPrefHeight( sCanvasHeight );
	canvas_->setPrefWidth( sCanvasWidth );
	canvas_->setStretch( 2, 2 );
	histogramdisplay_ = new uiHistogramDisplay( canvas_ );
	histogramdisplay_->setColor( Color(200,100,65,5) );
	canvas_->postDraw.notify( mCB(this,uiStatsDisplay,reDraw) );
    }

    uiSeparator* sep = 0;
    if ( withplot && withtext )
    {
	sep = new uiSeparator( this, "Hor sep" );
	sep->attach( stretchedBelow, canvas_ );
    }

    if ( withtext )
    {
	uiGroup* valgrp = new uiGroup( this, "Values group" );
	countfld_ = new uiGenInput( valgrp, "Number of values" );
	countfld_->setReadOnly();
	minmaxfld_ = new uiGenInput( valgrp, "Value range",
				     FloatInpSpec(), FloatInpSpec() );
	minmaxfld_->attach( alignedBelow, countfld_ );
	minmaxfld_->setReadOnly();
	avgstdfld_ = new uiGenInput( valgrp, "Mean/Std Deviation",
				     DoubleInpSpec(), DoubleInpSpec() );
	avgstdfld_->attach( alignedBelow, minmaxfld_ );	
	avgstdfld_->setReadOnly();
	medrmsfld_ = new uiGenInput( valgrp, "Median/RMS",
				     FloatInpSpec(), DoubleInpSpec() );
	medrmsfld_->attach( alignedBelow, avgstdfld_ );	
	medrmsfld_->setReadOnly();

	if ( sep )
	{
	    valgrp->attach( centeredBelow, canvas_ );
	    valgrp->attach( ensureBelow, sep );
	}
    }
}


bool uiStatsDisplay::setDataPackID( DataPack::ID dpid, DataPackMgr::ID dmid )
{
    DataPackMgr& dpman = DPM( dmid );
    const DataPack* datapack = dpman.obtain( dpid );
    if ( !datapack ) return false;

    if ( dmid == DataPackMgr::CubeID )
    {
	mDynamicCastGet(const ::CubeDataPack*,cdp,datapack);
	const Array3D<float>* arr3d = cdp ? &cdp->data() : 0;
	if ( !arr3d ) return false;
	setData( arr3d->getData(), arr3d->info().getTotalSz() );
    }
    else if ( dmid == DataPackMgr::FlatID )
    {
	mDynamicCastGet(const FlatDataPack*,fdp,datapack);
	if ( !fdp ) return false;
	setData( &fdp->data() );
    }
    else
	return false;

    dpman.release( dpid );
    return true;
}


void uiStatsDisplay::setInfo( const Stats::RunCalc<float>& rc )
{
    if ( !countfld_ ) return;
    countfld_->setValue( rc.count() );
    minmaxfld_->setValue( rc.min(), 0 );
    minmaxfld_->setValue( rc.max(), 1 );
    avgstdfld_->setValue( rc.average(), 0 );
    avgstdfld_->setValue( rc.stdDev(), 1 );
    medrmsfld_->setValue( rc.median(), 0 );
    medrmsfld_->setValue( rc.rms(), 1 );
}


static int getNrIntervals( int nrpts )
{
    int res = nrpts / 25;
    if ( res < 10 ) res = 10;
    else if ( res < 20 ) res = 20;
    else if ( res < 50 ) res = 50;
    else res = 100;
    return res;
}


#define mDeclRC \
    Stats::RunCalc<float> rc( Stats::RunCalcSetup() \
	    			.require(Stats::Min) \
				.require(Stats::Max) \
				.require(Stats::Average) \
				.require(Stats::Median) \
				.require(Stats::StdDev) \
				.require(Stats::RMS) )

void uiStatsDisplay::setData( const Array2D<float>* array )
{
    if ( !array ) return;

    mDeclRC;
    const int sz2d0 = array->info().getSize( 0 );
    const int sz2d1 = array->info().getSize( 1 );
    for ( int idx0=0; idx0<sz2d0; idx0++ )
    {
	for ( int idx1=0; idx1<sz2d1; idx1++ )
	{
	     const float val = array->get( idx0, idx1 );
	     if ( mIsUdf(val) ) continue ;
	     rc.addValue( array->get( idx0, idx1 ) );
	 }
    }

    setData( rc );
}


void uiStatsDisplay::setData( const float* array, int sz )
{	
    if ( !array ) return;

    mDeclRC;
    for ( int idx=0; idx<sz; idx++ )
    {
	const float val = array[idx];
	if ( mIsUdf(val) ) continue ;
	rc.addValue( array[idx] );
    }

    setData( rc );
}


void uiStatsDisplay::setData( const Stats::RunCalc<float>& rc )
{
    const int nrpts = rc.count();
    const int nrintv = getNrIntervals( nrpts );
    TypeSet<float> histogram( nrintv, 0 );
    const float min = rc.min(); const float max = rc.max();
    const float factor = nrintv / (max-min);
    for ( int idx=0; idx<nrpts; idx++ )
    {
	const int seg = (int)(factor * (rc.vals_[idx] - min));
	if ( seg < 0 || seg > nrintv )
	    { pErrMsg("Huh"); continue; }
	histogram[seg]++;
    }

    const float step = nrpts / ((float)nrintv);
    xrg_ = StepInterval<float>( rc.min(), rc.max(), step );
    setHistogram( histogram );
    setInfo( rc );
}


void uiStatsDisplay::setHistogram( const TypeSet<float>& histogram )
{
    if ( !canvas_ ) return;

    float maxhist = 0;
    for ( int idx=0; idx<histogram.size(); idx++ )
    {
	if ( maxhist < histogram[idx] )
	maxhist = histogram[idx];
    }

    SamplingData<float> sd( 0, 1 );
    histogramdisplay_->setHistogram( histogram, sd );
    histogramdisplay_->setBoundaryRect(
	    uiRect(cBoundarySz,0,cBoundarySz,cBoundarySz) );
    histogramdisplay_->setXAxis( xrg_ );

    canvas_->update();
}


void uiStatsDisplay::reDraw( CallBacker* cb )
{
    ioDrawTool& drawtool = canvas_->drawTool();
    drawtool.setBackgroundColor( Color::White );
    drawtool.clear();
    histogramdisplay_->reDraw( cb );
}


uiStatsDisplayWin::uiStatsDisplayWin( uiParent* p,
					const uiStatsDisplayWin::Setup& su )
    : uiMainWin(p,"Data statistics",-1,false,su.modal_)
    , disp_(*new uiStatsDisplay(this,su.withplot_,su.withtext_))
{
    statusBar()->addMsgFld( "Data name", uiStatusBar::Left, 2 );
    statusBar()->addMsgFld( "Ref name", uiStatusBar::Right, 2 );
}


void uiStatsDisplayWin::setData( const Stats::RunCalc<float>& rc )
{
    disp_.setData( rc );
}


void uiStatsDisplayWin::setDataName( const char* nm )
{
    BufferString txt( nm );
    char* ptr = strchr( txt, '\n' );
    if ( ptr ) *ptr++ = '\0';
    else	ptr = "";
    statusBar()->setLabelTxt( 0, txt );
    statusBar()->setLabelTxt( 1, ptr );
}
