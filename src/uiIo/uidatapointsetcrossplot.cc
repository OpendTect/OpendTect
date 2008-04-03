/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplot.cc,v 1.1 2008-04-03 08:28:30 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidatapointsetcrossplotwin.h"
#include "uidatapointset.h"
#include "datapointset.h"
#include "uiaxishandler.h"
#include "uirgbarray.h"
#include "uitoolbar.h"
#include "uispinbox.h"
#include "uilabel.h"
#include "iodrawtool.h"
#include "randcolor.h"
#include "statruncalc.h"
#include "linear.h"
#include "draw.h"
#include "uimsg.h"

static const int cMaxPtsForMarkers = 20000;
static const int cMaxPtsForDisplay = 100000;


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


uiDataPointSetCrossPlotter::AxisData::AxisData( uiDataPointSetCrossPlotter& cp,
						uiRect::Side s )
    : cp_(cp)
    , axis_(0)
    , defsu_(s)
    , rg_(0,1,1)
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

    defsu_ = axis_->setup();
    delete axis_; axis_ = 0;
}


void uiDataPointSetCrossPlotter::AxisData::setCol( DataPointSet::ColID cid )
{
    if ( axis_ && cid == colid_ )
	return;

    stop();
    colid_ = cid;
    if ( colid_ < cp_.mincolid_ )
	return;

    axis_ = new uiAxisHandler( cp_.drawTool(), defsu_ );
    axis_->setName( cp_.uidps_.userName(colid_) );
    needautoscale_ = true;
}


void uiDataPointSetCrossPlotter::AxisData::newDevSize()
{
    if ( axis_ ) axis_->newDevSize();
}


void uiDataPointSetCrossPlotter::AxisData::handleAutoScale()
{
    if ( !axis_ || !needautoscale_ ) return;

    const Stats::RunCalc<float>& rc = cp_.uidps_.getRunCalc( colid_ );
    AxisLayout al( Interval<float>(rc.min(),rc.max()) );
    rg_ = StepInterval<float>( al.sd.start, al.stop, al.sd.step );

    axis_->setRange( rg_ );
}


uiDataPointSetCrossPlotter::uiDataPointSetCrossPlotter( uiParent* p,
			    uiDataPointSet& uidps,
			    const uiDataPointSetCrossPlotter::Setup& su )
    : uiRGBArrayCanvas(p,*new uiRGBArray)
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
    , isy2_(false)
    , doy2_(true)
    , dobd_(false)
    , eachrow_(1)
{
    dodraw_ = false;
    x_.defsu_.style_ = setup_.xstyle_;
    y_.defsu_.style_ = setup_.ystyle_;
    y2_.defsu_.style_ = setup_.y2style_;
    x_.defsu_.border_ = setup_.minborder_;
    y_.defsu_.border_ = setup_.minborder_;
    y2_.defsu_.border_ = setup_.minborder_;

    meh_.buttonPressed.notify( mCB(this,uiDataPointSetCrossPlotter,mouseClick));
    meh_.buttonReleased.notify( mCB(this,uiDataPointSetCrossPlotter,mouseRel));
    preDraw.notify( mCB(this,uiDataPointSetCrossPlotter,initDraw) );
    postDraw.notify( mCB(this,uiDataPointSetCrossPlotter,drawContent) );

    if ( dps_.size() > cMaxPtsForMarkers )
	setup_.markerstyle_.type_ = MarkerStyle2D::None;

    setPrefWidth( 600 );
    setPrefHeight( 500 );
    setStretch( 2, 2 );
}


uiDataPointSetCrossPlotter::~uiDataPointSetCrossPlotter()
{
    delete &rgbarr_;
}


bool uiDataPointSetCrossPlotter::selNearest( const MouseEvent& ev )
{
    const uiPoint pt( ev.pos() );
    selrow_ = getRow( y_, pt );
    isy2_ = selrow_ < 0;
    if ( isy2_ )
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
	if ( rid % eachrow_ ) continue;

	const float xval = uidps_.getVal( x_.colid_, rid );
	const float yval = uidps_.getVal( ad.colid_, rid );
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
    const int nrrows = dps_.size();
    const uiAxisHandler& xah = *x_.axis_;
    const uiAxisHandler& yah = *yad.axis_;
    drawTool().setPenColor( yah.setup().style_.color_ );
    MarkerStyle2D mstyle( setup_.markerstyle_ );
    mstyle.color_ = yah.setup().style_.color_;

    for ( uiDataPointSet::DRowID rid=0; rid<nrrows; rid++ )
    {
	if ( rid % eachrow_ || dps_.isInactive(rid) ) continue;

	const float xval = getVal( x_.colid_, rid );
	const float yval = getVal( yad.colid_, rid );
	if ( mIsUdf(xval) || mIsUdf(yval) ) continue;

	const uiPoint pt( xah.getPix(xval), yah.getPix(yval) );
	const bool issel = dps_.isSelected( rid );
	if ( mstyle.type_ == MarkerStyle2D::None )
	    drawTool().drawPoint( pt, issel );
	else
	{
	    mstyle.size_ = issel ? 4 : 2;
	    drawTool().drawMarker( pt, mstyle );
	}
    }
}


uiDataPointSetCrossPlotWin::uiDataPointSetCrossPlotWin( uiDataPointSet& uidps )
    : uiMainWin(&uidps,BufferString(uidps.pointSet().name()," Cross-plot"),
	    			    2,false)
    , uidps_(uidps)
    , plotter_(*new uiDataPointSetCrossPlotter(this,uidps,defsetup_))
    , disptb_(*new uiToolBar(this,"Crossplot display toolbar"))
    , maniptb_(*new uiToolBar(this,"Crossplot manipulation toolbar"))
    , rbissel_(true)
{
    windowClosed.notify( mCB(this,uiDataPointSetCrossPlotWin,closeNotif) );

    showy2tbid_ = disptb_.addButton( "showy2.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,showY2),
		  "Toggle show Y2", true );
    showbdtbid_ = disptb_.addButton( "showbackdrop.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,showBD),
		      "Toggle data density backdrop", true );

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

    disptb_.addButton( "zoombackward.png",
			mCB(this,uiDataPointSetCrossPlotWin,unZoom),
			"Unzoom", false );

    disptb_.turnOn( showy2tbid_, plotter_.doy2_ );
    disptb_.turnOn( showbdtbid_, plotter_.dobd_ );

    selzoomtbid_ = maniptb_.addButton( "selzoom.png",
	    	  mCB(this,uiDataPointSetCrossPlotWin,selZoom),
		      "Toggle zoom/select", true );
    maniptb_.turnOn( selzoomtbid_, rbissel_ );
    maniptb_.addButton( "delsel.png",
			mCB(this,uiDataPointSetCrossPlotWin,delSel),
			"Delete selected", false );

    setPrefWidth( 600 );
    setPrefWidth( 500 );
}


void uiDataPointSetCrossPlotWin::closeNotif( CallBacker* )
{
    defsetup_ = plotter_.setup();
}


void uiDataPointSetCrossPlotWin::showY2( CallBacker* )
{
    plotter_.doy2_ = disptb_.isOn( showy2tbid_ );
    plotter_.update();
}


void uiDataPointSetCrossPlotWin::showBD( CallBacker* )
{
    plotter_.dobd_ = disptb_.isOn( showbdtbid_ );
    plotter_.update();
    uiMSG().message( "Under construction: data density backdrop" );
}


void uiDataPointSetCrossPlotWin::eachChg( CallBacker* )
{
    int neweachrow = eachfld_->getValue();
    if ( neweachrow < 1 ) neweachrow = 1;
    plotter_.eachrow_ = neweachrow;
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
