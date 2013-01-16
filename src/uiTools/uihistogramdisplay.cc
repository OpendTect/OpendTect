/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uihistogramdisplay.h"

#include "uiaxishandler.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uimsg.h"

#include "arraynd.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "datapointset.h"
#include "statparallelcalc.h"

uiHistogramDisplay::uiHistogramDisplay( uiParent* p, 
					uiHistogramDisplay::Setup& su,
       					bool withheader	)
    : uiFunctionDisplay( p, su.fillbelow( true ).
	    		       yrg(Interval<float>(0,mUdf(float))) )
    , rc_(*new Stats::ParallelCalc<float>(Stats::CalcSetup(false)
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
    , mydrawrg_(mUdf(float),mUdf(float))
    , usemydrawrg_(false) 
    , drawRangeChanged(this)  
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

	setData( arr3d->getData(), mCast(int,arr3d->info().getTotalSz()) );
    }
    else if ( dmid == DataPackMgr::FlatID() )
    {
	mDynamicCastGet(const FlatDataPack*,fdp,datapack)
	mDynamicCastGet(const MapDataPack*,mdp,datapack)
	if ( mdp )
	    setData( &mdp->rawData() );
	else if( fdp )
	    setData( &fdp->data() );
	else
	    return false;
    }
    else if ( dmid == DataPackMgr::SurfID() )
    {
	mDynamicCastGet(const DataPointSet*,dpset,datapack)
	if ( !dpset )
	    return false;

	setData( *dpset );
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


void uiHistogramDisplay::setData( const DataPointSet& dpset )
{
    TypeSet<float> valarr;
    for ( int idx=0; idx<dpset.size(); idx++ )
    {
	const float val = dpset.value( 2, idx );
	if ( mIsUdf(val) )
	    continue;

	valarr += val;
    }
    setData( valarr.arr(), valarr.size() ); 
}


void uiHistogramDisplay::setData( const Array2D<float>* array )
{
    const float* data = array ? array->getData() : 0;
    if ( !data ) return;

    const int totalsz = mCast(int, array->info().getTotalSz() );
    TypeSet<float> valarr;
    for ( int idx=0; idx<totalsz; idx++ )
    {
	if ( !mIsUdf(data[idx]) )
	    valarr += data[idx];
    }
    setData( valarr.arr(), valarr.size() ); 
}


void uiHistogramDisplay::setData( const float* array, int sz )
{
    if ( !array ) return;

    if ( array != originaldata_.arr() )
    {
    	originaldata_.erase();
    	for ( int idx=0; idx<sz; idx++ )
    	    originaldata_ += array[idx];
    }

    const bool usedrawrg = usemydrawrg_ && !mIsUdf(mydrawrg_.start) && 
	!mIsUdf(mydrawrg_.stop);
    if ( usedrawrg )
    {
	mydisplaydata_.erase();
	for ( int idx=0; idx<sz; idx++ )
	{
    	    if ( mIsUdf(array[idx]) )
     		continue;
      	    
	    if ( mydrawrg_.includes(array[idx],false) )
    		mydisplaydata_ += array[idx];
	}
    
	rc_.setValues( mydisplaydata_.arr(), mydisplaydata_.size() );
    }
    else
	rc_.setValues( originaldata_.arr(), originaldata_.size() );

    updateAndDraw();
}


void uiHistogramDisplay::updateAndDraw()
{
    updateHistogram();
    draw();
}


void uiHistogramDisplay::updateHistogram()
{
    if ( !rc_.execute() )
	{ uiMSG().error( rc_.errMsg() ); return; }

    const int nrpts = rc_.count();
    nrclasses_ = getNrIntervals( nrpts );
    TypeSet<float> histdata( nrclasses_, 0 );
    const float min = rc_.min(); const float max = rc_.max();
    const float step = (max - min) / nrclasses_;
    if ( mIsZero(step,1e-6) )
    {
	histdata[nrclasses_/2] = mCast( float, nrpts );
	setHistogram( histdata, Interval<float>(min-1,max+1), nrpts );
	return;
    }

    nrinpvals_ = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	int seg = (int)((rc_.medvals_[idx] - min) / step);
	if ( seg < -1 || seg > nrclasses_ )
	   { pErrMsg("Huh"); continue; }

	if ( seg < 0 )			seg = 0;

	if ( seg == nrclasses_ )	seg = nrclasses_ - 1;

	histdata[seg] += 1; nrinpvals_++;
    }

    setHistogram( histdata, Interval<float>(min + 0.5f*step, max - 0.5f*step),
	    	  nrinpvals_ );
}


void uiHistogramDisplay::useDrawRange( bool yn )    
{
   if ( usemydrawrg_==yn )
      return;

    usemydrawrg_ = yn; 
    if ( usemydrawrg_ && !mIsUdf(mydrawrg_.start) && !mIsUdf(mydrawrg_.stop) )
    {
	setData( originaldata_.arr(), originaldata_.size() );
    	drawRangeChanged.trigger();
    }
}


void uiHistogramDisplay::setDrawRange( const Interval<float>& ni )
{
    if ( mIsEqual(ni.start,mydrawrg_.start,1e-5) && 
	 mIsEqual(ni.stop,mydrawrg_.stop,1e-5) )
	return;

    mydrawrg_ = ni;
    if ( usemydrawrg_ )
    {
	setData( originaldata_.arr(), originaldata_.size() );
    	drawRangeChanged.trigger();
    }
}



void uiHistogramDisplay::setHistogram( const TypeSet<float>& histdata,
				       Interval<float> xrg, int nrvals )
{
    nrinpvals_ = nrvals;
    setVals( xrg, histdata.arr(), histdata.size() );
}


void uiHistogramDisplay::putN()
{
    delete nitm_; nitm_ = 0;
    nitm_ = scene().addItem( new uiTextItem(uiPoint(width()/10,0),
				BufferString("N=",nrinpvals_)) );
    nitm_->setPenColor( Color::Black() );
    nitm_->setZValue( 99999 );
}
