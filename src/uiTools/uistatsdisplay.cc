/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistatsdisplay.cc,v 1.17 2008-12-02 03:30:31 cvssatyaki Exp $";

#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"

#include "uiaxishandler.h"
#include "uifunctiondisplay.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uistatusbar.h"

#include "arraynd.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "errh.h"
#include "statruncalc.h"

static const int cCanvasHeight = 250;
static const int cCanvasWidth = 400;
static const int cBoundarySz = 10;
static const Color cHistColor( 0, 200, 0 );
static const Color cMarkColor( 230, 0, 0 );

uiStatsDisplay::uiStatsDisplay( uiParent* p, const uiStatsDisplay::Setup& su )
    : uiGroup( p, "Statistics display group" )
    , setup_(su)
    , funcdisp_(0)
    , minmaxfld_(0)
    , countfld_(0)
    , nrclasses_(0)
    , nrinpvals_(0)
{
    if ( setup_.withplot_ )
    {
	uiFunctionDisplay::Setup fsu;
	fsu.yrg_.start = 0; fsu.annoty( setup_.vertaxis_ ).fillbelow( true );
	funcdisp_ = new uiFunctionDisplay( this, fsu );
	funcdisp_->xAxis()->setName( "Value" );
	funcdisp_->yAxis(false)->setName( "Count" );
    }

    uiSeparator* sep = 0;
    if ( setup_.withplot_ && setup_.withtext_ )
    {
	sep = new uiSeparator( this, "Hor sep" );
	sep->attach( stretchedBelow, funcdisp_ );
    }

    const bool putcountinplot = funcdisp_ && setup_.countinplot_;
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
	if ( !putcountinplot )
	{
	    countfld_ = new uiGenInput( valgrp, "Number of values" );
	    countfld_->setReadOnly();
	    countfld_->attach( alignedBelow, medrmsfld_ );	
	}

	if ( sep )
	{
	    valgrp->attach( centeredBelow, funcdisp_ );
	    valgrp->attach( ensureBelow, sep );
	}
    }

    if ( putcountinplot )
	putN();
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
    funcdisp_->draw();
}


void uiStatsDisplay::updateHistogram( const Stats::RunCalc<float>& rc )
{
    const int nrpts = rc.count();
    nrclasses_ = getNrIntervals( nrpts );
    TypeSet<float> histdata( nrclasses_, 0 );
    const float min = rc.min(); const float max = rc.max();
    const float step = (max - min) / nrclasses_;
    nrinpvals_ = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	int seg = (int)((rc.vals_[idx] - min) / step);
	if ( seg < -1 || seg > nrclasses_ )
	    { pErrMsg("Huh"); continue; }
	if ( seg < 0 )			seg = 0;
	if ( seg == nrclasses_ )	seg = nrclasses_ - 1;
	histdata[seg] += 1; nrinpvals_++;
    }

    setHistogram( histdata, Interval<float>(min + 0.5*step, max - 0.5*step),
	    	  nrinpvals_ );
}


void uiStatsDisplay::setHistogram( const TypeSet<float>& histdata,
				   Interval<float> xrg, int nrvals )
{
    nrinpvals_ = nrvals;
    if ( funcdisp_ )
	funcdisp_->setVals( xrg, histdata.arr(), histdata.size() );
}


void uiStatsDisplay::setMarkValue( float val, bool forx )
{
    if ( funcdisp_ )
    {
	funcdisp_->setMarkValue( val, forx );
	//funcdisp_->update();
    }
}


void uiStatsDisplay::putN()
{
    if ( !setup_.countinplot_ || !funcdisp_ || nrinpvals_ < 1 ) return;


    BufferString str = "N="; str += nrinpvals_;
    uiTextItem* textitem = funcdisp_->scene().addText( str );
    textitem->setPos( funcdisp_->width()/2, 0 );
    textitem->setPenColor( Color::Black );
    funcdisp_->draw();
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


void uiStatsDisplayWin::setMarkValue( float val, bool forx )
{
    disp_.setMarkValue( val, forx );
}
