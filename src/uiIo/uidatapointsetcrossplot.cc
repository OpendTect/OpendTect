/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplot.cc,v 1.11 2008-06-26 16:18:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidatapointsetcrossplotwin.h"
#include "uidpscrossplotpropdlg.h"
#include "uidatapointset.h"
#include "datapointset.h"
#include "uiaxishandler.h"
#include "uirgbarray.h"
#include "uitoolbar.h"
#include "uispinbox.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "iodrawtool.h"
#include "randcolor.h"
#include "statruncalc.h"
#include "linear.h"
#include "sorting.h"
#include "draw.h"
#include "settings.h"
#include "envvars.h"
#include "uimsg.h"

static const int cMaxPtsForMarkers = 20000;
static const int cMaxPtsForDisplay = 100000;
float uiDataPointSetCrossPlotter::AutoScalePars::defclipratio_ = -1;


uiDataPointSetCrossPlotter::Setup uiDataPointSetCrossPlotWin::defsetup_;

uiDataPointSetCrossPlotter::Setup::Setup()
    : noedit_(false)
    , markerstyle_(MarkerStyle2D::Square)
    , xstyle_(LineStyle::Solid,1,Color::Black)
    , ystyle_(LineStyle::Solid,1,Color::stdDrawColor(0))
    , y2style_(LineStyle::Dot,1,Color::stdDrawColor(1))
    , minborder_(10,20,20,5)
{
}


uiDataPointSetCrossPlotter::AutoScalePars::AutoScalePars()
    : doautoscale_(true)
{
    if ( defclipratio_ == -1 )
    {
	const char* res = Settings::common().find( "AxisData.Clip Ratio" );
	const float val = res && *res ? atof( res )
		        : GetEnvVarDVal( "OD_DEFAULT_AXIS_CLIPRATIO", 0 );
	defclipratio_ = val < 0 || val >= 1 ? 0 : val;
    }
    clipratio_ = defclipratio_;
}


uiDataPointSetCrossPlotter::AxisData::AxisData( uiDataPointSetCrossPlotter& cp,
						uiRect::Side s )
    : cp_(cp)
    , axis_(0)
    , defaxsu_(s)
    , rg_(0,1)
{
    stop();
}


uiDataPointSetCrossPlotter::AxisData::~AxisData()
{
    delete axis_;
}


void uiDataPointSetCrossPlotter::AxisData::stop()
{
    colid_ = cp_.mincolid_ - 1;
    needautoscale_ = false;
    if ( !axis_ ) return;

    defaxsu_ = axis_->setup();
    delete axis_; axis_ = 0;
}


void uiDataPointSetCrossPlotter::AxisData::setCol( DataPointSet::ColID cid )
{
    if ( axis_ && cid == colid_ )
	return;

    colid_ = cid;
    newColID();
}


void uiDataPointSetCrossPlotter::AxisData::newColID()
{
    const DataPointSet::ColID cid = colid_;
    stop();
    colid_ = cid;
    if ( colid_ < cp_.mincolid_ )
	return;

    axis_ = new uiAxisHandler( cp_.drawTool(), defaxsu_ );
    axis_->setName( cp_.uidps_.userName(colid_) );
    needautoscale_ = true;
    handleAutoScale();
}


void uiDataPointSetCrossPlotter::AxisData::newDevSize()
{
    if ( axis_ ) axis_->newDevSize();
}


void uiDataPointSetCrossPlotter::AxisData::handleAutoScale()
{
    if ( !axis_ || !needautoscale_ || !autoscalepars_.doautoscale_ )
	return;

    const Stats::RunCalc<float>& rc = cp_.uidps_.getRunCalc( colid_ );
    rg_ = Interval<float>( rc.min(), rc.max() );
    if ( !mIsZero(autoscalepars_.clipratio_,1e-5) )
    {
	rg_.start = rc.clipVal( autoscalepars_.clipratio_, false );
	rg_.stop = rc.clipVal( autoscalepars_.clipratio_, true );
    }

    AxisLayout al( rg_ );
    axis_->setRange( StepInterval<float>( al.sd.start, al.stop, al.sd.step ) );
    needautoscale_ = false;
}


uiDataPointSetCrossPlotter::uiDataPointSetCrossPlotter( uiParent* p,
			    uiDataPointSet& uidps,
			    const uiDataPointSetCrossPlotter::Setup& su )
    : uiRGBArrayCanvas(p,*new uiRGBArray(false))
    , dps_(uidps.pointSet())
    , uidps_(uidps)
    , setup_(su)
    , meh_(getMouseEventHandler())
    , mincolid_(1-uidps.pointSet().nrFixedCols())
    , selrow_(-1)
    , x_(*this,uiRect::Bottom)
    , y_(*this,uiRect::Left)
    , y2_(*this,uiRect::Right)
    , selectionChanged( this )
    , removeRequest( this )
    , doy2_(true)
    , dobd_(false)
    , eachrow_(1)
    , curgrp_(0)
    , selrowisy2_(false)
    , lsy1_(*new LinStats2D)
    , lsy2_(*new LinStats2D)
{
    dodraw_ = false;
    x_.defaxsu_.style_ = setup_.xstyle_;
    y_.defaxsu_.style_ = setup_.ystyle_;
    y2_.defaxsu_.style_ = setup_.y2style_;
    x_.defaxsu_.border_ = setup_.minborder_;
    y_.defaxsu_.border_ = setup_.minborder_;
    y2_.defaxsu_.border_ = setup_.minborder_;

    meh_.buttonPressed.notify( mCB(this,uiDataPointSetCrossPlotter,mouseClick));
    meh_.buttonReleased.notify( mCB(this,uiDataPointSetCrossPlotter,mouseRel));
    preDraw.notify( mCB(this,uiDataPointSetCrossPlotter,initDraw) );
    postDraw.notify( mCB(this,uiDataPointSetCrossPlotter,drawContent) );

    setPrefWidth( 600 );
    setPrefHeight( 500 );
    setStretch( 2, 2 );
}


uiDataPointSetCrossPlotter::~uiDataPointSetCrossPlotter()
{
    delete &rgbarr_;
    delete &lsy1_;
    delete &lsy2_;
}


void uiDataPointSetCrossPlotter::dataChanged()
{
    x_.handleAutoScale(); y_.handleAutoScale(); y2_.handleAutoScale();
    calcStats();
    update();
}


static void updLS( const TypeSet<float>& inpxvals,
		   const TypeSet<float>& inpyvals,
		   const uiDataPointSetCrossPlotter::AxisData& axdx,
		   const uiDataPointSetCrossPlotter::AxisData& axdy,
		   LinStats2D& ls )
{
    const int inpsz = inpxvals.size();
    if ( inpsz < 2 ) { ls = LinStats2D(); return; }

    int firstxidx = 0, firstyidx = 0;
    if ( axdx.autoscalepars_.doautoscale_ && axdy.autoscalepars_.doautoscale_ )
    {
	firstxidx = (int)(axdx.autoscalepars_.clipratio_ * inpsz + .5);
	firstyidx = (int)(axdy.autoscalepars_.clipratio_ * inpsz + .5);
	if ( firstxidx < 1 && firstyidx < 1 )
	{
	    ls.use( inpxvals.arr(), inpyvals.arr(), inpsz );
	    return;
	}
    }

    Interval<float> xrg; assign( xrg, axdx.axis_->range() );
    if ( firstxidx > 0 )
    {
	TypeSet<float> sortvals( inpxvals );
	sortFor( sortvals.arr(), inpsz, firstxidx );
	xrg.start = sortvals[firstxidx]; xrg.stop = sortvals[inpsz-firstxidx-1];
    }
    Interval<float> yrg; assign( yrg, axdy.axis_->range() );
    if ( firstyidx > 0 )
    {
	TypeSet<float> sortvals( inpyvals );
	sortFor( sortvals.arr(), inpsz, firstyidx );
	yrg.start = sortvals[firstyidx]; yrg.stop = sortvals[inpsz-firstyidx-1];
    }

    TypeSet<float> xvals, yvals;
    for ( int idx=0; idx<inpxvals.size(); idx++ )
    {
	const float x = inpxvals[idx]; const float y = inpyvals[idx];
	if ( xrg.includes(x) && yrg.includes(y) )
	    { xvals += x; yvals += y; }
    }

    ls.use( xvals.arr(), yvals.arr(), xvals.size() );
}


void uiDataPointSetCrossPlotter::calcStats()
{
    if ( !x_.axis_ || (!y_.axis_ && !y2_.axis_) )
	return;

    const Interval<int> udfrg( 0, 1 );
    const Interval<int> xpixrg( x_.axis_->pixRange() ),
	  		ypixrg( y_.axis_ ? y_.axis_->pixRange() : udfrg ),
			y2pixrg( y2_.axis_ ? y2_.axis_->pixRange() : udfrg );
    TypeSet<float> xvals, yvals, x2vals, y2vals;

    for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
    {
	if ( dps_.isInactive(rid)
	  || (curgrp_ > 0 && dps_.group(rid) != curgrp_) )
	    continue;

	const float xval = uidps_.getVal( x_.colid_, rid, true );
	if ( mIsUdf(xval) ) continue;

	if ( y_.axis_ )
	{
	    const float yval = uidps_.getVal( y_.colid_, rid, true );
	    if ( !mIsUdf(yval) )
		{ xvals += xval; yvals += yval; }
	}
	if ( y2_.axis_ )
	{
	    const float yval = uidps_.getVal( y2_.colid_, rid, true );
	    if ( !mIsUdf(yval) )
		{ x2vals += xval; y2vals += yval; }
	}
    }

    updLS( xvals, yvals, x_, y_, lsy1_ );
    updLS( x2vals, y2vals, x_, y2_, lsy2_ );
}


bool uiDataPointSetCrossPlotter::selNearest( const MouseEvent& ev )
{
    const uiPoint pt( ev.pos() );
    selrow_ = getRow( y_, pt );
    selrowisy2_ = selrow_ < 0;
    if ( selrowisy2_ )
	selrow_ = getRow( y2_, pt );
    return selrow_ >= 0;
}


static const float cRelTol = 0.01; // 1% of axis size


int uiDataPointSetCrossPlotter::getRow(
	const uiDataPointSetCrossPlotter::AxisData& ad, uiPoint pt ) const
{
    if ( !ad.axis_ ) return -1;

    const float xpix = x_.axis_->getRelPos( x_.axis_->getVal(pt.x) );
    const float ypix = ad.axis_->getRelPos( ad.axis_->getVal(pt.y) );
    int row = -1; float mindistsq = 1e30;
    for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
    {
	if ( rid % eachrow_ || (curgrp_ > 0 && dps_.group(rid) != curgrp_) )
	    continue;

	const float xval = uidps_.getVal( x_.colid_, rid, true );
	const float yval = uidps_.getVal( ad.colid_, rid, true );
	if ( mIsUdf(xval) || mIsUdf(yval) ) continue;

	const float x = x_.axis_->getRelPos( xval );
	const float y = ad.axis_->getRelPos( yval );
	const float distsq = (x-xpix)*(x-xpix) + (y-ypix)*(y-ypix);
	if ( distsq < mindistsq )
	    { row = rid; mindistsq = distsq; }
    }

    return mindistsq < cRelTol*cRelTol ? row : -1;
}


void uiDataPointSetCrossPlotter::mouseClick( CallBacker* cb )
{
    if ( meh_.isHandled() ) return;

    const MouseEvent& ev = meh_.event();
    const bool isnorm = !ev.ctrlStatus() && !ev.shiftStatus() &&!ev.altStatus();

    if ( isnorm && selNearest(ev) )
    {
	selectionChanged.trigger();
	meh_.setHandled( true );
    }
}


void uiDataPointSetCrossPlotter::mouseRel( CallBacker* cb )
{
    if ( meh_.isHandled() ) return;

    const MouseEvent& ev = meh_.event();
    const bool isdel = ev.ctrlStatus() && !ev.shiftStatus() && !ev.altStatus();

    if ( !setup_.noedit_ && isdel && selNearest(ev) )
    {
	removeRequest.trigger();
	meh_.setHandled( true );
	selrow_ = -1;
    }
}


void uiDataPointSetCrossPlotter::setCols( DataPointSet::ColID x,
		    DataPointSet::ColID y, DataPointSet::ColID y2 )
{
    if ( y < mincolid_ && y2 < mincolid_ )
	x = mincolid_ - 1;
    if ( x < mincolid_ )
	y = y2 = mincolid_ - 1;

    x_.setCol( x ); y_.setCol( y ); y2_.setCol( y2 );

    if ( y_.axis_ )
    {
	y_.axis_->setBegin( x_.axis_ );
	x_.axis_->setBegin( y_.axis_ );
    }
    else if ( x_.axis_ )
	x_.axis_->setBegin( 0 );

    if ( y2_.axis_ )
    {
	y2_.axis_->setBegin( x_.axis_ );
	x_.axis_->setEnd( y2_.axis_ );
    }
    else if ( x_.axis_ )
	x_.axis_->setEnd( 0 );

    x_.handleAutoScale(); y_.handleAutoScale(); y2_.handleAutoScale();
    calcStats();

    update();
}


void uiDataPointSetCrossPlotter::initDraw( CallBacker* cb )
{
    x_.newDevSize();
    y_.newDevSize();
    y2_.newDevSize();
}


void uiDataPointSetCrossPlotter::mkNewFill()
{
    if ( !dobd_ ) return;
}


void uiDataPointSetCrossPlotter::drawContent( CallBacker* cb )
{
    if ( x_.axis_ )
	x_.axis_->plotAxis();
    if ( y_.axis_ )
	y_.axis_->plotAxis();
    if ( doy2_ && y2_.axis_ )
	y2_.axis_->plotAxis();

    if ( !x_.axis_ || !y_.axis_ )
	return;

    if ( y2_.axis_ && doy2_ ) drawData( y2_ );
    drawData( y_ );
}


void uiDataPointSetCrossPlotter::drawData(
			    const uiDataPointSetCrossPlotter::AxisData& yad )
{
    const uiAxisHandler& xah = *x_.axis_;
    const uiAxisHandler& yah = *yad.axis_;
    drawTool().setPenColor( yah.setup().style_.color_ );

    int estpts = dps_.size() / eachrow_;
    if ( curgrp_ > 0 ) estpts = cMaxPtsForMarkers;

    MarkerStyle2D mstyle( setup_.markerstyle_ );
    if ( estpts > cMaxPtsForMarkers )
	mstyle.type_ = MarkerStyle2D::None;
    mstyle.color_ = yah.setup().style_.color_;

    const Interval<int> xpixrg( xah.pixRange() ), ypixrg( yah.pixRange() );

    int nrptsdisp = 0;
    Interval<int> usedxpixrg, usedypixrg;
    for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
    {
	if ( rid % eachrow_ || dps_.isInactive(rid)
	  || (curgrp_ > 0 && dps_.group(rid) != curgrp_) )
	    continue;

	const float xval = uidps_.getVal( x_.colid_, rid, true );
	const float yval = uidps_.getVal( yad.colid_, rid, true );
	if ( mIsUdf(xval) || mIsUdf(yval) ) continue;

	const uiPoint pt( xah.getPix(xval), yah.getPix(yval) );
	if ( !xpixrg.includes(pt.x) || !ypixrg.includes(pt.y) )
	    continue;

	const bool issel = dps_.isSelected( rid );
	if ( mstyle.type_ == MarkerStyle2D::None )
	    drawTool().drawPoint( pt, issel );
	else
	{
	    mstyle.size_ = issel ? 4 : 2;
	    drawTool().drawMarker( pt, mstyle );
	}

	nrptsdisp++;
	if ( nrptsdisp > 1 )
	    { usedxpixrg.include( pt.x ); usedypixrg.include( pt.y ); }
	else
	{
	    usedxpixrg = Interval<int>( pt.x, pt.x );
	    usedypixrg = Interval<int>( pt.y, pt.y );
	}

    }

    if ( nrptsdisp < 1 )
	return;

    if ( setup_.showcc_ )
    {
	int r100 = (int)((y_.axis_ == &yah ? lsy1_ : lsy2_)
					    .corrcoeff * 100 + .5);
	BufferString txt( "r=" );
	if ( r100 == 0 )	txt += "0";
	else if ( r100 == 100 )	txt += "1";
	else
	{
	    txt += "0.";
	    txt += r100%10 ? r100 : r100 / 10;
	}
	yah.annotAtEnd( txt );
    }

    if ( setup_.showregrline_ )
	drawRegrLine( yah, usedxpixrg, usedypixrg );
}


void uiDataPointSetCrossPlotter::drawRegrLine( const uiAxisHandler& yah,
				    Interval<int> xpixrg, Interval<int> ypixrg )
{
    const uiAxisHandler& xah = *x_.axis_;
    const LinStats2D& ls = y_.axis_ == &yah ? lsy1_ : lsy2_;
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    Interval<float> yvalrg( yah.getVal(ypixrg.start), yah.getVal(ypixrg.stop) );

    uiPoint from(xpixrg.start,ypixrg.start), to(xpixrg.stop,ypixrg.stop);
    if ( ls.lp.ax == 0 )
    {
	const int ypix = yah.getPix( ls.lp.a0 );
	if ( !ypixrg.includes( ypix ) ) return;
	from.x = xpixrg.start; to.x = xpixrg.stop;
	from.y = to.y = ypix;
    }
    else
    {
	const float xx0 = xvalrg.start; const float yx0 = ls.lp.getValue( xx0 );
	const float xx1 = xvalrg.stop; const float yx1 = ls.lp.getValue( xx1 );
	const float yy0 = yvalrg.start; const float xy0 = ls.lp.getXValue( yy0);
	const float yy1 = yvalrg.stop; const float xy1 = ls.lp.getXValue( yy1 );

	const bool yx0hi = yx0 > yvalrg.stop;
	const bool yx0lo = yx0 < yvalrg.start;
	const bool yx1hi = yx1 > yvalrg.stop;
	const bool yx1lo = yx1 < yvalrg.start;
	const bool xy0hi = xx0 > xvalrg.stop;
	const bool xy0lo = xx0 < xvalrg.start;
	const bool xy1hi = xx1 > xvalrg.stop;
	const bool xy1lo = xx1 < xvalrg.start;
	if ( (yx0hi && yx1hi) || (yx0lo && yx1lo) )
	    return; // line doesn't cross rectangle
	if ( !yx0hi && !yx1hi && !yx0lo && !yx1lo )
	    { from.y = yah.getPix( yx0 ); to.x = yah.getPix( yx1 ); }
	    // line goes through left and right sides
	else if ( !xy0hi && !xy1hi && !xy0lo && !xy1lo )
	    { from.x = xah.getPix( xy0 ); to.x = xah.getPix( xy1 ); }
	    // line goes through top and bottom
	else
	{
	    // line goes through two perpendicular sides
	    if ( yx0lo )
		{ from.x = xah.getPix( xy0 ); to.y = yah.getPix( yx1 ); }
	    else if ( yx0hi )
		{ from.x = xah.getPix( xy1 ); to.y = yah.getPix( yx1 ); }
	    else if ( xy0lo )
		{ from.y = yah.getPix( yx0 ); to.x = xah.getPix( xy1 ); }
	    else if ( xy1lo )
		{ from.y = yah.getPix( yx0 ); to.x = xah.getPix( xy0 ); }
	}
    }

    drawTool().drawLine( from, to );
}


uiDataPointSetCrossPlotWin::uiDataPointSetCrossPlotWin( uiDataPointSet& uidps )
    : uiMainWin(&uidps,BufferString(uidps.pointSet().name()," Cross-plot"),
	    			    2,false)
    , uidps_(uidps)
    , plotter_(*new uiDataPointSetCrossPlotter(this,uidps,defsetup_))
    , disptb_(*new uiToolBar(this,"Crossplot display toolbar"))
    , maniptb_(*new uiToolBar(this,"Crossplot manipulation toolbar"))
    , rbissel_(true)
    , grpfld_(0)
{
    windowClosed.notify( mCB(this,uiDataPointSetCrossPlotWin,closeNotif) );

    const int nrpts = uidps.pointSet().size();
    const int eachrow = 1 + nrpts / cMaxPtsForDisplay;
    uiGroup* grp = new uiGroup( &disptb_, "Each grp" );
    eachfld_ = new uiSpinBox( grp, 0, "Each" );
    eachfld_->setValue( eachrow );
    eachfld_->setInterval( 1, mUdf(int), 1 );
    eachfld_->valueChanged.notify(
	    		mCB(this,uiDataPointSetCrossPlotWin,eachChg) );
    new uiLabel( grp, "Plot each", eachfld_ );
    disptb_.addObject( grp->attachObj() );
    plotter_.eachrow_ = eachrow;

    showy2tbid_ = disptb_.addButton( "showy2.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,showY2),
		  "Toggle show Y2", true );
    disptb_.turnOn( showy2tbid_, plotter_.doy2_ );

/*
    disptb_.addButton( "zoombackward.png",
			mCB(this,uiDataPointSetCrossPlotWin,unZoom),
			"Unzoom", false );

    selzoomtbid_ = maniptb_.addButton( "selzoom.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,selZoom),
		      "Toggle zoom/select", true );
    maniptb_.turnOn( selzoomtbid_, rbissel_ );
    maniptb_.addButton( "delsel.png",
			mCB(this,uiDataPointSetCrossPlotWin,delSel),
			"Delete selected", false );
*/
    maniptb_.addButton( "xplotprop.png",
			mCB(this,uiDataPointSetCrossPlotWin,editProps),
			"Properties", false );

    if ( uidps_.groupNames().size() > 1 )
    {
	grpfld_ = new uiComboBox( grp, "Group selection" );
	BufferString txt( "All " ); txt += uidps_.groupType(); txt += "s";
	grpfld_->addItem( txt );
	for ( int idx=0; idx<uidps_.groupNames().size(); idx++ )
	    grpfld_->addItem( uidps_.groupNames().get(idx) );
	grpfld_->attach( rightOf, eachfld_ );
	grpfld_->setCurrentItem( 0 );
	grpfld_->selectionChanged.notify(
			    mCB(this,uiDataPointSetCrossPlotWin,grpChg) );
    }

    setPrefWidth( 600 );
    setPrefHeight( 500 );
}


void uiDataPointSetCrossPlotWin::closeNotif( CallBacker* )
{
    defsetup_ = plotter_.setup();
    plotter_.eachrow_ = mUdf(int); // Make sure eachChg knows we are closing
}


void uiDataPointSetCrossPlotWin::showY2( CallBacker* )
{
    plotter_.doy2_ = disptb_.isOn( showy2tbid_ );
    plotter_.update();
}


void uiDataPointSetCrossPlotWin::showBD( CallBacker* )
{
    // plotter_.dobd_ = disptb_.isOn( showbdtbid_ );
    plotter_.update();
    uiMSG().message( "Under construction: data density backdrop" );
}


void uiDataPointSetCrossPlotWin::eachChg( CallBacker* )
{
    if ( mIsUdf(plotter_.eachrow_) ) return; // window is closing

    int neweachrow = eachfld_->getValue();
    if ( neweachrow < 1 ) neweachrow = 1;
    plotter_.eachrow_ = neweachrow;
    plotter_.update();
}


void uiDataPointSetCrossPlotWin::grpChg( CallBacker* )
{
    if ( !grpfld_ ) return;
    plotter_.curgrp_ = grpfld_->currentItem();
    plotter_.update();
}


void uiDataPointSetCrossPlotWin::selZoom( CallBacker* )
{
    rbissel_ = maniptb_.isOn( selzoomtbid_ );
}


void uiDataPointSetCrossPlotWin::delSel( CallBacker* )
{
}


void uiDataPointSetCrossPlotWin::unZoom( CallBacker* )
{
}


void uiDataPointSetCrossPlotWin::editProps( CallBacker* )
{
    uiDataPointSetCrossPlotterPropDlg dlg( &plotter_ );
    dlg.go();
}
