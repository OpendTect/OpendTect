/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uifunctiondisplay.h"
#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicssaveimagedlg.h"

#include "hiddenparam.h"
#include "mouseevent.h"
#include "axislayout.h"

static HiddenParam<uiFunctionDisplay,OD::Orientation>
					hp_orientation( OD::Horizontal );
static HiddenParam<uiFunctionDisplay,char> hp_allowaddpts(0);

uiFunctionDisplay::uiFunctionDisplay( uiParent* p,
				      const uiFunctionDisplay::Setup& su )
    : uiFuncDispBase(su)
    , uiGraphicsView(p,"Function display viewer")
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
    , pointChanged(this)
    , pointSelected(this)
    , mouseMove(this)
{
    init( OD::Horizontal );
}


uiFunctionDisplay::uiFunctionDisplay( uiParent* p,
				      const uiFunctionDisplay::Setup& su,
				      OD::Orientation orient )
    : uiFuncDispBase(su)
    , uiGraphicsView(p,"Function display viewer")
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
    , pointChanged(this)
    , pointSelected(this)
    , mouseMove(this)
{
    init( orient );
}


uiFunctionDisplay::~uiFunctionDisplay()
{
    detachAllNotifiers();
    cleanUp();
    delete xax_; delete yax_; delete y2ax_;
    hp_allowaddpts.removeParam(this);
    hp_orientation.removeParam(this);
}


void uiFunctionDisplay::init( OD::Orientation orient )
{
    hp_allowaddpts.setParam( this, 1 );
    hp_orientation.setParam( this, orient );
    disableScrollZoom();
    const bool isvert = isVertical();
    const int width = isvert ? setup_.canvasheight_ : setup_.canvaswidth_ ;
    const int height = isvert ? setup_.canvaswidth_ : setup_.canvasheight_;
    setPrefWidth( width );
    setPrefHeight( height );
    setStretch( 2, 2 );
    uiRect::Side xside = isvert ? uiRect::Left : uiRect::Bottom;
    uiRect::Side yside = isvert ? uiRect::Top : uiRect::Left;

    uiAxisHandler::Setup asu( xside, width, height );
    asu.noaxisline( setup_.noxaxis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annotx_ );
    asu.nogridline( asu.noaxisline_ ? true : setup_.noxgridline_ );
    asu.border_ = setup_.border_;
    asu.annotinint_ = setup_.xannotinint_;
    xax_ = new uiAxisHandler( &scene(), asu );

    // now set up Y axis on the correct side
    asu.noaxisline( setup_.noyaxis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annoty_ );
    asu.nogridline( asu.noaxisline_ ? true : setup_.noygridline_ );
    asu.side( yside );
    asu.annotinint_ = setup_.yannotinint_;
    xax_->setBounds( isvert ? setup_.yrg_ : setup_.xrg_ );
    yax_ = new uiAxisHandler( &scene(), asu );
    yax_->setBounds( isvert ? setup_.xrg_ : setup_.yrg_ );

    asu.noaxisline( setup_.noy2axis_ );
    asu.noaxisannot( asu.noaxisline_ ? true : !setup_.annoty2_ );
    asu.nogridline( setup_.noy2gridline_ );

    xAxis()->setBegin( yAxis(false) );
    yAxis(false)->setBegin( xAxis() );

    // Select side based on orientation
    if ( isvert )
	asu.side( uiRect::Top );
    else
	asu.side( uiRect::Right );

    y2ax_ = new uiAxisHandler( &scene(), asu );

    mAttachCB(getMouseEventHandler().movement,uiFunctionDisplay::mouseMoveCB);
    if ( setup_.editable_ )
    {
	mAttachCB(getMouseEventHandler().buttonPressed,
		  uiFunctionDisplay::mousePressCB);
	mAttachCB(getMouseEventHandler().buttonReleased,
		  uiFunctionDisplay::mouseReleaseCB);
	mAttachCB(getMouseEventHandler().doubleClick,
		  uiFunctionDisplay::mouseDClick);
    }

    setToolTip( tr("Press Ctrl-P to save as image") );
    mAttachCB(reSize,uiFunctionDisplay::reSized);
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
	if ( isVertical() )
	{
	    titleitem_->setAlignment( Alignment(Alignment::Left,
					       Alignment::Top));
	}
	else
	{
	    titleitem_->setAlignment( Alignment(Alignment::HCenter,
						Alignment::Top));
	    titleitem_->setPos( uiPoint(viewWidth()/2,0) );
	}

	titleitem_->setZValue( 2 );
    }

    titleitem_->setText( title );
    draw();//TODO: Figure out how to align without doing this call
}


void uiFunctionDisplay::setTitleColor( const OD::Color& col )
{
    if ( !titleitem_ )
    {
	titleitem_ = scene().addItem( new uiTextItem() );
	if ( isVertical() )
	{
	    titleitem_->setAlignment( Alignment(Alignment::Left,
					       Alignment::Top));
	}
	else
	{
	    titleitem_->setAlignment( Alignment(Alignment::HCenter,
					       Alignment::Top));
	}
	titleitem_->setZValue( 2 );
    }

    titleitem_->setTextColor( col );
    draw();
}


void uiFunctionDisplay::setTitleAlignment( const Alignment& al )
{
    if ( titleitem_ )
    {
	titleitem_->setAlignment(al);
    }
    else
    {
	pErrMsg("Title Item is null, cannot set the title alignment");
    }
    draw();
}

Geom::Point2D<float> uiFunctionDisplay::getFuncXY( int xpix, bool y2 ) const
{
    const uiAxisHandler* axis = xAxis();
    if ( !axis )
	return Geom::Point2D<float>::udf();

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
    if ( !isVertical() )
    {
	return Geom::Point2D<float>(
		    xaxis ? xaxis->getVal( pix.x_ ) : mUdf(float),
		    yaxis ? yaxis->getVal( pix.y_ ) : mUdf(float) );
    }
    else
    {
	return Geom::Point2D<float>(
	    yaxis ? yaxis->getVal( pix.y_ ) : mUdf(float),
	    xaxis ? xaxis->getVal( pix.x_ ) : mUdf(float) );
    }
}


void uiFunctionDisplay::gatherInfo( bool fory2 )
{
    uiFuncDispBase::gatherInfo( fory2 );
    const bool usey2 = fory2 && !setup_.useyscalefory2_;
    if ( usey2 )
    {
	xAxis()->setEnd( yAxis(true) );
	xAxis()->setBegin( yAxis(true) );
	yAxis( true )->setEnd( yAxis(false) );
    }
}


void uiFunctionDisplay::setUpAxis( bool havey2 )
{
    uiAxisHandler* xaxis = xAxis();
    uiAxisHandler* yaxis = yAxis( false );
    uiAxisHandler* y2axis = yAxis( true );
    if ( !xaxis || !yaxis )
	return;

    if ( isVertical() )
	y2axis->setup().side( uiRect::Left );
    else
	y2axis->setup().side( uiRect::Right );

    xaxis->updateDevSize();
    yaxis->updateDevSize();

    xaxis->updateScene();
    yaxis->updateScene();
    if ( y2axis )
    {
	if ( !havey2 )
	    y2axis->setup().noannot( true );
	else
	{
	    const bool noy2axis = setup_.noy2axis_;
	    y2axis->setup().noaxisline(noy2axis).noaxisannot(noy2axis)
			  .nogridline(setup_.noy2gridline_);
	}

	y2axis->updateDevSize();
	y2axis->updateScene();
    }
}


void uiFunctionDisplay::getPointSet( TypeSet<uiPoint>& ptlist, bool y2 )
{
    const uiAxisHandler* xaxis = xAxis();
    const uiAxisHandler* yaxis = yAxis( y2 && !setup_.useyscalefory2_ );
    if ( !xaxis ||!yaxis )
	return;

    const StepInterval<float>& yrg = yaxis->range();
    const int nrpts = y2 ? y2xvals_.size() : xvals_.size();
    const uiPoint closept( xaxis->getPix(xaxis->range().start_),
			   yaxis->getPix(yrg.start_) );
    const bool fillbelow = y2 ? setup_.fillbelowy2_ : setup_.fillbelow_;
    if ( fillbelow )
	ptlist += closept;

    Interval<int> xpixintv( xaxis->getPix(xaxis->range().start_),
				  xaxis->getPix(xaxis->range().stop_) );
    Interval<int> ypixintv( yaxis->getPix(yrg.start_),
				  yaxis->getPix(yrg.stop_) );
    uiPoint pt = closept;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	float xval = y2 ? y2xvals_[idx] : xvals_[idx];
	float yval = y2 ? y2yvals_[idx] : yvals_[idx];

	if ( isVertical() )
	{
	    xval = y2 ? y2xvals_[nrpts-idx-1]
		      : xvals_[idx];
	    yval = y2 ? y2yvals_[nrpts-idx-1]
		      : yvals_[idx];
	}

	if ( mIsUdf(xval) || mIsUdf(yval) )
	{
	    ptlist += uiPoint( mUdf(int), mUdf(int) ); //break in curve
	    continue;
	}

	int xpix, ypix;
	if ( isVertical() )
	{
	    // swap coordinate mapping for vertical layout
	    xpix = yaxis->getPix( yval );
	    ypix = xaxis->getPix( xval );

	    if ( y2 )
	    {
		xpix = yaxis->getPix( abs(yaxis->range().start_
					 - yaxis->range().stop_) - yval );
		ypix = xaxis->getPix( abs(xaxis->range().start_
					 - xaxis->range().stop_) - xval );
	    }
	}
	else
	{
	    xpix = xaxis->getPix( xval );
	    ypix = yaxis->getPix( yval );
	}

	if ( xpixintv.includes(xpix,true) && ypixintv.includes(ypix,true) )
	{
            pt.x_ = xpix;
            pt.y_ = ypix;
	    ptlist += pt;
	}
    }

    if ( setup_.closepolygon_ && fillbelow )
        ptlist += uiPoint( pt.x_, closept.y_ );
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
	ypolygonitem_->setFillColor( setup_.ylinestyle_.color_ );
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
	ypolyitem_->setPenStyle( setup_.ylinestyle_ );
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
		setup_.fillbelowy2_ ? setup_.y2linestyle_.color_
				    : OD::Color::NoColor() );
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
	y2polyitem_->setPenStyle( setup_.y2linestyle_ );
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
        itm->setPos( mCast(float,ptlist[idx].x_), mCast(float,ptlist[idx].y_) );
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
    {
	if ( isVertical() )
	{
	    titleitem_->setPos( uiPoint(viewWidth()/1.6,0) );
	}
	else
	    titleitem_->setPos( uiPoint(viewWidth()/2,0) );
    }

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
    uiAxisHandler* xax = xAxis();
    uiAxisHandler* yax = yAxis( false );

#define mDrawMarkLine(xy,nr,colnr) \
    if ( !mIsUdf(xy##markline##nr##val_) ) \
	drawMarkLine( xy##ax, xy##markline##nr##val_, \
		     OD::Color::stdDrawColor(colnr), xy##markline##nr##item_)
    mDrawMarkLine(x,,0);
    mDrawMarkLine(y,,0);
    mDrawMarkLine(x,2,1);
    mDrawMarkLine(y,2,1);
}


void uiFunctionDisplay::drawMarkLine( uiAxisHandler* ah, float val,
					    OD:: Color col, uiLineItem*& itm )
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
    const Geom::Point2D<int> pos = orientedPix(ev);
    const uiAxisHandler* xax = xAxis();
    const uiAxisHandler* yax = yAxis( false );

    int newsel = -1;
    float mindistsq = 1e30;
    const float xpix = xax->getRelPos( xax->getVal(pos.x_) );
    const float ypix = yax->getRelPos( yax->getVal(pos.y_) );

    for ( int idx=0; idx<xvals_.size(); idx++ )
    {
	const float x = xax->getRelPos( xvals_[idx] );
	const float y = yax->getRelPos( yvals_[idx] );
	const float distsq = (x-xpix)*(x-xpix) + (y-ypix)*(y-ypix);
	if ( distsq < mindistsq )
	{
	    newsel = idx;
	    mindistsq = distsq;
	}
    }
    selpt_ = -1;
    if ( mindistsq > setup_.ptsnaptol_*setup_.ptsnaptol_ )
	return false;

    if ( newsel < 0 || newsel > xvals_.size() - 1 )
    {
	selpt_ = -1;
	return false;
    }

    selpt_ = newsel;
    return true;
}


Geom::Point2D<int> uiFunctionDisplay::orientedPix( const MouseEvent& ev ) const
{
    if ( isVertical() )
	return Geom::Point2D<int>( ev.pos().y_, ev.pos().x_ ); // swap X/Y

    return Geom::Point2D<int>( ev.pos().x_, ev.pos().y_ );
}


void uiFunctionDisplay::mousePressCB( CallBacker* )
{
    if ( mousedown_ ) return;

    mousedown_ = true;
    mGetMousePos();
    if ( isoth || !setSelPt() ) return;

    if ( isnorm )
	pointSelected.trigger();
}


void uiFunctionDisplay::mouseReleaseCB( CallBacker* )
{
    if ( !mousedown_ ) return;

    mousedown_ = false;
    mGetMousePos();

    if ( isnorm && selpt_<0 )
    {
	const Geom::Point2D<int> pos = orientedPix(ev);
	addPoint( Geom::PointF(pos.x_, pos.y_) );
	pointSelected.trigger();
	draw();
	return;
    }

    if ( !isctrl || selpt_<=0 || selpt_>=xvals_.size()-1 || xvals_.size()<3 )
	return;

    if ( hp_allowaddpts.getParam(this)==1 )
    {
	xvals_.removeSingle( selpt_ );
	yvals_.removeSingle( selpt_ );

	selpt_ = -1;
    }

    pointChanged.trigger();
    draw();
}


void uiFunctionDisplay::mouseMoveCB( CallBacker* )
{
    const MouseEvent& mev = getMouseEventHandler().event();
    const Geom::Point2D<int> pos2 = orientedPix(mev);
    mouseMove.trigger( Geom::PointF(pos2.x_, pos2.y_) );
    if ( !setup_.editable_ )
	return;

    if ( !mousedown_ )
	return;

    const uiAxisHandler* xax = xAxis();
    const uiAxisHandler* yax = yAxis( false );
    mGetMousePos();
    if ( !isnorm || selpt_<0 )
	return;

    const Geom::Point2D<int> pos = orientedPix(ev);
    float xval = xax->getVal( pos.x_ );
    float yval = yax->getVal( pos.y_ );

    if ( selpt_>0 && xvals_[selpt_-1]>=xval )
        xval = xvals_[selpt_-1];
    else if ( selpt_<xvals_.size()-1 && xvals_[selpt_+1]<=xval )
        xval = xvals_[selpt_+1];

    if ( xval > xax->range().stop_ )
	xval = xax->range().stop_;
    else if ( xval < xax->range().start_ )
	xval = xax->range().start_;

    if ( yval > yax->range().stop_ )
	yval = yax->range().stop_;
    else if ( yval < yax->range().start_ )
	yval = yax->range().start_;

    xvals_[selpt_] = xval; yvals_[selpt_] = yval;

    pointChanged.trigger();
    draw();
}


void uiFunctionDisplay::mouseDClick( CallBacker* )
{
}


Geom::PointF uiFunctionDisplay::mapToPosition( const Geom::PointF& pt, bool y2 )
{
    uiAxisHandler* xax = xAxis();
    uiAxisHandler* yax = yAxis( y2 );

    if ( !xax || !yax )
	return Geom::PointF::udf();

    return Geom::PointF( xax->getPix(pt.x_), yax->getPix(pt.y_) );
}


Geom::PointF uiFunctionDisplay::mapToValue( const Geom::PointF& pt, bool y2 )
{
    uiAxisHandler* xax = xAxis();
    uiAxisHandler* yax = yAxis( y2 );

    if ( !xax || !yax )
	return Geom::PointF::udf();

    return Geom::PointF( xax->getVal(pt.x_), yax->getVal(pt.y_) );
}


uiAxisHandler* uiFunctionDisplay::xAxis() const
{
    mDynamicCastGet(uiAxisHandler*, xax, xax_);
    return xax;
}


uiAxisHandler* uiFunctionDisplay::yAxis( bool y2 ) const
{
    if ( y2 )
    {
	mDynamicCastGet(uiAxisHandler*, yax, y2ax_);
	return yax;
    }
    else
    {
	mDynamicCastGet(uiAxisHandler*, yax, yax_);
	return yax;
    }
}


void uiFunctionDisplay::allowAddingPoints( bool yn )
{
    hp_allowaddpts.setParam( this, yn ? 1 : 0 );
}


bool uiFunctionDisplay::isVertical() const
{
    return hp_orientation.getParam( this ) == OD::Vertical;
}
