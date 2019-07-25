/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"

#include "uiaxishandler.h"
#include "uicombobox.h"
#include "uihistogramdisplay.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"

#include "arrayndimpl.h"
#include "draw.h"
#include "bufstring.h"
#include "datapointset.h"
#include "datadistributiontools.h"
#include "mouseevent.h"
#include "statparallelcalc.h"

#define mPutCountInPlot() (histgramdisp_ && setup_.countinplot_)


uiStatsDisplay::uiStatsDisplay( uiParent* p, const uiStatsDisplay::Setup& su )
    : uiGroup( p, "Statistics display group" )
    , setup_(su)
    , histgramdisp_(0)
    , minmaxfld_(0)
    , countfld_(0)
    , namefld_(0)
{
    const bool putcountinplot = mPutCountInPlot();
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

    if ( setup_.withtext_ )
    {
	if ( setup_.withname_ )
	    namefld_ = new uiLabel( this, tr("Data Name") );

	uiGroup* valgrp = new uiGroup( this, "Values group" );
	if ( setup_.withname_ )
	    valgrp->attach( alignedBelow, namefld_ );
	minmaxfld_ = new uiGenInput( valgrp, tr("Value range"),
				     FloatInpSpec(), FloatInpSpec() );
	minmaxfld_->setReadOnly();
	avgstdfld_ = new uiGenInput( valgrp, tr("Mean/Std Deviation"),
				     DoubleInpSpec(), DoubleInpSpec() );
	avgstdfld_->attach( alignedBelow, minmaxfld_ );
	avgstdfld_->setReadOnly();
	medrmsfld_ = new uiGenInput( valgrp, tr("Median/RMS"),
				     FloatInpSpec(), DoubleInpSpec() );
	medrmsfld_->attach( alignedBelow, avgstdfld_ );
	medrmsfld_->setReadOnly();
	if ( !putcountinplot )
	{
	    countfld_ = new uiGenInput( valgrp, tr("Number of values") );
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


void uiStatsDisplay::setDataName( const char* nm )
{
    if ( namefld_ )
    {
	namefld_->setText( toUiString(nm) );
	namefld_->setAlignment( OD::Alignment::HCenter );
    }
}


bool uiStatsDisplay::setDataPackID(
		DataPack::ID dpid, DataPackMgr::ID dmid, int version )
{
    if ( !histgramdisp_ || (histgramdisp_ &&
		!histgramdisp_->setDataPackID(dpid,dmid,version)) )
    {
	Stats::ParallelCalc<float> rc( (Stats::CalcSetup()
						.require(Stats::Min)
						.require(Stats::Max)
						.require(Stats::Average)
						.require(Stats::Median)
						.require(Stats::StdDev)
						.require(Stats::RMS)) );

	DataPackMgr& dpman = DPM( dmid );
	ConstRefMan<DataPack> datapack = dpman.getDP( dpid );
	if ( !datapack ) return false;

	TypeSet<float> valarr;
	if ( dmid == DataPackMgr::SeisID() )
	{
	    mDynamicCastGet(const VolumeDataPack*,vdp,datapack.ptr());
	    const Array3D<float>* arr3d = vdp ? &vdp->data(version) : 0;
	    if ( !arr3d ) return false;

	    const float* array = arr3d->getData();
	    if ( array )
		rc.setValues( array, mCast(int,arr3d->totalSize()) );
	    else
	    {
		valarr.setCapacity(mCast(int,arr3d->totalSize()),false);
		const int sz0 = arr3d->getSize( 0 );
		const int sz1 = arr3d->getSize( 1 );
		const int sz2 = arr3d->getSize( 2 );
		for ( int idx=0; idx<sz0; idx++ )
		{
		    for ( int idy=0; idy<sz1; idy++ )
		    {
			for ( int idz=0; idz<sz2; idz++ )
			{
			    const float val = arr3d->get( idx, idy, idz );
			    if ( !mIsUdf(val) )
				valarr += val;
			}
		    }
		}

		rc.setValues( valarr.arr(), valarr.size() );
	    }
	}
	else if ( dmid == DataPackMgr::FlatID() )
	{
	    const Array2D<float>* array = 0;
	    mDynamicCastGet(const FlatDataPack*,fdp,datapack.ptr());
	    if ( fdp )
		array = &fdp->data();
	    if ( !array )
		return false;

	    if ( array->getData() )
		rc.setValues( array->getData(), (int)array->totalSize() );
	    else
	    {
		valarr.setCapacity(mCast(int,array->totalSize()),false);
		const int sz2d0 = array->getSize( 0 );
		const int sz2d1 = array->getSize( 1 );
		for ( int idx0=0; idx0<sz2d0; idx0++ )
		{
		    for ( int idx1=0; idx1<sz2d1; idx1++ )
		    {
			const float val = array->get( idx0, idx1 );
			if ( mIsUdf(val) ) continue;

			valarr += val;
		    }
		}

		rc.setValues( valarr.arr(), valarr.size() );
	    }
	}
	else if ( dmid == DataPackMgr::SurfID() ||
		  dmid == DataPackMgr::PointID())
	{
	    mDynamicCastGet(const DataPointSet*,dpset,datapack.ptr())
	    if ( !dpset )
		return false;

	    const int colid = dpset->nrCols() - 1;
	    valarr.setCapacity( dpset->size(), false );
	    for ( int idx=0; idx<dpset->size(); idx++ )
		valarr += dpset->value( colid, idx );

	    rc.setValues( valarr.arr(), valarr.size() );
	}
	if ( !rc.execute() )
	    { uiMSG().error( rc.errMsg() ); return false; }

	setData( rc );
	return false;
    }

    setData( histgramdisp_->getStatCalc() );
    return true;
}


void uiStatsDisplay::setData( const TypeSet<float>& vals )
{
    setData( vals.arr(), (od_int64)vals.size() );
}


void uiStatsDisplay::setData( const float* array, od_int64 sz )
{
    if ( !histgramdisp_ )
	return;

    histgramdisp_->setData( array, sz );
    setData( histgramdisp_->getStatCalc() );
}


void uiStatsDisplay::setData( const Array2D<float>* array )
{
    if ( !histgramdisp_ )
	return;

    histgramdisp_->setData( array );
    setData( histgramdisp_->getStatCalc() );
}


void uiStatsDisplay::setData( const Stats::ParallelCalc<float>& rc )
{
    if ( mPutCountInPlot() )
	putN();

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


#define mGetAndSetParam( type, nm, str, fld, idx ) \
    type nm = mUdf(type); \
    iop.get( str, nm ); \
    if ( fld ) \
	fld->setValue( nm, idx );

void uiStatsDisplay::usePar( const IOPar& iop )
{
    Interval<float> xrg;
    iop.get( sKey::ValueRange(), xrg );
    mGetAndSetParam( int, nrvals, sKey::NrValues(), countfld_, 0 );
    if ( histgramdisp_ )
    {
	TypeSet<float> histdata;
	iop.get( sKey::Data(), histdata );
	histgramdisp_->setHistogram( histdata, xrg, nrvals );
    }

    minmaxfld_->setValue( xrg.start, 0 );
    minmaxfld_->setValue( xrg.stop, 1 );
    mGetAndSetParam( double, avg, sKey::Average(), avgstdfld_, 0 );
    mGetAndSetParam( double, stddev, sKey::StdDev(), avgstdfld_, 1 );
    mGetAndSetParam( float, median, sKey::Median(), medrmsfld_, 0 );
    mGetAndSetParam( double, rms, sKey::RMS(), medrmsfld_, 1 );
}


void uiStatsDisplay::setData( const FloatDistrib& distrib,
			      od_int64 count, Interval<float> rg )
{
    histgramdisp_->setDistribution( distrib );
    if ( countfld_ )
	countfld_->setValue( count );
    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
	rg = distrib.dataRange();
    DataDistributionInfoExtracter<float> distrinfextr( distrib );
    float med, avg, stdev, rms;
    med = distrib.medianPosition();
    distrinfextr.getAvgStdRMS( avg, stdev, rms );

    minmaxfld_->setValue( rg.start, 0 );
    minmaxfld_->setValue( rg.stop, 1 );
    avgstdfld_->setValue( avg, 0 );
    avgstdfld_->setValue( stdev, 1 );
    medrmsfld_->setValue( med, 0 );
    medrmsfld_->setValue( rms, 1 );
}


void uiStatsDisplay::setMarkValue( float val, bool forx )
{
    if ( histgramdisp_ )
	histgramdisp_->setMarkValue( val, forx );
}


void uiStatsDisplay::putN()
{
    if ( !histgramdisp_ )
	return;

    histgramdisp_->putN();
}


uiStatsDisplayWin::uiStatsDisplayWin( uiParent* p,
					const uiStatsDisplay::Setup& su,
					int nr, bool ismodal )
    : uiMainWin(p,uiStrings::phrData(uiStrings::sStatistics()),1,false,ismodal)
    , statnmcb_(0)
    , currentdispidx_(-1)
{
    uiLabeledComboBox* lblcb=0;
    if ( nr > 1 )
    {
	lblcb = new uiLabeledComboBox( this, uiStrings::phrSelect(
							   uiStrings::sData()));
	statnmcb_ = lblcb->box();
    }

    for ( int idx=0; idx<nr; idx++ )
    {
	uiStatsDisplay* disp = new uiStatsDisplay( this, su );
	if ( statnmcb_ )
	    disp->attach( rightAlignedBelow, lblcb );

	if ( disp->funcDisp() )
	{
	    disp->funcDisp()->getMouseEventHandler().movement.notify(
				   mCB(this,uiStatsDisplayWin,mouseMoveCB) );
	}

	disps_ += disp;
	disp->display( false );
    }

    showStat( 0 );
    if ( statnmcb_ )
	statnmcb_->selectionChanged.notify(
		mCB(this,uiStatsDisplayWin,dataChanged) );
}


void uiStatsDisplayWin::showStat( int idx )
{
    currentdispidx_ = idx;
    if ( disps_.validIdx(idx) )
	disps_[idx]->display( true );
}


void uiStatsDisplayWin::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb)
    if ( !meh ) return;

    uiHistogramDisplay* disp = disps_.validIdx( currentdispidx_ )
		? disps_[currentdispidx_]->funcDisp() : 0;
    if ( !disp ) return;

    const Geom::Point2D<int>& pos = disp->getMouseEventHandler().event().pos();
    Geom::Point2D<float> val = disp->getFuncXY( pos.x_, false );
    uiString str = tr("Values/Count: %1/%2").arg(toUiString(val.x_,4)).
		   arg(toUiString(val.y_,0));
    toStatusBar( str );
}


void uiStatsDisplayWin::dataChanged( CallBacker* )
{
    if ( !statnmcb_ )
	return;

    for ( int idx=0; idx<disps_.size(); idx++ )
	disps_[idx]->display( false );

    showStat( statnmcb_->currentItem() );
}


void uiStatsDisplayWin::setData( const float* medvals, int medsz, int dispidx )
{
    if ( disps_.validIdx(dispidx) )
	disps_[dispidx]->setData( medvals, medsz );
}


void uiStatsDisplayWin::addDataNames( const BufferStringSet& nms )
{ if ( statnmcb_ ) statnmcb_->addItems( nms ); }


void uiStatsDisplayWin::setDataName( const char* nm, int idx )
{
    if ( statnmcb_ )
    {
	idx > statnmcb_->size()-1 ? statnmcb_->addItem(toUiString(nm))
			 : statnmcb_->setItemText(idx,toUiString(nm));
	return;
    }

    if ( disps_.validIdx(idx) )
	disps_[idx]->setDataName( nm );
}


void uiStatsDisplayWin::setMarkValue( float val, bool forx, int idx )
{
    if ( disps_.validIdx(idx) )
	disps_[idx]->setMarkValue( val, forx );
}
