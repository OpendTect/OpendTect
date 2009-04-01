/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistatsdisplay.cc,v 1.23 2009-04-01 14:35:39 cvsbert Exp $";

#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"

#include "uihistogramdisplay.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uistatusbar.h"

#include "arraynd.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "statruncalc.h"

uiStatsDisplay::uiStatsDisplay( uiParent* p, const uiStatsDisplay::Setup& su )
    : uiGroup( p, "Statistics display group" )
    , setup_(su)
    , histgramdisp_(0)
    , minmaxfld_(0)
    , countfld_(0)
{
    if ( setup_.withplot_ )
    {
	uiHistogramDisplay::Setup fsu;
	fsu.annoty( setup_.vertaxis_ );
	histgramdisp_ = new uiHistogramDisplay( this, fsu );
    }

    uiSeparator* sep = 0;
    if ( setup_.withplot_ && setup_.withtext_ )
    {
	sep = new uiSeparator( this, "Hor sep" );
	sep->attach( stretchedBelow, histgramdisp_ );
    }

    const bool putcountinplot = histgramdisp_ && setup_.countinplot_;
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
	    valgrp->attach( centeredBelow, histgramdisp_ );
	    valgrp->attach( ensureBelow, sep );
	}
    }

    if ( putcountinplot )
	putN();
}


bool uiStatsDisplay::setDataPackID( DataPack::ID dpid, DataPackMgr::ID dmid )
{
    if ( !histgramdisp_ || 
	 (histgramdisp_ && !histgramdisp_->setDataPackID(dpid,dmid)) )
	return false;
    
    setData( histgramdisp_->getRunCalc() );

    return true;
}


void uiStatsDisplay::setData( const float* array, int sz )
{
    if ( !histgramdisp_ )
	return;

    histgramdisp_->setData( array, sz );
    setData( histgramdisp_->getRunCalc() );
}


void uiStatsDisplay::setData( const Array2D<float>* array )
{
    if ( !histgramdisp_ )
	return;

    histgramdisp_->setData( array );
    setData( histgramdisp_->getRunCalc() );
}


void uiStatsDisplay::setData( const Stats::RunCalc<float>& rc )
{
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


void uiStatsDisplay::setMarkValue( float val, bool forx )
{
    if ( histgramdisp_ )
	histgramdisp_->setMarkValue( val, forx );
}


void uiStatsDisplay::putN()
{
    if ( !setup_.countinplot_ || !histgramdisp_ ) return;

    histgramdisp_->putN();
}


uiStatsDisplayWin::uiStatsDisplayWin( uiParent* p,
					const uiStatsDisplay::Setup& su,
       					bool ismodal )
    : uiMainWin(p,"Data statistics",-1,false,ismodal)
    , disp_(*new uiStatsDisplay(this,su))
{
    statusBar()->addMsgFld( "Data name", Alignment::Left, 1 );
}


void uiStatsDisplayWin::setData( const Stats::RunCalc<float>& rc )
{
    disp_.setData( rc.vals_.arr(), rc.vals_.size() );
}


void uiStatsDisplayWin::setDataName( const char* nm )
{
    BufferString txt( nm );
    char* nlptr = strchr( txt.buf(), '\n' );
    if ( nlptr ) *nlptr = '\0';
    statusBar()->message( txt, 0 );
}


void uiStatsDisplayWin::setMarkValue( float val, bool forx )
{
    disp_.setMarkValue( val, forx );
}
