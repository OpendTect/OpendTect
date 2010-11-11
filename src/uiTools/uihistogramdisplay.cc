/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uihistogramdisplay.cc,v 1.20 2010-11-11 05:50:01 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uihistogramdisplay.h"

#include "uiaxishandler.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"

#include "arraynd.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "statruncalc.h"

uiHistogramDisplay::uiHistogramDisplay( uiParent* p, 
					uiHistogramDisplay::Setup& su,
       					bool withheader	)
    : uiFunctionDisplay( p, su.fillbelow( true ).
	    		       yrg(Interval<float>(0,mUdf(float))) )
    , rc_( *new Stats::RunCalc<float>(Stats::RunCalcSetup()
					.require(Stats::Min)
					.require(Stats::Max)
					.require(Stats::Average)
					.require(Stats::Median)
					.require(Stats::StdDev)
					.require(Stats::RMS)) ) 
    , nrinpvals_(0)
    , nrclasses_(0)		   
    , withheader_(withheader)
    , header_(0)
    , nitm_(0)
{
    xAxis()->setName( "Value" );
    yAxis(false)->setName( "Count" );
}


uiHistogramDisplay::~uiHistogramDisplay()
{ 
    delete &rc_; delete header_; delete nitm_;
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


bool uiHistogramDisplay::setDataPackID( DataPack::ID dpid, DataPackMgr::ID dmid)
{
    DataPackMgr& dpman = DPM( dmid );
    const DataPack* datapack = dpman.obtain( dpid );
    if ( !datapack ) return false;

    if ( dmid == DataPackMgr::CubeID() )
    {
	mDynamicCastGet(const ::CubeDataPack*,cdp,datapack);
	const Array3D<float>* arr3d = cdp ? &cdp->data() : 0;
	if ( !arr3d ) return false;

	setData( arr3d->getData(), arr3d->info().getTotalSz() );
    }
    else if ( dmid == DataPackMgr::FlatID() )
    {
	mDynamicCastGet(const FlatDataPack*,fdp,datapack);
	mDynamicCastGet(const MapDataPack*,mdp,datapack);
	if ( mdp )
	    setData( &mdp->rawData() );
	else if( fdp )
	    setData( &fdp->data() );
	else
	    return false;
    }
    else 
	return false;

    if ( withheader_ )
    {
	if ( !header_ )
	{
	    const uiPoint pt( width()/2, 0 );
	    header_ = scene().addItem( new uiTextItem(pt,dpman.nameOf(dpid)) );
	    header_->setZValue( 2 );
	}
	else
	    header_->setText( dpman.nameOf(dpid) );
    }

    dpman.release( dpid );
    return true;
}


void uiHistogramDisplay::setData( const Array2D<float>* array )
{
    rc_.clear();
    if ( !array ) return;

    if ( array->getData() )
    {
	rc_.addValues( array->info().getTotalSz(), array->getData() );
	updateAndDraw();
	return;
    }
    
    const int sz2d0 = array->info().getSize( 0 );
    const int sz2d1 = array->info().getSize( 1 );
    for ( int idx0=0; idx0<sz2d0; idx0++ )
    {
	for ( int idx1=0; idx1<sz2d1; idx1++ )
	{
	    const float val = array->get( idx0, idx1 );
	    if ( mIsUdf(val) ) continue;

	    rc_.addValue( array->get( idx0, idx1 ) );
	}
    }

    updateAndDraw();
}


void uiHistogramDisplay::setData( const float* array, int sz )
{
    rc_.clear();
    if ( !array ) return;

    for ( int idx=0; idx<sz; idx++ )
    {
	const float val = array[idx];
	if ( mIsUdf(val) ) continue ;
	rc_.addValue( array[idx] );
    }

    updateAndDraw();
}


void uiHistogramDisplay::updateAndDraw()
{
    updateHistogram();
    draw();
}


void uiHistogramDisplay::updateHistogram()
{
    const int nrpts = rc_.count();
    nrclasses_ = getNrIntervals( nrpts );
    TypeSet<float> histdata( nrclasses_, 0 );
    const float min = rc_.min(); const float max = rc_.max();
    float step = (max - min) / nrclasses_;
    if ( mIsZero(step,1e-6) )
	step = 1;

    nrinpvals_ = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	int seg = (int)((rc_.vals_[idx] - min) / step);
	if ( seg < -1 || seg > nrclasses_ )
	   { pErrMsg("Huh"); continue; }

	if ( seg < 0 )			seg = 0;

	if ( seg == nrclasses_ )	seg = nrclasses_ - 1;

	histdata[seg] += 1; nrinpvals_++;
    }

    setHistogram( histdata, Interval<float>(min + 0.5*step, max - 0.5*step),
	    	  nrinpvals_ );
}


void uiHistogramDisplay::setHistogram( const TypeSet<float>& histdata,
				       Interval<float> xrg, int nrvals )
{
    nrinpvals_ = nrvals;
    setVals( xrg, histdata.arr(), histdata.size() );
}


void uiHistogramDisplay::putN()
{
    if ( nrinpvals_ < 1 || nitm_ ) return;

    BufferString str = "N="; str += nrinpvals_;
    if ( !nitm_ )
    {
	nitm_ = scene().addItem( new uiTextItem(uiPoint(width()/2,0),str) );
	nitm_->setPenColor( Color::Black() );
    }
    else
	nitm_->setText( str );
}
