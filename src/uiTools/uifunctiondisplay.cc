/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uifunctiondisplay.h"
#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicssaveimagedlg.h"
#include "mouseevent.h"
#include "axislayout.h"
#include "od_iostream.h"

uiFunctionDisplay::uiFunctionDisplay( uiParent* p,
				      const uiFunctionDisplay::Setup& su )
    : uiGraphicsView(p,"Function display viewer")
    , setup_(su)
    , xax_(0)
    , yax_(0)
    , y2ax_(0)
    , xmarklineval_(mUdf(float))
    , ymarklineval_(mUdf(float))
    , xmarkline2val_(mUdf(float))
    , ymarkline2val_(mUdf(float))
    , selpt_(0)
    , ypolyitem_(0)
    , y2polyitem_(0)
    , ypolygonitem_(0)
    , y2polygonitem_(0)
    , ypolylineitem_(0)
    , y2polylineitem_(0)
    , ymarkeritems_(0)
    , y2markeritems_(0)
    , xmarklineitem_(0)
    , ymarklineitem_(0)
    , xmarkline2item_(0)
    , ymarkline2item_(0)
    , borderrectitem_(0)
    , titleitem_(0)
    , pointSelected(this)
    , pointChanged(this)
    , mousedown_(false)
{
    disableScrollZoom();
    setPrefWidth( setup_.canvaswidth_ );
    setPrefHeight( setup_.canvasheight_ );
    setStretch( 2, 2 );
    uiAxisHandler::Setup asu( uiRect::Bottom, setup_.canvaswidth_,
			      setup_.canvasheight_ );
    asu.noaxisline( setup_.noxaxis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annotx_ );
    asu.nogridline( asu.noaxisline_ ? true : setup_.noxgridline_ );
    asu.border_ = setup_.border_;
    asu.annotinint_ = setup_.xannotinint_;
    xax_ = new uiAxisHandler( &scene(), asu );
    asu.noaxisline( setup_.noyaxis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annoty_ );
    asu.nogridline( asu.noaxisline_ ? true : setup_.noygridline_ );
    asu.side( uiRect::Left );
    asu.annotinint_ = setup_.yannotinint_;
    xax_->setBounds(setup_.xrg_);
    yax_ = new uiAxisHandler( &scene(), asu );
    asu.noaxisline( setup_.noy2axis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annoty2_ );
    asu.nogridline( setup_.noy2gridline_ );
    xax_->setBegin( yax_ ); yax_->setBegin( xax_ );
    asu.side( uiRect::Right );
    y2ax_ = new uiAxisHandler( &scene(), asu );

    if ( setup_.editable_ )
    {
	getMouseEventHandler().buttonPressed.notify(
				mCB(this,uiFunctionDisplay,mousePress) );
	getMouseEventHandler().buttonReleased.notify(
				mCB(this,uiFunctionDisplay,mouseRelease) );
	getMouseEventHandler().movement.notify(
				mCB(this,uiFunctionDisplay,mouseMove) );
	getMouseEventHandler().doubleClick.notify(
				mCB(this,uiFunctionDisplay,mouseDClick) );
    }

    setToolTip( tr("Press Ctrl-P to save as image") );
    reSize.notify( mCB(this,uiFunctionDisplay,reSized) );
}


uiFunctionDisplay::~uiFunctionDisplay()
{
    cleanUp();
    delete xax_; delete yax_; delete y2ax_;
}


void uiFunctionDisplay::cleanUp()
{
    delete ypolylineitem_; delete y2polylineitem_;
    delete ypolygonitem_; delete y2polygonitem_;
    delete ymarkeritems_; delete y2markeritems_;
    delete xmarklineitem_; delete ymarklineitem_;
    delete xmarkline2item_; delete ymarkline2item_;
    ypolylineitem_ = y2polylineitem_ = 0;
    ypolygonitem_ = y2polygonitem_ = 0;
    ymarkeritems_ = y2markeritems_ = 0;
    xmarklineitem_ = ymarklineitem_ = xmarkline2item_ = ymarkline2item_ = 0;
    ypolyitem_ = y2polyitem_ = 0;
    delete titleitem_; titleitem_ = 0;
}


void uiFunctionDisplay::reSized( CallBacker* )
{
    draw();
}

void uiFunctionDisplay::saveImageAs( CallBacker* )
{
}


void uiFunctionDisplay::setTitle( const uiString& title )
{
    if ( !titleitem_ )
    {
	titleitem_ = scene().addItem( new uiTextItem() );
	titleitem_->setAlignment( Alignment(Alignment::HCenter,Alignment::Top));
	titleitem_->setPos( uiPoint(viewWidth()/2,0) );
	titleitem_->setZValue( 2 );
    }

    titleitem_->setText( title );
}


void uiFunctionDisplay::setVals( const float* xvals, const float* yvals,
				 int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz > 0 )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    xvals_ += xvals[idx];
	    yvals_ += yvals[idx];
	}
    }

    gatherInfo( false ); draw();
}


void uiFunctionDisplay::setVals( const Interval<float>& xrg, const float* yvals,
				 int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz > 0 )
    {
	const float dx = (xrg.stop-xrg.start) / (sz-1);
	for ( int idx=0; idx<sz; idx++ )
	{
	    xvals_ += xrg.start + idx * dx;
	    yvals_ += yvals[idx];
	}
    }

    gatherInfo( false ); draw();
}


void uiFunctionDisplay::setY2Vals( const float* xvals, const float* yvals,
				   int sz )
{
    y2xvals_.erase(); y2yvals_.erase();
    if ( sz > 0 )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    y2xvals_ += xvals[idx];
	    y2yvals_ += yvals[idx];
	}
    }

    gatherInfo( true ); draw();
}


void uiFunctionDisplay::setY2Vals( const Interval<float>& xrg,
				   const float* yvals,
				   int sz )
{
    y2xvals_.erase(); y2yvals_.erase();
    if ( sz > 0 )
    {
	const float dx = (xrg.stop-xrg.start) / (sz-1);
	for ( int idx=0; idx<sz; idx++ )
	{
	    y2xvals_ += xrg.start + idx * dx;
	    y2yvals_ += yvals[idx];
	}
    }

    gatherInfo( true ); draw();
}


void uiFunctionDisplay::setMarkValue( float val, bool is_x )
{
    (is_x ? xmarklineval_ : ymarklineval_) = val;
    drawMarkLines();
}


void uiFunctionDisplay::setMark2Value( float val, bool is_x )
{
    (is_x ? xmarkline2val_ : ymarkline2val_) = val;
    drawMarkLines();
}


void uiFunctionDisplay::setEmpty()
{
    xmarklineval_ = ymarklineval_ =
    xmarkline2val_ = ymarkline2val_ = mUdf(float);
    setVals( 0, 0, 0 );
    setY2Vals( 0, 0, 0 );
    cleanUp();
}


Geom::Point2D<float> uiFunctionDisplay::getFuncXY( int xpix, bool y2 ) const
{
    const uiAxisHandler* axis = xAxis();
    if ( !axis ) return Geom::Point2D<float>::udf();

    const float xval = axis->getVal( xpix );
    const TypeSet<float>& xvals = y2 ? y2xvals_ : xvals_;
    const TypeSet<float>& yvals = y2 ? y2yvals_ : yvals_;
    float mindist = mUdf(float);
    int xidx = -1;
    // Not most optimal search
    for ( int idx=0; idx<xvals.size(); idx++ )
    {
	if ( mIsUdf(xvals[idx]) || mIsUdf(yvals[idx]) )
	    continue;

	const float dist = Math::Abs( xval-xvals[idx] );
	if ( dist<mindist )
	{
	    mindist = dist;
	    xidx = idx;
	}
    }

    return xidx==-1 ? Geom::Point2D<float>::udf()
		    : Geom::Point2D<float>( xvals[xidx], yvals[xidx] );
}


Geom::Point2D<float> uiFunctionDisplay::getXYFromPix(
				const Geom::Point2D<int>& pix, bool y2 ) const
{
    const uiAxisHandler* xaxis = xAxis();
    const uiAxisHandler* yaxis = yAxis( y2 );
    return Geom::Point2D<float>(
		xaxis ? xaxis->getVal( pix.x ) : mUdf(float),
		yaxis ? yaxis->getVal( pix.y ) : mUdf(float) );
}


void uiFunctionDisplay::gatherInfo( bool fory2 )
{
    const bool usey2 = fory2 && !setup_.useyscalefory2_;
    if ( !xax_ || ( !usey2 && !yax_ ) || ( usey2 && !y2ax_ ) )
	return;

    Interval<float> xrg, yrg;
    if ( xvals_.isEmpty() )
    {
	xrg.start = mUdf(float); xrg.stop = -mUdf(float);
	yrg = xrg;
    }
    else
    {
	getAxisRanges( xvals_, setup_.xrg_, xrg );
	getAxisRanges( usey2 ? y2yvals_ : yvals_,
		       usey2 ? setup_.y2rg_ : setup_.yrg_, yrg );
    }

    uiAxisHandler* yaxis = usey2 ? y2ax_ : yax_;
    xax_->setBounds( xrg );
    yaxis->setBounds( yrg );

    if ( xax_ && y2ax_ )
    {
	xax_->setEnd( y2ax_ );
	y2ax_->setBegin( xax_ );
    }
}


void uiFunctionDisplay::getAxisRanges( const TypeSet<float>& vals,
				       const Interval<float>& setuprg,
				       Interval<float>& rg ) const
{
    rg.set( mUdf(float), -mUdf(float) );
    for ( int idx=0; idx<vals.size(); idx++ )
    {
	if ( mIsUdf(vals[idx]) )
	    continue;

	rg.include( vals[idx], false );
    }

    if ( !setup_.fixdrawrg_ )
	return;

    if ( !mIsUdf(setuprg.start) ) rg.start = setuprg.start;
    if ( !mIsUdf(setuprg.stop) ) rg.stop = setuprg.stop;
}


void uiFunctionDisplay::setUpAxis( bool havey2 )
{
    xax_->updateDevSize();
    yax_->updateDevSize();

    xax_->updateScene();
    yax_->updateScene();
    if ( y2ax_ )
    {
	if ( !havey2 )
	    y2ax_->setup().noannot( true );
	else
	{
	    const bool noy2axis = setup_.noy2axis_;
	    y2ax_->setup().noaxisline(noy2axis).noaxisannot(noy2axis)
			  .nogridline(setup_.noy2gridline_);
	}

	y2ax_->updateDevSize(); y2ax_->updateScene();
    }
}


void uiFunctionDisplay::getPointSet( TypeSet<uiPoint>& ptlist, bool y2 )
{
    const uiAxisHandler* yax = y2 ? y2ax_ : yax_;
    if ( !yax ) return;

    const StepInterval<float>& yrg = yax->range();
    const int nrpts = y2 ? y2xvals_.size() : xvals_.size();
    const uiPoint closept( xax_->getPix(xax_->range().start),
			   yax->getPix(yrg.start) );
    const bool fillbelow = y2 ? setup_.fillbelowy2_ : setup_.fillbelow_;
    if ( fillbelow )
	ptlist += closept;

    const Interval<int> xpixintv( xax_->getPix(xax_->range().start),
				  xax_->getPix(xax_->range().stop) );
    const Interval<int> ypixintv( yax->getPix(yrg.start),
				  yax->getPix(yrg.stop) );
    uiPoint pt = closept;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	const float xval = y2 ? y2xvals_[idx] : xvals_[idx];
	const float yval = y2 ? y2yvals_[idx] : yvals_[idx];
	if ( mIsUdf(xval) || mIsUdf(yval) )
	{
	    ptlist += uiPoint( mUdf(int), mUdf(int) ); //break in curve
	    continue;
	}

	const int xpix = xax_->getPix( xval );
	const int ypix = yax->getPix( yval );
	if ( xpixintv.includes(xpix,true) && ypixintv.includes(ypix,true) )
	{
	    pt.x = xpix;
	    pt.y = ypix;
	    ptlist += pt;
	}
    }

    if ( setup_.closepolygon_ && fillbelow )
	ptlist += uiPoint( pt.x, closept.y );
}


void uiFunctionDisplay::drawYCurve( const TypeSet<uiPoint>& ptlist )
{
    bool polydrawn = false;
    if ( setup_.fillbelow_ )
    {
	if ( !ypolygonitem_ )
	    ypolygonitem_ = scene().addPolygon( ptlist, setup_.fillbelow_ );
	else
	    ypolygonitem_->setPolygon( ptlist );
	ypolygonitem_->setFillColor( setup_.ycol_ );
	ypolyitem_ = ypolygonitem_;
	polydrawn = true;
    }
    else if ( setup_.drawliney_ )
    {
	if ( !ypolylineitem_ )
	    ypolylineitem_ = scene().addPolyLine( ptlist );
	else
	    ypolylineitem_->setPolyLine( ptlist );
	ypolyitem_ = ypolylineitem_;
	polydrawn = true;
    }

    if ( polydrawn )
    {
	OD::LineStyle ls;
	ls.width_ = setup_.ywidth_;
	ls.color_ = setup_.ycol_;
	ypolyitem_->setPenStyle( ls );
	ypolyitem_->setZValue( setup_.curvzvaly_ );
	ypolyitem_->setVisible( true );
    }
    else if ( ypolyitem_ )
	ypolyitem_->setVisible( false );
}


void uiFunctionDisplay::drawY2Curve( const TypeSet<uiPoint>& ptlist,
				     bool havey2 )
{
    bool polydrawn = false;
    if ( setup_.fillbelowy2_ )
    {
	if ( !y2polygonitem_ )
	    y2polygonitem_ = scene().addPolygon( ptlist, setup_.fillbelowy2_ );
	else
	    y2polygonitem_->setPolygon( ptlist );
	y2polygonitem_->setFillColor(
		setup_.fillbelowy2_ ? setup_.y2col_ : Color::NoColor());
	y2polyitem_ = y2polygonitem_;
	polydrawn = true;
    }
    else if ( setup_.drawliney2_ )
    {
	if ( !y2polylineitem_ )
	    y2polylineitem_ = scene().addPolyLine( ptlist );
	else
	    y2polylineitem_->setPolyLine( ptlist );
	y2polyitem_ = y2polylineitem_;
	polydrawn = true;
    }

    if ( polydrawn )
    {
	OD::LineStyle ls;
	ls.width_ = setup_.y2width_;
	ls.color_ = setup_.y2col_;
	y2polyitem_->setPenStyle( ls );
	y2polyitem_->setZValue( setup_.curvzvaly2_ );
	y2polyitem_->setVisible( true );
    }
    else if ( y2polyitem_ && !havey2)
	y2polyitem_->setVisible( false );
}


void uiFunctionDisplay::drawMarker( const TypeSet<uiPoint>& ptlist, bool isy2 )
{
    if ( isy2 ? !y2markeritems_ : !ymarkeritems_ )
    {
	if ( isy2 )
	{
	    if ( !setup_.drawscattery2_ )
	    {
		if ( y2markeritems_ )
		    y2markeritems_->setVisible( false );
		return;
	    }

	    y2markeritems_ = new uiGraphicsItemGroup( true );
	    scene().addItemGrp( y2markeritems_ );
	}
	else
	{
	    if ( !setup_.drawscattery1_ )
	    {
		if ( ymarkeritems_ )
		    ymarkeritems_->setVisible( false );
		return;
	    }

	    ymarkeritems_ = new uiGraphicsItemGroup( true );
	    scene().addItemGrp( ymarkeritems_ );
	}
    }

    uiGraphicsItemGroup* curitmgrp = isy2 ? y2markeritems_ : ymarkeritems_;
    curitmgrp->setVisible( true );
    const MarkerStyle2D& mst = isy2 ? setup_.markerstyley2_
				    : setup_.markerstyley1_;
    const bool markerisfilled = isy2 ? setup_.markerfilly2_
				     : setup_.markerfilly1_;
    for ( int idx=0; idx<ptlist.size(); idx++ )
    {
	if ( idx >= curitmgrp->size() )
	{
	    uiMarkerItem* markeritem = new uiMarkerItem( mst, markerisfilled );
	    curitmgrp->add( markeritem );
	}
	uiGraphicsItem* itm = curitmgrp->getUiItem(idx);
	itm->setPos( mCast(float,ptlist[idx].x), mCast(float,ptlist[idx].y) );
	itm->setPenColor( mst.color_ );
    }

    if ( ptlist.size() < curitmgrp->size() )
    {
	for ( int idx=ptlist.size(); idx<curitmgrp->size(); idx++ )
	    curitmgrp->getUiItem(idx)->setVisible( false );
    }
    curitmgrp->setZValue( isy2 ? setup_.curvzvaly2_ : setup_.curvzvaly_ );
}


void uiFunctionDisplay::drawBorder()
{
    if ( setup_.drawborder_ )
    {
	const int scwidth = (int)scene().width();
	const int scheight = (int)scene().height();
	const uiRect r( xAxis()->pixBefore(), yAxis(false)->pixAfter(),
		scwidth -xAxis()->pixAfter()-xAxis()->pixBefore(),
		scheight -yAxis(false)->pixAfter()-yAxis(false)->pixBefore() );

	if ( !borderrectitem_ )
	    borderrectitem_ = scene().addRect( mCast(float,r.left()),
					       mCast(float,r.top()),
					       mCast(float,r.right()),
					       mCast(float,r.bottom()) );
	else
	    borderrectitem_->setRect( r.left(), r.top(),
				      r.right(), r.bottom() );
	borderrectitem_->setPenStyle( setup_.borderstyle_ );
    }
    if ( borderrectitem_ )
	borderrectitem_->setVisible( setup_.drawborder_ );
}


void uiFunctionDisplay::draw()
{
    if ( titleitem_ )
	titleitem_->setPos( uiPoint(viewWidth()/2,0) );

    const bool havey = !yvals_.isEmpty();
    const bool havey2 = !y2yvals_.isEmpty();
    if ( !havey && !havey2 )
	return;

    setUpAxis( havey2 );
    drawData();
    drawBorder();
    drawMarkLines();
}


void uiFunctionDisplay::drawData()
{
    const bool havey = !yvals_.isEmpty();
    const bool havey2 = !y2yvals_.isEmpty();
    if ( !havey && !havey2 )
	return;

    TypeSet<uiPoint> yptlist, y2ptlist;
    if ( havey )
    {
	getPointSet( yptlist, false );
	drawYCurve( yptlist );
	drawMarker(yptlist,false);
    }
    else if ( ymarkeritems_ )
	ymarkeritems_->setVisible( false );

    if ( havey2 )
    {
	getPointSet( y2ptlist, true );
	drawY2Curve( y2ptlist, havey2 );
	drawMarker(y2ptlist,true);
    }
    else if ( y2markeritems_ )
	y2markeritems_->setVisible( false );
}


void uiFunctionDisplay::drawMarkLines()
{
#define mDrawMarkLine(xy,nr,colnr) \
    if ( !mIsUdf(xy##markline##nr##val_) ) \
	drawMarkLine( xy##ax_, xy##markline##nr##val_, \
		      Color::stdDrawColor(colnr), xy##markline##nr##item_)
    mDrawMarkLine(x,,0);
    mDrawMarkLine(y,,0);
    mDrawMarkLine(x,2,1);
    mDrawMarkLine(y,2,1);
}


void uiFunctionDisplay::drawMarkLine( uiAxisHandler* ah, float val, Color col,
				      uiLineItem*& itm )
{
    delete itm;
    itm = ah->getGridLine( ah->getPix(val) );
    itm->setPenColor( col );
    itm->setZValue( 100 );
    scene().addItem( itm );
}


#define mGetMousePos()  \
    if ( getMouseEventHandler().isHandled() ) \
	return; \
    const MouseEvent& ev = getMouseEventHandler().event(); \
    if ( !(ev.buttonState() & OD::LeftButton ) || \
	  (ev.buttonState() & OD::MidButton ) || \
	  (ev.buttonState() & OD::RightButton ) ) \
        return; \
    const bool isctrl = ev.ctrlStatus(); \
    const bool isoth = ev.shiftStatus() || ev.altStatus(); \
    const bool isnorm mUnusedVar = !isctrl && !isoth;


bool uiFunctionDisplay::setSelPt()
{
    const MouseEvent& ev = getMouseEventHandler().event();

    int newsel = -1; float mindistsq = 1e30;
    const float xpix = xax_->getRelPos( xax_->getVal(ev.pos().x) );
    const float ypix = yax_->getRelPos( yax_->getVal(ev.pos().y) );
    for ( int idx=0; idx<xvals_.size(); idx++ )
    {
	const float x = xax_->getRelPos( xvals_[idx] );
	const float y = yax_->getRelPos( yvals_[idx] );
	const float distsq = (x-xpix)*(x-xpix) + (y-ypix)*(y-ypix);
	if ( distsq < mindistsq )
	    { newsel = idx; mindistsq = distsq; }
    }
    selpt_ = -1;
    if ( mindistsq > setup_.ptsnaptol_*setup_.ptsnaptol_ ) return false;
    if ( newsel < 0 || newsel > xvals_.size() - 1 )
    {
	selpt_ = -1;
	return false;
    }

    selpt_ = newsel;
    return true;
}


void uiFunctionDisplay::mousePress( CallBacker* )
{
    if ( mousedown_ ) return;

    mousedown_ = true;
    mGetMousePos();
    if ( isoth || !setSelPt() ) return;

    if ( isnorm )
	pointSelected.trigger();
}


void uiFunctionDisplay::mouseRelease( CallBacker* )
{
    if ( !mousedown_ ) return;

    mousedown_ = false;
    mGetMousePos();

    if ( isnorm && selpt_<=0 )
    {
	addPoint( ev.pos() );
	return;
    }

    if ( !isctrl || selpt_ <= 0 || selpt_ >= xvals_.size()-1
	 || xvals_.size() < 3 ) return;

    xvals_.removeSingle( selpt_ );
    yvals_.removeSingle( selpt_ );

    selpt_ = -1;
    pointChanged.trigger();
    draw();
}


void uiFunctionDisplay::mouseMove( CallBacker* )
{
    if ( !mousedown_ ) return;

    mGetMousePos();
    if ( !isnorm || selpt_<0 ) return;

    float xval = xax_->getVal( ev.pos().x );
    float yval = yax_->getVal( ev.pos().y );

    if ( selpt_>0 && xvals_[selpt_-1]>=xval )
        xval = xvals_[selpt_-1];
    else if ( selpt_<xvals_.size()-1 && xvals_[selpt_+1]<=xval )
        xval = xvals_[selpt_+1];

    if ( xval > xax_->range().stop )
	xval = xax_->range().stop;
    else if ( xval < xax_->range().start )
	xval = xax_->range().start;

    if ( yval > yax_->range().stop )
	yval = yax_->range().stop;
    else if ( yval < yax_->range().start )
	yval = yax_->range().start;

    xvals_[selpt_] = xval; yvals_[selpt_] = yval;

    pointChanged.trigger();
    draw();
}


void uiFunctionDisplay::mouseDClick( CallBacker* )
{
}


void uiFunctionDisplay::addPoint( const uiPoint& pt )
{
    float xval = xax_->getVal( pt.x );
    float yval = yax_->getVal( pt.y );

    if ( xval > xax_->range().stop )
	xval = xax_->range().stop;
    else if ( xval < xax_->range().start )
	xval = xax_->range().start;

    if ( yval > yax_->range().stop )
	yval = yax_->range().stop;
    else if ( yval < yax_->range().start )
	yval = yax_->range().start;

    if ( !xvals_.isEmpty() && xval > xvals_.last() )
    {
	xvals_ += xval; yvals_ += yval;
	selpt_ = xvals_.size()-1;
    }
    else
    {
	for ( int idx=0; idx<xvals_.size(); idx++ )
	{
	    if ( xval > xvals_[idx] )
		continue;

	    if ( xval == xvals_[idx] )
		yvals_[idx] = yval;
	    else
	    {
		xvals_.insert( idx, xval );
		yvals_.insert( idx, yval );
	    }

	    selpt_ = idx;
	    break;
	}
    }

    pointSelected.trigger();
    draw();
}


void uiFunctionDisplay::dump( od_ostream& strm, bool y2 ) const
{
    const TypeSet<float>& xvals = y2 ? y2xvals_ : xvals_;
    const TypeSet<float>& yvals = y2 ? y2yvals_ : yvals_;

    strm.stdStream() << std::fixed;
    for ( int idx=0; idx<xvals.size(); idx++ )
	strm << xvals[idx] << od_tab << yvals[idx] << od_endl;
}
