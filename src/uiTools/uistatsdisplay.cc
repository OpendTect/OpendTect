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
#include "mouseevent.h"
#include "statparallelcalc.h"

#define mPutCountInPlot() (histgramdisp_ && setup_.countinplot_)


uiStatsDisplay::uiStatsDisplay( uiParent* p, const uiStatsDisplay::Setup& su )
    : uiGroup( p, "Statistics display group" )
    , setup_(su)
    , histgramdisp_(nullptr)
    , minmaxfld_(nullptr)
    , countfld_(nullptr)
    , namefld_(nullptr)
{
    const bool putcountinplot = mPutCountInPlot();
    if ( setup_.withplot_ )
    {
	uiHistogramDisplay::Setup fsu;
	fsu.annoty( setup_.vertaxis_ );
	histgramdisp_ = new uiHistogramDisplay( this, fsu );
    }

    if ( setup_.withtext_ )
    {
	uiGroup* valgrp = new uiGroup( this, "Values group" );
	if ( setup_.withname_ )
	{
	    namefld_ = new uiLabel( this, tr("Data Name") );
	    namefld_->setStretch( 2, 0 );
	    if ( setup_.withplot_ )
		histgramdisp_->attach( alignedBelow, namefld_ );
	    else
		valgrp->attach( alignedBelow, namefld_ );
	}

	minmaxfld_ = new uiGenInput( valgrp, tr("Value range"),
				     FloatInpSpec(), FloatInpSpec() );
	minmaxfld_->setReadOnly();
	avgstdfld_ = new uiGenInput( valgrp, tr("Mean/Std deviation"),
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

	valgrp->setHAlignObj( minmaxfld_ );

	if ( histgramdisp_ )
	    valgrp->attach( leftAlignedBelow, histgramdisp_ );
    }

    if ( putcountinplot )
	putN();

    postFinalise().notify( mCB(this,uiStatsDisplay,finalizeCB) );
}


void uiStatsDisplay::finalizeCB( CallBacker* )
{
    if ( !setup_.withtext_ )
	return;

    const int nrdec = 4;
    minmaxfld_->setNrDecimals( nrdec, 0 );
    minmaxfld_->setNrDecimals( nrdec, 1 );
    avgstdfld_->setNrDecimals( nrdec, 0 );
    avgstdfld_->setNrDecimals( nrdec, 1 );
    medrmsfld_->setNrDecimals( nrdec, 0 );
    medrmsfld_->setNrDecimals( nrdec, 1 );
}


void uiStatsDisplay::setDataName( const char* nm )
{
    if ( !namefld_ )
	return;

    namefld_->setText( toUiString(nm) );
    namefld_->setAlignment( Alignment::HCenter );
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
	ConstDataPackRef<DataPack> datapack = dpman.obtain( dpid );
	if ( !datapack ) return false;

	TypeSet<float> valarr;
	if ( dmid == DataPackMgr::SeisID() )
	{
	    mDynamicCastGet(const SeisDataPack*,sdp,datapack.ptr());
	    const Array3D<float>* arr3d = sdp ? &sdp->data(version) : 0;
	    if ( !arr3d ) return false;

	    const float* array = arr3d->getData();
	    if ( array )
		rc.setValues( array, int(arr3d->info().getTotalSz()) );
	    else
	    {
		valarr.setCapacity( int(arr3d->info().getTotalSz()), false );
		const int sz0 = arr3d->info().getSize( 0 );
		const int sz1 = arr3d->info().getSize( 1 );
		const int sz2 = arr3d->info().getSize( 2 );
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
	    mDynamicCastGet(const MapDataPack*,mdp,datapack.ptr());
	    if ( mdp )
		array = &mdp->rawData();
	    else if ( fdp )
		array = &fdp->data();

	    if ( !array )
		return false;

	    if ( array->getData() )
		rc.setValues( array->getData(),
				int(array->info().getTotalSz()) );
	    else
	    {
		valarr.setCapacity( int(array->info().getTotalSz()), false );
		const int sz2d0 = array->info().getSize( 0 );
		const int sz2d1 = array->info().getSize( 1 );
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


void uiStatsDisplay::setData( const float* array, int sz )
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


void uiStatsDisplay::setMarkValue( float val, bool forx )
{
    if ( histgramdisp_ )
	histgramdisp_->setMarkValue( val, forx );
}


void uiStatsDisplay::putN()
{
    if ( histgramdisp_ )
	histgramdisp_->putN();
}


// uiStatsDisplayWin
uiStatsDisplayWin::uiStatsDisplayWin( uiParent* p,
					const uiStatsDisplay::Setup& su,
					int nr, bool ismodal )
    : uiMainWin(p,uiStrings::phrData(uiStrings::sStatistics()),1,false,ismodal)
    , statnmcb_(0)
    , currentdispidx_(-1)
{
    if ( su.withplot_ && nr <= 8 )
    {
	const int maxonrow = 4;
	const int nrrows = mNINT32( Math::Ceil(float(nr)/maxonrow) );
	const int nrcols = mNINT32( Math::Ceil(float(nr)/nrrows) );
	const int total = nrrows * nrcols;

	uiSeparator* sep = nullptr;
	for ( int idx=0; idx<total; idx++ )
	{
	    auto* disp = new uiStatsDisplay( this, su );
	    disp->setName( BufferString("Statistics Group ",idx) );
	    if ( disp->funcDisp() )
		disp->funcDisp()->getMouseEventHandler().movement.notify(
				mCB(this,uiStatsDisplayWin,mouseMoveCB) );
	    disps_ += disp;
	    if ( idx==0 )
		continue;

	    if ( idx==nrcols )
	    {
		sep = new uiSeparator( this, "Horizontal Separator" );
		sep->attach( stretchedBelow, disps_[0] );
	    }

	    if ( idx>=nrcols)
	    {
		disp->attach( alignedBelow, disps_[idx-nrcols] );
		disp->attach( ensureBelow, sep );
	    }

	    if ( idx%nrcols != 0 )
		disp->attach( rightTo, disps_[idx-1] );

	    disp->display( idx<nr );
	}

	return;
    }

    uiStatsDisplay::Setup sucopy( su );
    uiLabeledComboBox* lblcb=0;
    if ( nr > 1 )
    {
	lblcb = new uiLabeledComboBox( this, tr("Select data") );
	statnmcb_ = lblcb->box();
	statnmcb_->setHSzPol( uiObject::MedVar );
	sucopy.withname(false);
    }

    for ( int idx=0; idx<nr; idx++ )
    {
	auto* disp = new uiStatsDisplay( this, sucopy );
	if ( statnmcb_ )
	    disp->attach( ensureBelow, lblcb );

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

    Geom::Point2D<float> val;
    const Geom::Point2D<int>& pos = meh->event().pos();
    for ( int idx=0; idx<disps_.size(); idx++ )
    {
	uiHistogramDisplay* disp = disps_[idx]->funcDisp();
	if ( !disp || meh != &disp->getMouseEventHandler() )
	     continue;

	val = disp->getFuncXY( pos.x, false );
	break;
    }

    uiString str = tr("Value / Count:  %1 / %2").arg(toUiString(val.x,4)).
		   arg(toUiString(val.y,0));
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
{
    if ( statnmcb_ )
	statnmcb_->addItems( nms );
}


void uiStatsDisplayWin::setDataName( const char* nm, int idx )
{
    if ( statnmcb_ )
    {
	idx>statnmcb_->size()-1 ? statnmcb_->addItem( toUiString(nm) )
				: statnmcb_->setItemText( idx, toUiString(nm) );
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
