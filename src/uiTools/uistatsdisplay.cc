/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistatsdisplay.h"

#include "uiaxishandler.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uihistogramdisplay.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistatsdisplaywin.h"

#include "arrayndimpl.h"
#include "bufstring.h"
#include "datapointset.h"
#include "draw.h"
#include "mouseevent.h"
#include "statparallelcalc.h"

#define mPutCountInPlot() (histgramdisp_ && setup_.countinplot_)


// uiStatsDisplay::Setup

uiStatsDisplay::Setup::Setup()
    : withplot_(true)
    , withname_(true)
    , withtext_(true)
    , vertaxis_(true)
    , countinplot_(false)
{}


uiStatsDisplay::Setup::~Setup()
{}


// uiStatsDisplay

uiStatsDisplay::uiStatsDisplay( uiParent* p, const uiStatsDisplay::Setup& su )
    : uiGroup(p,"Statistics display group")
    , setup_(su)
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
	auto* valgrp = new uiGroup( this, "Values group" );
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

    mAttachCB( postFinalize(), uiStatsDisplay::finalizeCB );
}


uiStatsDisplay::~uiStatsDisplay()
{
    detachAllNotifiers();
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


bool uiStatsDisplay::setDataPackID( const DataPackID& dpid,
				    const DataPackMgr::MgrID& dmid,
				    int version )
{
    ConstRefMan<DataPack> datapack = DPM( dmid ).getDP( dpid );
    return datapack ? setDataPack( *datapack.ptr(), version ) :  false;
}


bool uiStatsDisplay::setDataPack( const DataPack& dp, int version )
{
    if ( !histgramdisp_ ||
	 (histgramdisp_ && !histgramdisp_->setDataPack(dp,version)) )
    {
	Stats::ParallelCalc<float> rc( (Stats::CalcSetup()
						.require(Stats::Min)
						.require(Stats::Max)
						.require(Stats::Average)
						.require(Stats::Median)
						.require(Stats::StdDev)
						.require(Stats::RMS)) );

	mDynamicCastGet(const SeisDataPack*,seisdp,&dp)
	mDynamicCastGet(const FlatDataPack*,fdp,&dp)
	mDynamicCastGet(const DataPointSet*,dps,&dp)
	TypeSet<float> valarr;
	if ( seisdp )
	{
	    const Array3D<float>& arr3d = seisdp->data( version );
	    if ( !arr3d.isOK() )
		return false;

	    const float* array = arr3d.getData();
	    if ( array )
		rc.setValues( array, int(arr3d.info().getTotalSz()) );
	    else
	    {
		valarr.setCapacity( int(arr3d.info().getTotalSz()), false );
		const int sz0 = arr3d.info().getSize( 0 );
		const int sz1 = arr3d.info().getSize( 1 );
		const int sz2 = arr3d.info().getSize( 2 );
		for ( int idx=0; idx<sz0; idx++ )
		{
		    for ( int idy=0; idy<sz1; idy++ )
		    {
			for ( int idz=0; idz<sz2; idz++ )
			{
			    const float val = arr3d.get( idx, idy, idz );
			    if ( !mIsUdf(val) )
				valarr += val;
			}
		    }
		}

		rc.setValues( valarr.arr(), valarr.size() );
	    }
	}
	else if ( fdp )
	{
	    mDynamicCastGet(const MapDataPack*,mdp,fdp);
	    const Array2D<float>& data = mdp ? mdp->rawData() : fdp->data();
	    if ( data.getData() )
		rc.setValues( data.getData(),
				int(data.info().getTotalSz()) );
	    else
	    {
		valarr.setCapacity( int(data.info().getTotalSz()), false );
		const int sz2d0 = data.info().getSize( 0 );
		const int sz2d1 = data.info().getSize( 1 );
		for ( int idx0=0; idx0<sz2d0; idx0++ )
		{
		    for ( int idx1=0; idx1<sz2d1; idx1++ )
		    {
			const float val = data.get( idx0, idx1 );
			if ( mIsUdf(val) )
			    continue;

			valarr += val;
		    }
		}

		rc.setValues( valarr.arr(), valarr.size() );
	    }
	}
	else if ( dps )
	{
	    const int colid = dps->nrCols() - 1;
	    valarr.setCapacity( dps->size(), false );
	    for ( int idx=0; idx<dps->size(); idx++ )
		valarr += dps->value( colid, idx );

	    rc.setValues( valarr.arr(), valarr.size() );
	}
	else
	    return false;

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


uiStatsDisplayWin::~uiStatsDisplayWin()
{}


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

        val = disp->getFuncXY( pos.x_, false );
	break;
    }

    uiString str = tr("Value / Count:  %1 / %2").arg(toUiString(val.x_,4)).
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
