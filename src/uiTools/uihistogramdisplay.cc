/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uihistogramdisplay.h"

#include "uiaxishandler.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uimsg.h"
#include "uistrings.h"

#include "arrayndimpl.h"
#include "bufstring.h"
#include "datadistribution.h"
#include "datapointset.h"
#include "statparallelcalc.h"

uiHistogramDisplay::uiHistogramDisplay( uiParent* p,
					uiHistogramDisplay::Setup& su,
					bool withheader	)
    : uiFunctionDisplay( p, su.fillbelow(true).yannotinint(true)
			    .noy2axis(true).noy2gridline(true)
			    .yrg(Interval<float>(0,mUdf(float))) )
    , rc_(*new Stats::ParallelCalc<float>(Stats::CalcSetup(false)
					    .require(Stats::Min)
					    .require(Stats::Max)
					    .require(Stats::Average)
					    .require(Stats::Median)
					    .require(Stats::StdDev)
					    .require(Stats::RMS)) )
    , withheader_(withheader)
    , mydrawrg_(mUdf(float),mUdf(float))
    , drawRangeChanged(this)
{
    xAxis()->setCaption( uiStrings::sValue() );
    yAxis(false)->setCaption( uiStrings::sCount() );
}


uiHistogramDisplay::~uiHistogramDisplay()
{
    delete &rc_; delete header_; delete nitm_;
}


static int getNrIntervals( int nrpts )
{
    int res = nrpts / 25;
    if ( res < 10 && nrpts < 10 ) res = 10;
    else if ( res < 20 && nrpts < 20 ) res = 20;
    else if ( res < 50 && nrpts < 50 ) res = 50;
    else res = 100;

    return res;
}


void uiHistogramDisplay::setEmpty()
{
    uiFunctionDisplay::setEmpty();
    rc_.setEmpty();
    updateAndDraw();
}


bool uiHistogramDisplay::setDataPackID(
	DataPack::ID dpid, DataPackMgr::ID dmid, int version )
{
    rc_.setEmpty();
    auto dp = DPM(dmid).getDP( dpid );
    if ( !dp )
	return false;

    BufferString dpversionnm;

    if ( dmid == DataPackMgr::SeisID() )
    {
	mDynamicCastGet(const VolumeDataPack*,voldp,dp.ptr());
	if ( !voldp || voldp->isEmpty() )
	    return false;

	const Array3D<float>* arr3d = &voldp->data( version );
	dpversionnm = voldp->name();
	setData( arr3d );
    }
    else if ( dmid == DataPackMgr::FlatID() )
    {
	mDynamicCastGet(const FlatDataPack*,fdp,dp.ptr())
	if ( !fdp )
	    return false;
	{
	    dpversionnm = fdp->name();
	    setData( &fdp->data() );
	}
    }
    else if ( dmid == DataPackMgr::SurfID() || dmid == DataPackMgr::PointID() )
    {
	mDynamicCastGet(const DataPointSet*,dpset,dp.ptr())
	if ( !dpset )
	    return false;

	dpversionnm = dpset->name();
	setDataDPS( *dpset, dpset->nrCols()-1 );
    }
    else
	return false;

    if ( withheader_ )
    {
	if ( !header_ )
	{
	    const uiPoint pt( viewWidth()/2, 0 );
	    header_ = scene().addItem(
			new uiTextItem(pt,toUiString(dpversionnm)) );
	    header_->setZValue( 2 );
	}
	else
	    header_->setText( toUiString(dpversionnm) );
    }

    return true;
}


void uiHistogramDisplay::setDataDPS( const DataPointSet& dpset, int dpsidx )
{
    TypeSet<float> vals;
    for ( int idx=0; idx<dpset.size(); idx++ )
    {
	const float val = dpset.value( dpsidx, idx );
	if ( !mIsUdf(val) )
	    vals += val;
    }

    setData( vals.arr(), vals.size() );
}


void uiHistogramDisplay::setData( const Array2D<float>* array )
{
    if ( !array )
	{ rc_.setEmpty(); return; }

    if ( array->getData() )
	{ setData( array->getData(), array->totalSize() ); return; }

    const int sz2d0 = array->getSize( 0 );
    const int sz2d1 = array->getSize( 1 );
    LargeValVec<float> vals;
    for ( int idx0=0; idx0<sz2d0; idx0++ )
    {
	for ( int idx1=0; idx1<sz2d1; idx1++ )
	{
	    const float val = array->get( idx0, idx1 );
	    if ( !mIsUdf(val) )
		vals += val;
	}
    }
    rc_.setValues( vals.arr(), vals.size() );
    updateAndDraw();
}


void uiHistogramDisplay::setData( const Array3D<float>* array )
{
    if ( !array )
	{ rc_.setEmpty(); return; }

    if ( array->getData() )
	{ setData( array->getData(), array->totalSize() ); return; }

    const int sz0 = array->getSize( 0 );
    const int sz1 = array->getSize( 1 );
    const int sz2 = array->getSize( 2 );
    LargeValVec<float> vals;
    for ( int idx0=0; idx0<sz0; idx0++ )
	for ( int idx1=0; idx1<sz1; idx1++ )
	    for ( int idx2=0; idx2<sz2; idx2++ )
	    {
		const float val = array->get( idx0, idx1, idx2 );
		if ( !mIsUdf(val) )
		    vals += val;
	    }

    rc_.setValues( vals.arr(), vals.size() );
    updateAndDraw();
}


void uiHistogramDisplay::setData( const LargeValVec<float>& vals )
{
    setData( vals.arr(), vals.size() );
}


void uiHistogramDisplay::setData( const float* array, od_int64 sz )
{
    if ( !array || sz < 1 )
	{ rc_.setEmpty(); return; }

    if ( array != originaldata_.arr() )
    {
	originaldata_.setSize( sz, mUdf(float) );
	if ( originaldata_.arr() )
	    OD::memCopy( originaldata_.arr(), array, sz*sizeof(float) );
	else
	{
	    for ( od_int64 idx=0; idx<sz; idx++ )
		originaldata_[idx] = array[idx];
	}
    }

    const bool usedrawrg = usemydrawrg_ && !mIsUdf(mydrawrg_.start) &&
			   !mIsUdf(mydrawrg_.stop);
    if ( usedrawrg )
    {
	LargeValVec<float> mydisplaydata( sz, mUdf(float) );
	od_int64 addedcount = 0;
	for ( od_int64 idx=0; idx<sz; idx++ )
	{
	    const float& arrval = array[idx];
	    if ( mIsUdf(arrval) )
		continue;

	    if ( mydrawrg_.includes(arrval,false) )
		mydisplaydata[addedcount++] = arrval;
	}

	rc_.setValues( mydisplaydata.arr(), addedcount );
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
	{ gUiMsg().error( rc_.errMsg() ); return; }

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
    for ( od_int64 idx=0; idx<nrpts; idx++ )
    {
	int seg = mCast(int,(rc_.medvals_[idx] - min) / step);
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
	setData( originaldata_ );
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
	setData( originaldata_ );
	drawRangeChanged.trigger();
    }
}



void uiHistogramDisplay::setHistogram( const TypeSet<float>& histdata,
				       Interval<float> xrg, int nrvals )
{
    nrinpvals_ = nrvals;
    setVals( xrg, histdata.arr(), histdata.size() );
}


void uiHistogramDisplay::setDistribution( const FloatDistrib& distr,
					  int nrvals )
{
    nrinpvals_ = nrvals;
    const int sz = distr.size();
    const SamplingData<float> sd = distr.sampling();
    const Interval<float> xrg( sd.start, sd.atIndex(sz-1) );
    const TypeSet<float> distrdata( distr.getSet(false) );
    setVals( xrg, distrdata.arr(), distrdata.size() );
}


void uiHistogramDisplay::putN()
{
    delete nitm_; nitm_ = 0;
    nitm_ = scene().addItem( new uiTextItem(uiPoint(viewWidth()/10,0),
						tr("N=%1").arg(nrinpvals_)) );
    nitm_->setPenColor( Color::Black() );
    nitm_->setZValue( 99999 );
}
