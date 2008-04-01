/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uistatsdisplay.cc,v 1.7 2008-04-01 09:27:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"

#include "uicanvas.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uistatusbar.h"
#include "uiaxishandler.h"

#include "arraynd.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "samplingdata.h"
#include "statruncalc.h"
#include "linear.h"
#include "errh.h"

static const int cCanvasHeight = 250;
static const int cCanvasWidth = 400;
static const int cBoundarySz = 10;
static const Color cHistColor( 0, 200, 0 );
static const Color cMarkColor( 230, 0, 0 );

uiStatsDisplay::uiStatsDisplay( uiParent* p, const uiStatsDisplay::Setup& su )
    : uiGroup( p, "Statistics display group" )
    , setup_(su)
    , canvas_(0)
    , countfld_(0)
    , xax_(0)
    , yax_(0)
    , histmaxidx_(0)
    , histcount_(0)
    , markval_(mUdf(float))
{
    if ( setup_.withplot_ )
    {
	canvas_ = new uiCanvas( this );
	canvas_->setPrefHeight( cCanvasHeight );
	canvas_->setPrefWidth( cCanvasWidth );
	canvas_->setStretch( 2, 2 );
	canvas_->postDraw.notify( mCB(this,uiStatsDisplay,reDraw) );
	uiAxisHandler::Setup asu( uiRect::Bottom );
	asu.border_ = uiBorder( 10 );
	xax_ = new uiAxisHandler( canvas_->drawTool(), asu );
	asu.side( uiRect::Left ).noannot( true );
	yax_ = new uiAxisHandler( canvas_->drawTool(), asu );
	xax_->setBegin( yax_ ); yax_->setBegin( xax_ );
    }

    uiSeparator* sep = 0;
    if ( setup_.withplot_ && setup_.withtext_ )
    {
	sep = new uiSeparator( this, "Hor sep" );
	sep->attach( stretchedBelow, canvas_ );
    }

    if ( setup_.withtext_ )
    {
	uiGroup* valgrp = new uiGroup( this, "Values group" );
	minmaxfld_ = new uiGenInput( valgrp, "Value range",
				     FloatInpSpec(), FloatInpSpec() );
	minmaxfld_->setReadOnly();
	avgstdfld_ = new uiGenInput( valgrp, "Mean/Std Deviation",
				     DoubleInpSpec(), DoubleInpSpec() );
	avgstdfld_->attach( alignedBelow, minmaxfld_ );	
	avgstdfld_->setReadOnly();
	medrmsfld_ = new uiGenInput( valgrp, "Median/RMS",
				     FloatInpSpec(), DoubleInpSpec() );
	medrmsfld_->attach( alignedBelow, avgstdfld_ );	
	medrmsfld_->setReadOnly();
	if ( !canvas_ || !setup_.countinplot_ )
	{
	    countfld_ = new uiGenInput( valgrp, "Number of values" );
	    countfld_->setReadOnly();
	    countfld_->attach( alignedBelow, medrmsfld_ );	
	}

	if ( sep )
	{
	    valgrp->attach( centeredBelow, canvas_ );
	    valgrp->attach( ensureBelow, sep );
	}
    }
}


uiStatsDisplay::~uiStatsDisplay()
{
    delete xax_;
    delete yax_;
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
    updateHistogram( rc );

    if ( !minmaxfld_ ) return;
    if ( countfld_ )
	countfld_->setValue( rc.count() );
    minmaxfld_->setValue( rc.min(), 0 );
    minmaxfld_->setValue( rc.max(), 1 );
    avgstdfld_->setValue( rc.average(), 0 );
    avgstdfld_->setValue( rc.stdDev(), 1 );
    medrmsfld_->setValue( rc.median(), 0 );
    medrmsfld_->setValue( rc.rms(), 1 );
}


void uiStatsDisplay::updateHistogram( const Stats::RunCalc<float>& rc )
{
    const int nrpts = rc.count();
    const int nrintv = getNrIntervals( nrpts );
    histdata_.erase(); histdata_.setSize( nrintv, 0 );
    const float min = rc.min(); const float max = rc.max();
    const float step = (max - min) / nrintv;
    histmaxidx_ = histcount_ = 0;
    int histmax = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	int seg = (int)((rc.vals_[idx] - min) / step);
	if ( seg < -1 || seg > nrintv )
	    { pErrMsg("Huh"); continue; }
	if ( seg < 0 )		seg = 0;
	if ( seg == nrintv )	seg = nrintv - 1;
	histdata_[seg]++; histcount_++;
	if ( histdata_[seg] > histmax )
	    { histmax = histdata_[seg]; histmaxidx_ = seg; }
    }

    if ( canvas_ )
    {
	StepInterval<float> xrg( min + 0.5*step, max - 0.5*step, step );
	AxisLayout axlyo; axlyo.setDataRange( xrg );
	xrg.step = axlyo.sd.step;
	if ( !mIsEqual(xrg.start,axlyo.sd.start,axlyo.sd.step*1e-6) )
	    axlyo.sd.start += axlyo.sd.step;
	xax_->setRange( xrg, &axlyo.sd.start );

	StepInterval<float> yrg( 0, histmax, 1 );
	axlyo.setDataRange( yrg ); yrg.step = axlyo.sd.step;
	yax_->setRange( yrg );

	canvas_->update();
    }
}


void uiStatsDisplay::setMarkValue( float val )
{
    markval_ = val;
    if ( canvas_ ) canvas_->update();
}


void uiStatsDisplay::reDraw( CallBacker* cb )
{
    xax_->newDevSize();
    yax_->newDevSize();

    xax_->plotAxis();

    TypeSet<uiPoint> ptlist;
    const int nrhistpts = histdata_.size();
    float dx = (xax_->range().stop-xax_->range().start) / (nrhistpts-1);
    ptlist += uiPoint( xax_->getPix(xax_->range().start), yax_->getPix(0) );
    const uiPoint firstpt( xax_->getPix(xax_->range().start), yax_->getPix(0) );
    for ( int idx=0; idx<nrhistpts; idx++ )
    {
	const float xval = xax_->range().start + dx * idx;
	const uiPoint pt( xax_->getPix(xval), yax_->getPix(histdata_[idx]) );
	ptlist += pt;
	if ( idx == nrhistpts-1 )
	{
	    ptlist += uiPoint( pt.x, yax_->getPix(0) );
	    ptlist += firstpt;
	}
    }

    ioDrawTool& dt = canvas_->drawTool();
    dt.setPenColor( cHistColor );
    dt.setFillColor( cHistColor );
    dt.drawPolygon( ptlist );

    if ( !mIsUdf(markval_) )
    {
	dt.setPenColor( cMarkColor );
	xax_->drawGridLine( xax_->getPix(markval_) );
    }

    dt.setPenColor( Color::Black );
    if ( !setup_.countinplot_ ) return;

    BufferString str = "N="; str += histcount_;
    dt.drawText( dt.getDevWidth()/2, 0, str, mAlign(Middle,Start) );
}


uiStatsDisplayWin::uiStatsDisplayWin( uiParent* p,
					const uiStatsDisplay::Setup& su,
       					bool ismodal )
    : uiMainWin(p,"Data statistics",-1,false,ismodal)
    , disp_(*new uiStatsDisplay(this,su))
{
    statusBar()->addMsgFld( "Data name", uiStatusBar::Left, 1 );
}


void uiStatsDisplayWin::setData( const Stats::RunCalc<float>& rc )
{
    disp_.setData( rc );
}


void uiStatsDisplayWin::setDataName( const char* nm )
{
    BufferString txt( nm );
    char* nlptr = strchr( txt, '\n' );
    if ( nlptr ) *nlptr = '\0';
    statusBar()->message( txt, 0 );
}


void uiStatsDisplayWin::setMarkValue( float val )
{
    disp_.setMarkValue( val );
}
