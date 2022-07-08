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
#include "datapointset.h"
#include "statparallelcalc.h"

uiHistogramDisplay::uiHistogramDisplay( uiParent* p,
					uiHistogramDisplay::Setup& su,
					bool withheader	)
    : uiFunctionDisplay( p, su.fillbelow(true).yannotinint(true)
			    .noy2axis(true).noy2gridline(true)
			    .yrg(Interval<float>(0,mUdf(float)))
			    .fixdrawrg(true) )
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
    yAxis(false)->setCaption( tr("Count") );
}


uiHistogramDisplay::~uiHistogramDisplay()
{
    delete &rc_; delete header_; delete nitm_;
    deepErase( baritems_ );
}


static int getNrIntervals( int nrpts )
{
    return 100;
}


void uiHistogramDisplay::setEmpty()
{
    uiFunctionDisplay::setEmpty();
    rc_.setEmpty();
    updateAndDraw();
}


bool uiHistogramDisplay::setDataPackID(
	DataPack::ID dpid, DataPackMgr::MgrID dmid, int version )
{
    rc_.setEmpty();
    auto dp = DPM(dmid).getDP( dpid );
    if ( !dp ) return false;

    BufferString dpversionnm;

    if ( dmid == DataPackMgr::SeisID() )
    {
	mDynamicCastGet(const SeisDataPack*,seisdp,dp.ptr());
	if ( !seisdp || seisdp->isEmpty() ) return false;

	const Array3D<float>* arr3d = &seisdp->data( version );
	dpversionnm = seisdp->getComponentName(version);
	setData( arr3d );
    }
    else if ( dmid == DataPackMgr::FlatID() )
    {
	mDynamicCastGet(const FlatDataPack*,fdp,dp.ptr())
	mDynamicCastGet(const MapDataPack*,mdp,dp.ptr())
	if ( mdp )
	{
	    dpversionnm = mdp->name();
	    setData( &mdp->rawData() );
	}
	else if( fdp )
	{
	    dpversionnm = fdp->name();
	    setData( &fdp->data() );
	}
	else
	    return false;
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
	{ setData( array->getData(), array->info().getTotalSz() ); return; }

    const int sz2d0 = array->info().getSize( 0 );
    const int sz2d1 = array->info().getSize( 1 );
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
	{ setData( array->getData(), array->info().getTotalSz() ); return; }

    const int sz0 = array->info().getSize( 0 );
    const int sz1 = array->info().getSize( 1 );
    const int sz2 = array->info().getSize( 2 );
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


void uiHistogramDisplay::drawData()
{
    const int nrpts = xvals_.size();
    if ( nrpts < 2 )
	return;

    const uiAxisHandler* xax = xAxis();
    const uiAxisHandler* yax = yAxis( false );

    const float basepix = yax->getPix( yax->range().start );
    float xwidth = xvals_[1] - xvals_[0];
    for ( int idx=0; idx<nrpts; idx++ )
    {
	uiRectItem* baritem = baritems_.validIdx(idx) ? baritems_[idx] : 0;
	const float xleft = idx ? (xvals_[idx] + xvals_[idx-1])/2
				: xvals_[idx] - xwidth/2;
	const float xright = idx==nrpts-1 ? xvals_[idx] + xwidth/2
					  : (xvals_[idx] + xvals_[idx+1])/2;
	const float origxpix = xax->getPix( xleft );
	const float barwidth = xax->getPix( xright ) - origxpix;
	const float barheight = yax->getPix( yvals_[idx] ) - basepix;
	if ( baritem )
	{
	    baritem->setRect( origxpix, basepix, barwidth, barheight );
	    continue;
	}

	baritem = scene().addRect( origxpix, basepix, barwidth, barheight );
	baritem->setZValue( 30 );
	baritem->setFillColor( OD::Color(200,160,140) );
	baritems_ += baritem;
    }

    rePaint();
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

    const od_int64 nrpts = rc_.count();
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
    const float xstep = xrg.width() / (histdata.size()-1);
    setup_.xrg( Interval<float>(xrg.start-xstep/2, xrg.stop+xstep/2) );
    setVals( xrg, histdata.arr(), histdata.size() );
}


void uiHistogramDisplay::putN()
{
    delete nitm_; nitm_ = 0;
    nitm_ = scene().addItem( new uiTextItem(uiPoint(viewWidth()/10,0),
						tr("N=%1").arg(nrinpvals_)) );
    nitm_->setPenColor( OD::Color::Black() );
    nitm_->setZValue( 99999 );
}
