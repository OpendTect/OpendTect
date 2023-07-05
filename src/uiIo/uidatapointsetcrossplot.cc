/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidatapointsetcrossplot.h"

#include "uidatapointset.h"
#include "uigraphicscoltab.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uirgbarray.h"
#include "uiworld2ui.h"

#include "arrayndimpl.h"
#include "densitycalc.h"
#include "linear.h"
#include "mathexpression.h"
#include "mousecursor.h"
#include "statrand.h"


static const int sMarkerZValue = 5;
static const int sLineZValue = 10;
static const int sSelectionZValue = 15;


// uiDataPointSetCrossPlotter::Setup
uiDataPointSetCrossPlotter::Setup::Setup()
    : noedit_(false)
    , minborder_(10,20,20,5)
    , markerstyle_(MarkerStyle2D::Square)
    , xstyle_(OD::LineStyle::Solid,1,OD::Color::Black())
    , ystyle_(OD::LineStyle::Solid,1,OD::Color::stdDrawColor(0))
    , y2style_(OD::LineStyle::Dot,1,OD::Color::stdDrawColor(1))
    , showy1cc_(true)
    , showy2cc_(true)
    , showy1regrline_(false)
    , showy2regrline_(false)
    , showy1userdefpolyline_(false)
    , showy2userdefpolyline_(false)
{
}


// uiDataPointSetCrossPlotter::AxisData
uiDataPointSetCrossPlotter::AxisData::AxisData( uiDataPointSetCrossPlotter& cp,
						uiRect::Side s )
    : uiAxisData( s )
    , cp_( cp )
{
}


uiDataPointSetCrossPlotter::AxisData::~AxisData()
{}


void uiDataPointSetCrossPlotter::AxisData::stop()
{
    if ( isreset_ ) return;
    colid_ = cp_.mincolid_ - 1;
    uiAxisData::stop();
}


void uiDataPointSetCrossPlotter::AxisData::setCol( DataPointSet::ColID cid )
{
    if ( axis_ && cid == colid_ )
	return;

    stop();
    colid_ = cid;
    newColID();
}


void uiDataPointSetCrossPlotter::AxisData::newColID()
{
    if ( colid_ < cp_.mincolid_ )
	return;

    renewAxis( toUiString(cp_.uidps_.userName(colid_)), &cp_.scene(),
	       cp_.viewWidth(), cp_.viewHeight(), 0 );
    handleAutoScale( cp_.uidps_.getRunCalc( colid_ ) );
}



// uiDataPointSetCrossPlotter
mStartAllowDeprecatedSection
uiDataPointSetCrossPlotter::uiDataPointSetCrossPlotter( uiParent* p,
			    uiDataPointSet& uidp,
			    const uiDataPointSetCrossPlotter::Setup& su )
    : uiRGBArrayCanvas(p,*new uiRGBArray(false))
    , lineDrawn( this )
    , mouseReleased( this )
    , dataChgd( this )
    , pointsSelected( this )
    , removeRequest( this )
    , selectionChanged( this )
    , drawTypeChanged( this )
    , coltabRgChanged( this )
    , x_(*this,uiRect::Bottom)
    , y_(*this,uiRect::Left)
    , y2_(*this,uiRect::Right)
    , userdefy1lp_(*new LinePars)
    , userdefy2lp_(*new LinePars)
    , userdefy1str_(*new BufferString)
    , userdefy2str_(*new BufferString)
    , y1rmserr_(*new BufferString)
    , y2rmserr_(*new BufferString)
    , uidps_(uidp)
    , dps_(uidp.pointSet())
    , setup_(su)
    , y3ctab_(ColTab::Sequence("Rainbow"))
    , y4ctab_(ColTab::Sequence(""))
    , lsy1_(*new LinStats2D)
    , lsy2_(*new LinStats2D)
    , mincolid_(1-uidp.pointSet()->nrFixedCols())
{
    setup_.markerstyle_.size_ = 3;
    setup_.showy1userdefpolyline(false).showy2userdefpolyline(false);
    setup_.y1userdeflinestyle_.width_ = 3;
    setup_.y1userdeflinestyle_.color_ = OD::Color::stdDrawColor( 4 );
    setup_.y2userdeflinestyle_.width_ = 3;
    setup_.y2userdeflinestyle_.color_ = OD::Color::stdDrawColor( 5 );
    nrbins_.set( 50, 50 );

    enableImageSave();
    enableScrollZoom();

    x_.defaxsu_.style_ = setup_.xstyle_;
    y_.defaxsu_.style_ = setup_.ystyle_;
    y2_.defaxsu_.style_ = setup_.y2style_;
    x_.defaxsu_.gridlinestyle_ = setup_.xstyle_;
    x_.defaxsu_.gridlinestyle_.color_ = OD::Color(220,220,220);
    y_.defaxsu_.gridlinestyle_ = setup_.ystyle_;
    y_.defaxsu_.gridlinestyle_.color_ = OD::Color(220,110,110);
    y_.defaxsu_.gridlinestyle_.type_ = OD::LineStyle::Dot;
    y2_.defaxsu_.gridlinestyle_ = setup_.y2style_;
    y2_.defaxsu_.gridlinestyle_.color_ = OD::Color(0,170,255);
    y2_.defaxsu_.gridlinestyle_.type_ = OD::LineStyle::Dot;
    x_.defaxsu_.border_ = setup_.minborder_;
    y_.defaxsu_.border_ = setup_.minborder_;
    y2_.defaxsu_.border_ = setup_.minborder_;

    y3mapper_.setup_.cliprate_ = Interval<float>(0.0,0.0);
    y4mapper_.setup_.cliprate_ = Interval<float>(0.0,0.0);

    mAttachCB( reSize, uiDataPointSetCrossPlotter::reSizeDrawCB );
    mAttachCB( reDrawNeeded, uiDataPointSetCrossPlotter::reDrawCB );
    mAttachCB( getMouseEventHandler().buttonPressed,
		uiDataPointSetCrossPlotter::mouseClickedCB );
    mAttachCB( getMouseEventHandler().movement,
		uiDataPointSetCrossPlotter::mouseMoveCB );
    mAttachCB( getMouseEventHandler().buttonReleased,
		uiDataPointSetCrossPlotter::mouseReleasedCB );

    setStretch( 2, 2 );
    setSceneBorder( 2 );
    setDragMode( uiGraphicsView::ScrollHandDrag );

    yrowidxs_ = new Array1DImpl<char>( dps_->size() );
    y2rowidxs_ = new Array1DImpl<char>( dps_->size() );
    selgrpset_ += new SelectionGrp( "No 1", OD::Color::DgbColor() );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
}


uiDataPointSetCrossPlotter::~uiDataPointSetCrossPlotter()
{
    detachAllNotifiers();

    scene().removeItem( yptitems_ );
    scene().removeItem( y2ptitems_ );
    scene().removeItem( selrectitems_ );
    scene().removeItem( selpolyitems_ );
    scene().removeItem( y1overlayctitem_ );
    scene().removeItem( y2overlayctitem_ );

    delete &lsy1_;
    delete &lsy2_;
    delete &userdefy1lp_;
    delete &userdefy2lp_;
    delete &userdefy1str_;
    delete &userdefy2str_;
    delete &y1rmserr_;
    delete &y2rmserr_;
    delete yrowidxs_;
    delete y2rowidxs_;
    deepErase( selgrpset_ );
}
mStopAllowDeprecatedSection

#define mHandleAxisAutoScale(axis) \
    axis.handleAutoScale( uidps_.getRunCalc( axis.colid_ ) );

void uiDataPointSetCrossPlotter::setMathObj( Math::Expression* mathobj )
{
    if ( mathobj )
	mathobj_ = mathobj->clone();
    else
	mathobj_ = 0;
}


void uiDataPointSetCrossPlotter::reSizeDrawCB( CallBacker* )
{
    selyitems_ = 0;
    sely2items_ = 0;
    reDrawCB( 0 );
}


void uiDataPointSetCrossPlotter::reDrawCB( CallBacker* )
{
    MouseCursorChanger cursorchanger(MouseCursor::Wait);
    setDraw();
    reDrawSelArea();
    drawContent();
}


void uiDataPointSetCrossPlotter::showY2( bool yn )
{
    if ( y2ptitems_ )
	y2ptitems_->setVisible( yn );

    if ( !yn )
    {
	setOverlayY2Cols( mUdf(int) );
	setShowY4( false );
    }

    drawContent();
}


bool uiDataPointSetCrossPlotter::isY2Shown() const
{
     return y2_.axis_ && y2ptitems_ && y2ptitems_->isVisible();
}


int uiDataPointSetCrossPlotter::totalNrItems() const
{
     const int totalrows = mNINT32( plotperc_/(float)100 * dps_->nrActive() );
    return isY2Shown() ? totalrows*2 :  y_.axis_ ? totalrows : 0;
}


void uiDataPointSetCrossPlotter::dataChanged()
{
    mHandleAxisAutoScale( x_ )
    mHandleAxisAutoScale( y_ )
    mHandleAxisAutoScale( y2_ )
    calcStats();
    setDraw();
    removeSelections(true);
    drawContent();
    dataChgd.trigger();
}


void uiDataPointSetCrossPlotter::getRandRowids()
{
    const int totalrows = mNINT32( plotperc_/(float)100 * dps_->nrActive() );
    if ( mIsEqual(plotperc_,100,mDefEps) )
    {
	yrowidxs_->ArrayND<char>::setAll( '1' );
	y2rowidxs_->ArrayND<char>::setAll( '1' );
	return;
    }

    Stats::RandGen randgen;
    for ( int idx=0; idx<2; idx++ )
    {
	int rowcount =0;

	Array1D<char>* rowidxs = idx==0 ? yrowidxs_ : y2rowidxs_;
	const bool highperc = plotperc_ > 50;
	rowidxs->ArrayND<char>::setAll( highperc ? '1' : '0' );
	const float nrrowneeded =
		sCast(float,highperc ? dps_->nrActive()-totalrows : totalrows);
	while ( rowcount < nrrowneeded )
	{
	    int randrow = randgen.getIndex( dps_->size() );
	    if ( ((!highperc && rowidxs->get(randrow) == '0') ||
		 (highperc && rowidxs->get(randrow) == '1')) &&
		 !dps_->isInactive(randrow) )
		rowidxs->set( randrow, highperc ? '0' : '1' );
	    else
		continue;
	    rowcount ++;
	}
    }
}


void uiDataPointSetCrossPlotter::removeSelectionItems()
{
    if ( selpolyitems_ )
    {
	selpolyitems_->removeAll( true );
	selectionpolygonitem_ = 0;
    }

    if ( selrectitems_ )
	selrectitems_->removeAll( true );
}


void uiDataPointSetCrossPlotter::deleteSelections()
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );

    uidps_.setUnsavedChg( true );
    for ( uiDataPointSet::DRowID rid=0; rid<dps_->size(); rid++ )
    {
	checkSelection( rid, 0, false, y_, true );
	if ( y2_.axis_ )
	    checkSelection( rid, 0, true, y2_, true );
    }

    drawData( y_, false, true );
    if ( y2_.axis_ )
	drawData( y2_ , true, true );

    uidps_.reDoTable();
}


int uiDataPointSetCrossPlotter::getSelGrpIdx( int selareaid ) const
{
    for ( int idx=0; idx<selgrpset_.size(); idx++ )
    {
	const SelectionGrp* selgrp = selgrpset_[idx];
	for ( int selidx=0; selidx < selgrp->size(); selidx++ )
	{
	    if ( selgrp->getSelectionArea(selidx).id_ == selareaid )
		return idx;
	}
    }

    return -1;
}


int uiDataPointSetCrossPlotter::getNewSelAreaID() const
{
    int lastid = 0;
    for ( int idx=0; idx<selgrpset_.size(); idx++ )
    {
	const SelectionGrp* selgrp = selgrpset_[idx];
	for ( int idy=0; idy<selgrp->size(); idy++ )
	{
	    if ( selgrp->getSelectionArea(idy).id_ > lastid )
		lastid = selgrp->getSelectionArea(idy).id_;
	}
    }

    return lastid+1;

}


bool uiDataPointSetCrossPlotter::isSelAreaValid( int selareaid ) const
{
    for ( int idx=0; idx<selgrpset_.size(); idx++ )
    {
	const SelectionGrp* selgrp = selgrpset_[idx];
	for ( int idy=0; idy<selgrp->size(); idy++ )
	{
	    if ( selgrp->getSelectionArea(idy).id_ == selareaid )
		return true;
	}
    }

    return false;
}


TypeSet<OD::Color> uiDataPointSetCrossPlotter::selGrpCols() const
{
    TypeSet<OD::Color> selcols;
    for ( int idx=0; idx<selgrpset_.size(); idx++ )
	selcols += selgrpset_[idx]->col_;
    return selcols;
}


void uiDataPointSetCrossPlotter::reDrawSelections()
{
    removeSelections( false );
    reDrawSelArea();

    if ( isdensityplot_ )
    {
	for ( uiDataPointSet::DRowID rid=0; rid<dps_->size(); rid++ )
	{
	    if ( dps_->isInactive(rid) ) continue;

	    checkSelection( rid, 0, false, y_, false );
	    if ( y2_.axis_ )
		checkSelection( rid, 0, true, y2_, false );
	}
    }
    else
    {
	drawData( y_, false );
	if ( y2_.axis_ )
	    drawData( y2_, true );
	else if ( y2ptitems_ )
	    y2ptitems_->removeAll( true );
    }
}

mStartAllowDeprecatedSection
void uiDataPointSetCrossPlotter::removeSelections( bool remfrmselgrp )
{
    removeSelectionItems();
    selrowcols_.erase();
    dpsselrowcols_.erase();
    selyitems_ = 0;
    sely2items_ = 0;

    if ( remfrmselgrp )
    {
	for ( int idx=0; idx<selgrpset_.size(); idx++ )
	    selgrpset_[idx]->removeAll();
    }

    for ( int idx=0; idx<dps_->size(); idx++ )
	dps_->setSelected( idx, -1 );
}
mStopAllowDeprecatedSection

void uiDataPointSetCrossPlotter::setUserDefDrawType( bool dodrw, bool isy2,
							bool drwln )
{
    bool& drawuserdefpolyline = isy2 ? drawy2userdefpolyline_
				     : drawy1userdefpolyline_;
    drawuserdefpolyline = dodrw;
    drawuserdefline_ = drwln;
    selectable_ = !drwln;
    drawy2_ = isy2;
}


void uiDataPointSetCrossPlotter::setUserDefPolyLine(
	TypeSet<uiWorldPoint>& pts, bool isy2 )
{
    if ( !isy2 )
	y1userdefpts_ = pts;
    else
	y2userdefpts_ = pts;
}


void uiDataPointSetCrossPlotter::drawUserDefPolyLine( bool isy1 )
{
    uiPolyLineItem*& curpolylineitem =
	!isy1 ? y2userdefpolylineitm_ : y1userdefpolylineitm_;

    if ( !curpolylineitem )
    {
	curpolylineitem = new uiPolyLineItem();
	curpolylineitem->setZValue( sLineZValue );
	scene().addItem( curpolylineitem );
    }

    const bool dodrw = isy1 ? drawy1userdefpolyline_ : drawy2userdefpolyline_;
    if ( !dodrw )
    {
	curpolylineitem->setVisible( dodrw );
	return;
    }

    const TypeSet<uiWorldPoint>& pts = !isy1 ? y2userdefpts_ : y1userdefpts_;
    uiAxisHandler* xah = axisHandler( 0 );
    uiAxisHandler* yah = axisHandler( !isy1 ? 2 : 1 );
    if ( !xah || !yah )
	return;

    const int size = pts.size();
    if ( size==0 )
	return;

    TypeSet<uiPoint> pixpts;
    for ( int pixvar=0; pixvar<size; pixvar++ )
    {
	uiWorldPoint pt = pts[pixvar];
	if ( mIsUdf(pt.x) || mIsUdf(pt.y) ) continue;
	if ( mIsUdf(xah->getPix(pt.x)) || mIsUdf(yah->getPix(pt.y)) ) continue;

	if ( !mousepressed_ )
	{
	    if (!xah->pixRange().includes(xah->getPix(pt.x),false) ) continue;
	    if (!yah->pixRange().includes(yah->getPix(pt.y),false) ) continue;
	}

	pixpts += uiPoint( xah->getPix(pt.x), yah->getPix(pt.y) );
    }

    curpolylineitem->setPolyLine( pixpts );
    curpolylineitem->setPenStyle( isy1 ? setup_.y1userdeflinestyle_
					: setup_.y2userdeflinestyle_);
    curpolylineitem->setVisible( true );
}


void uiDataPointSetCrossPlotter::setSelectable( bool y1, bool y2 )
{
    isy1selectable_ = y1;
    isy2selectable_ = y2;
}

# define mSetSelAreaAxisType( selarea ) \
    if ( isy1selectable_ && isy2selectable_ && isY2Shown() ) \
	selarea.axistype_ = SelectionArea::Both; \
    else if ( !isy2selectable_ ) \
	selarea.axistype_ = SelectionArea::Y1; \
    else \
	selarea.axistype_ = SelectionArea::Y2;

void uiDataPointSetCrossPlotter::mouseClickedCB( CallBacker* )
{
    if ( drawuserdefline_ )
    {
	startpos_ = getCursorPos();
	mousepressed_ = true;
    }

    const MouseEvent& ev = getMouseEventHandler().event();
    const bool isnorm = !ev.ctrlStatus() && !ev.shiftStatus() &&!ev.altStatus();

    if ( isnorm && selNearest(ev) )
	selectionChanged.trigger();

    if ( !selectable_ || !selgrpset_.size() )
	return;

    mousepressed_ = true;

    SelectionGrp* selgrp = selgrpset_[curselgrp_];
    const int newselareaid = getNewSelAreaID();

    if ( rectangleselection_ )
    {
	SelectionArea selarea( uiRect(getCursorPos(),uiSize(0,0)) );
	selarea.id_ = newselareaid;
	mSetSelAreaAxisType( selarea )
	selgrp->addSelection( selarea );
	if ( !selectionrectitem_ )
	    selectionrectitem_ = new uiRectItem( 0, 0, 1, 1 );
	else
	    selectionrectitem_->setRect( 0, 0, 1, 1 );

	selectionrectitem_->setPenColor( selgrpset_[curselgrp_]->col_ );
    }
    else
    {
	ODPolygon<int> poly;
	SelectionArea selarea( poly );
	selarea.id_ = newselareaid;
	poly.add( getCursorPos() );

	mSetSelAreaAxisType( selarea )
	selgrp->addSelection( selarea );

	if ( !selectionpolygonitem_ )
	{
	    selectionpolygonitem_ = new uiPolygonItem();
	    if ( !selpolyitems_ )
	    {
		selpolyitems_ = new uiGraphicsItemGroup();
		scene().addItemGrp( selpolyitems_ );
		selpolyitems_->setZValue( sSelectionZValue );
	    }

	    selpolyitems_->add( selectionpolygonitem_ );
	}
	selectionpolygonitem_->setPenColor( selgrpset_[curselgrp_]->col_ );
    }

    curselarea_ = selgrp->size() - 1;
    SelectionArea& selarea = getCurSelArea();
    if ( !axisHandler(0) || !axisHandler(1) )
	return;

    selarea.xaxisnm_ = axisHandler(0)->getCaption().getFullString();
    selarea.yaxisnm_ =
	axisHandler( isy1selectable_ ? 1 : 2 )->getCaption().getFullString();
}


void uiDataPointSetCrossPlotter::mouseMoveCB( CallBacker* )
{
    if ( !mousepressed_ ) return;
    if ( drawuserdefline_ )
    {
	uiPoint stoppos = getCursorPos();
	const uiAxisHandler& xah = *x_.axis_;
	const uiAxisHandler& yah = drawy2_ ? *y2_.axis_ : *y_.axis_;

	LinePars& linepar = drawy2_ ? userdefy2lp_ : userdefy1lp_;
	const float base =
	    xah.getVal(stoppos.x) - xah.getVal(startpos_.x);
	const float perpendicular =
	    yah.getVal(stoppos.y) - yah.getVal(startpos_.y);

	if ( !mIsZero(base,1e-6) )
	{
	    linepar.ax = perpendicular/base;
	    linepar.a0 = yah.getVal(startpos_.y) -
			( linepar.ax * xah.getVal(startpos_.x) );
	}

	BufferString& linestr = drawy2_ ? userdefy2str_ : userdefy1str_;
	BufferString& rmserr = drawy2_ ? y2rmserr_ : y1rmserr_;
	linestr.setEmpty(); rmserr.setEmpty();

	if ( !mIsZero(base,1e-6) )
	{
	    bool makeabs = false;
	    if ( linepar.ax )
	    {
		linestr.add( linepar.ax, 4 ).add( " * X" );
		if ( linepar.a0 > 0 )
		    linestr.add( " + " );
		else
		{
		    linestr.add( " - " );
		    makeabs = true;
		}
	    }

	    linestr.add( makeabs ? Math::Abs(linepar.a0) : linepar.a0, 4 );
	}

	TypeSet<uiWorldPoint> linepts;
	linepts += uiWorldPoint( xah.getVal(startpos_.x),
				 yah.getVal(startpos_.y) );
	linepts += uiWorldPoint( xah.getVal(stoppos.x),
				 yah.getVal(stoppos.y) );
	setUserDefPolyLine( linepts, drawy2_ );
	drawUserDefPolyLine( !drawy2_ );
	lineDrawn.trigger();
	return;
    }

    if ( !selectable_ || rectangleselection_ )
	return;

    SelectionArea& selarea = getCurSelArea();
    selarea.poly_.add( getCursorPos() );
    selarea.geomChanged();

    if ( !selectionpolygonitem_ )
    {
	selectionpolygonitem_ = new uiPolygonItem();
	if ( !selpolyitems_ )
	{
	    selpolyitems_ = new uiGraphicsItemGroup();
	    scene().addItemGrp( selpolyitems_ );
	    selpolyitems_->setZValue( sSelectionZValue );
	}

	selpolyitems_->add( selectionpolygonitem_ );
    }
    selectionpolygonitem_->setPolygon( selarea.poly_ );
}


float uiDataPointSetCrossPlotter::getVal( int colid, int rid ) const
{ return uidps_.getVal( colid, rid, true ); }


void uiDataPointSetCrossPlotter::setCTMapper( const ColTab::MapperSetup& su )
{ ctmapper_.setup_ = su; }


void uiDataPointSetCrossPlotter::setShowY3( bool yn )
{
    showy3_ = yn;
    if ( y1overlayctitem_ )
	y1overlayctitem_->setColTabMapperSetup( y3mapper_.setup_ );
}


void uiDataPointSetCrossPlotter::setShowY4( bool yn )
{
    showy4_ = yn;
    if ( y2overlayctitem_ )
	y2overlayctitem_->setColTabMapperSetup( y4mapper_.setup_ );
}


void uiDataPointSetCrossPlotter::drawColTabItem( bool isy1 )
{
    if ( !axisHandler(0) || !axisHandler(isy1? 1:2) ) return;

    if ( (isy1 && !showy3_) || (!isy1 && !showy4_) )
    {
	if ( y1overlayctitem_ )
	    y1overlayctitem_->setVisible( showy3_ );

	if ( y2overlayctitem_ )
	    y2overlayctitem_->setVisible( showy4_ && isY2Shown() );

	if ( x_.axis_ && !showy3_ )
	    x_.axis_->setup().border_ = setup_.minborder_;

	return;
    }

    uiColTabItem* coltabitem = isy1 ? y1overlayctitem_ : y2overlayctitem_;

    if ( !coltabitem )
    {
	uiColTabItem::Setup ctsu( false );
	coltabitem = new uiColTabItem( ctsu );
	if ( isy1 )
	    y1overlayctitem_ = coltabitem;
	else
	    y2overlayctitem_ = coltabitem;
	scene().addItem( coltabitem );
    }

    uiColTabItem::Setup& ctsu = coltabitem->setup();
    ctsu.sz_.setHeight( viewHeight()/4 );

    uiBorder extraborder = setup_.minborder_;
    extraborder.setRight( extraborder.right() + ctsu.sz_.width()*2 );
    x_.axis_->setup().border_ = extraborder;
    y_.axis_->setup().border_ = extraborder;
    if ( y2_.axis_ )
	y2_.axis_->setup().border_ = extraborder;
    setDraw();

    const int xpos = viewWidth() - extraborder.right() + ctsu.sz_.width()/2;
    const int ypos = isy1
		? viewHeight() - y_.axis_->pixBefore() - ctsu.sz_.height()
		: y_.axis_->pixBefore();
    coltabitem->setPos( sCast(float,xpos), sCast(float,ypos) );
    ColTab::Sequence ctab = isy1 ? y3ctab_ : y4ctab_;
    coltabitem->setColTabSequence( ctab );
    const ColTab::MapperSetup& mappersetup = isy1 ? y3mapper_.setup_
						  : y4mapper_.setup_;
    coltabitem->setColTabMapperSetup( mappersetup );
    coltabitem->setVisible( isy1 ? showy3_ : ( showy4_ && isY2Shown() ) );
    coltabitem->update();
}


void uiDataPointSetCrossPlotter::updateOverlayMapper( bool isy1 )
{
    if ( mIsUdf(isy1 ? y3colid_ : y4colid_) )
	return;

    Array1DImpl<float> ydata( dps_->size() );
    float* yvals = ydata.getData();
    for ( int idx=0; idx<dps_->size(); idx++ )
	yvals[idx] = uidps_.getVal( isy1 ? y3colid_ : y4colid_ , idx, true );

    ColTab::Mapper& mapper = isy1 ? y3mapper_ : y4mapper_;
    mapper.setData( yvals, ydata.totalSize() );
}


void uiDataPointSetCrossPlotter::setOverlayY1AttMapr(
				const ColTab::MapperSetup& y3mpr )
{
    y3mapper_.setup_ = y3mpr;
}


void uiDataPointSetCrossPlotter::setOverlayY2AttMapr(
				const ColTab::MapperSetup& y4mpr )
{
    y4mapper_.setup_ = y4mpr;
}


void uiDataPointSetCrossPlotter::setOverlayY1AttSeq(
					const ColTab::Sequence& y3ct )
{
    y3ctab_ = y3ct;
}


void uiDataPointSetCrossPlotter::setOverlayY2AttSeq(
					const ColTab::Sequence& y4ct)
{
    y4ctab_ = y4ct;
}


int uiDataPointSetCrossPlotter::selAreaSize() const
{
    int size = 0;
    for ( int idx=0; idx<selgrpset_.size(); idx++ )
    {
	const SelectionGrp* selgrp = selgrpset_[idx];
	size += selgrp->size();
    }

    return size;
}


SelectionArea& uiDataPointSetCrossPlotter::getCurSelArea()
{
    return selgrpset_[curselgrp_]->getSelectionArea( curselarea_ );
}


void uiDataPointSetCrossPlotter::setSelArea( const SelectionArea& selarea,
					     int selgrpidx )
{
    if ( selgrpidx < 0 || selgrpidx >= selgrpset_.size() )
	return;

    selgrpset_[selgrpidx]->setSelectionArea( selarea );
}


bool uiDataPointSetCrossPlotter::getSelArea( SelectionArea& selarea,
					     int selareaid )
{
    for ( int idx=0; idx < selgrpset_.size(); idx++ )
    {
	const int valididx = selgrpset_[idx]->validIdx( selareaid );
	if ( valididx >= 0 )
	{
	    selarea = selgrpset_[idx]->getSelectionArea( valididx );
	    return true;
	}
    }

    return false;
}

void uiDataPointSetCrossPlotter::setWorldSelArea( int selareaid  )
{
    if ( !x_.axis_ || !y_.axis_ || !isSelAreaValid(selareaid) )
	return;

    const uiAxisHandler& xah = *x_.axis_;
    SelectionArea selarea;
    if ( !getSelArea(selarea,selareaid) )
	return;

    const uiAxisHandler& yah =
	(selarea.axistype_ == SelectionArea::Y2) ? *y2_.axis_ : *y_.axis_;

    if ( selarea.isrectangle_ )
    {
	uiRect selrect = selarea.rect_;
	uiWorldRect worldselarea( xah.getVal(selrect.left()),
				  yah.getVal(selrect.top()),
				  xah.getVal(selrect.right()),
				  yah.getVal(selrect.bottom()) );
	selarea.worldrect_ = worldselarea;

	if ( selarea.axistype_ == SelectionArea::Both )
	{
	    const uiAxisHandler& y2ah = *y2_.axis_;
	    uiWorldRect altrect( xah.getVal(selrect.left()),
				 y2ah.getVal(selrect.top()),
				 xah.getVal(selrect.right()),
				 y2ah.getVal(selrect.bottom()) );
	    selarea.altworldrect_ = altrect;
	}
    }
    else
    {
	ODPolygon<double> worldpoly;
	ODPolygon<double> altworldpoly;
	ODPolygon<int> selpoly = selarea.poly_;
	TypeSet<uiPoint> polypts = selpoly.data();
	for (  int idx=0; idx<polypts.size(); idx++ )
	{
	    worldpoly.add( uiWorldPoint(xah.getVal(polypts[idx].x),
					yah.getVal(polypts[idx].y)) );
	    if ( selarea.axistype_ == SelectionArea::Both )
	    {
		altworldpoly.add( uiWorldPoint(xah.getVal(polypts[idx].x),
				  y2_.axis_->getVal(polypts[idx].y)) );
	    }
	}

	selarea.worldpoly_= worldpoly;

	if ( selarea.axistype_ == SelectionArea::Both )
	    selarea.altworldpoly_ = altworldpoly;
    }

    setSelArea( selarea, curselgrp_ );
}


void uiDataPointSetCrossPlotter::reDrawSelArea()
{
    if ( !x_.axis_ || !y_.axis_ ) return;

    const uiAxisHandler& xah = *x_.axis_;

    int nrrect = 0;
    int nrpoly = 0;
    for ( int idx=0; idx<selgrpset_.size(); idx++ )
    {
	SelectionGrp* selgrp = selgrpset_[idx];

	for ( int selidx=0; selidx<selgrp->size(); selidx++ )
	{
	    SelectionArea& selarea = selgrp->getSelectionArea( selidx );
	    const uiAxisHandler& yah =
		(selarea.axistype_ == SelectionArea::Y2) ? *y2_.axis_
							 : *y_.axis_;
	    const OD::Color& col = selgrp->col_;
	    if ( selarea.isrectangle_ )
	    {
		const uiWorldRect& worldselarea = selarea.worldrect_;
		uiRect selrect( xah.getPix(worldselarea.left()),
				yah.getPix(worldselarea.top()),
				xah.getPix(worldselarea.right()),
				yah.getPix(worldselarea.bottom()) );
		selarea.rect_ = selrect;
		uiRectItem* selrectitem = 0;

		if ( !selrectitems_ )
		{
		    selrectitems_ = new uiGraphicsItemGroup();
		    scene().addItemGrp( selrectitems_ );
		    selrectitems_->setZValue( sSelectionZValue );
		}

		if ( nrrect >= selrectitems_->size() )
		{
		    selrectitem = new uiRectItem();
		    selrectitems_->add( selrectitem );
		}

		uiGraphicsItem* itm = selrectitems_->getUiItem( nrrect );
		if ( !itm )
		    continue;

		mDynamicCast(uiRectItem*,selrectitem,itm)
		if ( !selrectitem )
		    continue;

		selrectitem->setRect( selrect.left(),selrect.top(),
						 selrect.width(),
						 selrect.height() );

		selrectitem->setPenColor( col );
		nrrect++;
	    }
	    else
	    {
		ODPolygon<int> poly;
		const ODPolygon<double>& worldpoly = selarea.worldpoly_;
		TypeSet<uiWorldPoint> polypts = worldpoly.data();
		for (  int nrpts=0; nrpts<polypts.size(); nrpts++ )
		    poly.add( uiPoint(xah.getPix(polypts[nrpts].x),
				      yah.getPix(polypts[nrpts].y)) );
		selarea.poly_= poly;

		if ( !selpolyitems_ || selpolyitems_->size() <= nrpoly )
		{
		    if ( !selpolyitems_ )
		    {
			selpolyitems_ = new uiGraphicsItemGroup();
			scene().addItemGrp( selpolyitems_ );
			selpolyitems_->setZValue( sSelectionZValue );
		    }

		    selectionpolygonitem_ = new uiPolygonItem();
		    selpolyitems_->add( selectionpolygonitem_ );
		}

		uiGraphicsItem* item = selpolyitems_->getUiItem( nrpoly );
		mDynamicCastGet( uiPolygonItem*, polyitem, item );
		if ( !polyitem )
		    continue;
		polyitem->setPolygon( poly );
		polyitem->setPenColor( col );
		nrpoly ++;
	    }
	}
    }

    selectionrectitem_ = 0;
    selectionpolygonitem_ = 0;
}


void uiDataPointSetCrossPlotter::mouseReleasedCB( CallBacker* )
{
    mousepressed_ = false; mouseReleased.trigger();
    const MouseEvent& ev = getMouseEventHandler().event();
    const bool isdel = ev.shiftStatus() && !ev.ctrlStatus() && !ev.altStatus();
    if ( !setup_.noedit_ && isdel && selNearest(ev) )
    {
	removeRequest.trigger();
	selrow_ = -1;
    }

    if ( !selectable_ || !selgrpset_[curselgrp_]->isValidIdx(curselarea_) )
	return;

    int selareas = 0;
    for ( int idx=0; idx < selgrpset_.size(); idx++ )
	selareas += selgrpset_[idx]->size();
    if ( selareas == 0 )
	return;

    SelectionArea curselarea = getCurSelArea();

    if ( !getMouseEventHandler().event().ctrlStatus() && selareas>1 )
    {
	curselarea.id_ = 0;
	removeSelections();
    }

    if ( rectangleselection_ )
    {
	uiRect selrect = scene().getSelectedArea();
	curselarea.rect_.setTopLeft( selrect.topLeft() );
	curselarea.rect_.setBottomRight( selrect.bottomRight() );
	curselarea.geomChanged();
	if ( !selectionrectitem_ )
	    selectionrectitem_ =
		new uiRectItem( selrect.left(), selrect.top(),
				selrect.width(), selrect.height() );
	else
	    selectionrectitem_->setRect( selrect.left(), selrect.top(),
					 selrect.width(), selrect.height() );

	selectionrectitem_->setPenColor( selgrpset_[curselgrp_]->col_ );
	if ( !selrectitems_ )
	{
	    selrectitems_ = new uiGraphicsItemGroup();
	    scene().addItemGrp( selrectitems_ );
	    selrectitems_->setZValue( sSelectionZValue );
	}

	selrectitems_->add( selectionrectitem_ );
	selectionrectitem_ = 0;
    }
    else
    {
	if ( !selectionpolygonitem_ )
	{
	    selectionpolygonitem_ = new uiPolygonItem();
	    selectionpolygonitem_->setPolygon( curselarea.poly_ );
	    selectionpolygonitem_->setPenColor( selgrpset_[curselgrp_]->col_ );
	    selpolyitems_->add( selectionpolygonitem_ );
	}

	selectionpolygonitem_ = 0;
    }


    if ( curselarea.id_ == 0 )
    {
	mSetSelAreaAxisType( curselarea )
	selgrpset_[curselgrp_]->addSelection( curselarea );
    }
    else
	selgrpset_[curselgrp_]->setSelectionArea( curselarea );

    setWorldSelArea( curselarea.id_ );

    if ( !curselarea.isValid() )
	selgrpset_[curselgrp_]->removeSelection( curselarea_ );

    if ( isdensityplot_ )
    {
	for ( uiDataPointSet::DRowID rid=0; rid<dps_->size(); rid++ )
	{
	    checkSelection( rid, 0, false, y_, false );
	    if ( y2_.axis_ )
		checkSelection( rid, 0, true, y2_, false );
	}
    }
    else
    {
	drawData( y_, false );
	if ( y2_.axis_ )
	    drawData( y2_, true );
	else if ( y2ptitems_ )
	    y2ptitems_->removeAll( true );
    }

    pointsSelected.trigger();
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
	if ( xrg.includes(x,true) && yrg.includes(y,true) )
	{
	    xvals += x;
	    yvals += y;
	}
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
    for ( uiDataPointSet::DRowID rid=0; rid<dps_->size(); rid++ )
    {
	if ( dps_->isInactive(rid) ||
				(curgrp_>0 && dps_->group(rid) != curgrp_) )
	    continue;

	const float xval = uidps_.getVal( x_.colid_, rid, true );
	if ( mIsUdf(xval) )
	    continue;

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
    if ( !x_.axis_ || !ad.axis_ ) return -1;

    const float xpix = x_.axis_->getRelPos( x_.axis_->getVal(pt.x) );
    const float ypix = ad.axis_->getRelPos( ad.axis_->getVal(pt.y) );
    int row = -1; float mindistsq = 1e30;
    for ( uiDataPointSet::DRowID rid=0; rid<dps_->size(); rid++ )
    {
	if ( (curgrp_ > 0 && dps_->group(rid) != curgrp_) )
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


void uiDataPointSetCrossPlotter::setOverlayY1Cols( int y3 )
{ y3colid_ = y3; }


void uiDataPointSetCrossPlotter::setOverlayY2Cols( int y4 )
{ y4colid_ = y4; }


#define mIsHPosCol( colid ) \
    ( colid == mincolid_ || colid == mincolid_+1 )

void uiDataPointSetCrossPlotter::setCols( DataPointSet::ColID x,
		    DataPointSet::ColID y, DataPointSet::ColID y2 )
{
    if ( y < mincolid_ && y2 < mincolid_ )
	x = mincolid_ - 1;

    if ( x < mincolid_ )
	y = y2 = mincolid_ - 1;

    const bool posdisptypechgd = uidps_.posDispTypeChgd();
    const bool isprevx = ( x_.colid_ == x ) &&
			 ( posdisptypechgd ? !mIsHPosCol( x ) : true );
    const bool isprevy = ( y_.colid_ == y ) &&
			 ( posdisptypechgd ? !mIsHPosCol( y ) : true );
    const bool isprevy2 = ( y2_.colid_ == y2 ) &&
			  ( posdisptypechgd ? !mIsHPosCol( y2 ) : true );
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

    if ( !isprevx || !isprevy )
    {
	if ( y1userdefpolylineitm_ && !axisHandler(1) )
	    y1userdefpolylineitm_->setVisible( false );
	userdefy1str_.setEmpty(); y1rmserr_.setEmpty();
	setup().showy1userdefpolyline_ = false;
    }
    drawy1userdefpolyline_ = setup().showy1userdefpolyline_;

    if ( !isprevx || !isprevy2 )
    {
	if ( y2userdefpolylineitm_ && !axisHandler(2) )
	    y2userdefpolylineitm_->setVisible( false );
	userdefy2str_.setEmpty(); y2rmserr_.setEmpty();
	setup().showy2userdefpolyline_ = drawy2userdefpolyline_ = false;
    }

    if ( !isprevx )
	x_.needautoscale_ = x_.autoscalepars_.doautoscale_ = true;
    if ( !isprevx || !isprevy )
	y_.needautoscale_ = y_.autoscalepars_.doautoscale_ = true;
    if ( !isprevx || !isprevy2 )
	y2_.needautoscale_ = y2_.autoscalepars_.doautoscale_ = true;

    mHandleAxisAutoScale(x_);
    mHandleAxisAutoScale(y_);
    mHandleAxisAutoScale(y2_);

    if ( yptitems_ )
	yptitems_->setVisible( axisHandler(1) );
    if ( y2ptitems_ )
	y2ptitems_->setVisible( axisHandler(2) );

    getRandRowids();
    calcStats();
    setDraw();
    drawContent();
}


void uiDataPointSetCrossPlotter::initDraw()
{
    x_.newDevSize();
    y_.newDevSize();
    y2_.newDevSize();
}


void uiDataPointSetCrossPlotter::setDraw()
{
    if ( x_.axis_ )
	x_.axis_->updateDevSize();

    if ( y_.axis_ )
	y_.axis_->updateDevSize();

    if ( y2_.axis_ )
	y2_.axis_->updateDevSize();
}


void uiDataPointSetCrossPlotter::drawContent( bool withaxis )
{
    drawColTabItem( true );
    drawColTabItem( false );

    if ( withaxis )
    {
	if ( x_.axis_ )
	    x_.axis_->updateScene();

	if ( y_.axis_ )
	    y_.axis_->updateScene();

	if ( y2_.axis_ )
	    y2_.axis_->updateScene();

	if ( !x_.axis_ || !y_.axis_ )
	{
	    if ( pixmapitm_ )
		pixmapitm_->hide();

	    return;
	}
    }

    drawData( y_, false );
    if ( y2_.axis_ )
	drawData( y2_, true );
    else if ( y2ptitems_ )
	y2ptitems_->removeAll( true );

    pointsSelected.trigger();
}


void uiDataPointSetCrossPlotter::prepareItems( bool y2 )
{
    uiGraphicsItemGroup*& ptitems = y2 ? y2ptitems_ : yptitems_;

    if ( !ptitems )
    {
	ptitems = new uiGraphicsItemGroup();
	scene().addItemGrp( ptitems );
	ptitems->setZValue( sMarkerZValue );
    }
}


void uiDataPointSetCrossPlotter::addItemIfNew( int itmidx,
					       MarkerStyle2D& mstyle,
					       uiGraphicsItemGroup* curitmgrp,
					       uiAxisHandler& yah,
					       uiDataPointSet::DRowID rid,
					       bool isy2 )
{
    if ( itmidx >= curitmgrp->size() )
    {
	uiGraphicsItem* itm = nullptr;
	if ( mstyle.type_ == MarkerStyle2D::None )
	    itm = new uiPointItem();
	else
	{
	    mstyle.size_ = 2;
	    itm = new uiMarkerItem( mstyle, false );
	}

	itm->setZValue( sMarkerZValue );
	curitmgrp->add( itm );
    }
}


void uiDataPointSetCrossPlotter::setItem( uiGraphicsItem* item, bool isy2,
	const uiPoint& pt )
{
    item->setPos( sCast(float,pt.x), sCast(float,pt.y) );
    item->setVisible( isy2 ? isY2Shown() : true );
}


void uiDataPointSetCrossPlotter::setAnnotEndTxt( uiAxisHandler& yah )
{
    const bool isy1 = y_.axis_ == &yah;
    if ( isy1 ? setup_.showy1cc_ : setup_.showy2cc_ )
    {
	const float fr100 = (isy1 ? lsy1_ : lsy2_).corrcoeff * 100;
	int r100 = mNINT32(fr100);
	BufferString txt( "r=" );
	if ( r100 < 0 )
	    { txt += "-"; r100 = -r100; }

	if ( r100 == 0 )	txt += "0";
	else if ( r100 == 100 )	txt += "1";
	else
	{
	    txt += "0.";
	    txt += r100%10 ? r100 : r100 / 10;
	}

	yah.annotAtEnd( toUiString(txt) );
    }
    else
	yah.annotAtEnd( uiStrings::sEmptyString() );
}


bool uiDataPointSetCrossPlotter::isSelectionValid( uiDataPointSet::DRowID rid )
{
    if ( (curgrp_ > 0 && dps_->group(rid) != curgrp_) )
	return false;

    if ( modcolidxs_.size() && mathobj_ )
    {
	for ( int idx=0; idx<modcolidxs_.size(); idx++ )
	{
	    const float yval = uidps_.getVal( modcolidxs_[idx], rid, true );
	    mathobj_->setVariableValue( idx, yval );
	}

	const float result = sCast(float,mathobj_->getValue());
	if ( mIsZero(result,mDefEps) || mIsUdf(result) )
	    return false;
    }

    return true;
}


OD::Color uiDataPointSetCrossPlotter::getOverlayColor(
			uiDataPointSet::DRowID rid, bool isy1 )
{
    const float yval = uidps_.getVal( isy1 ? y3colid_ : y4colid_ ,
				      rid, true );

    ColTab::Sequence& seq = isy1 ? y3ctab_ : y4ctab_;
    if ( mIsUdf(yval) ) return seq.undefColor();

    ColTab::Mapper& mapper = isy1 ? y3mapper_ : y4mapper_;
    return seq.color( mapper.position(yval) );
}


bool uiDataPointSetCrossPlotter::checkSelArea( const SelectionArea& area ) const
{
    Interval<double> xrg = area.getValueRange( true );
    Interval<double> rg = area.getValueRange( false );
    Interval<double> altrg = area.getValueRange( false, true );

    if ( !x_.axis_->range().includes(xrg.start,true) ||
	 !x_.axis_->range().includes(xrg.stop,true) )
	return false;
    if ( area.axistype_ == SelectionArea::Y1 )
	return y_.axis_->range().includes(rg.start,true) &&
	       y_.axis_->range().includes(rg.stop,true);
    else if ( area.axistype_ == SelectionArea::Y2 )
	return y2_.axis_->range().includes( rg.start,true ) &&
	       y2_.axis_->range().includes( rg.stop,true );

    return y_.axis_->range().includes(rg.start,true) &&
	   y_.axis_->range().includes(rg.stop,true) &&
	   y2_.axis_->range().includes(altrg.start,true) &&
	   y2_.axis_->range().includes(altrg.stop,true);
}


float uiDataPointSetCrossPlotter::getSelectedness( uiDataPointSet::DRowID rid,
						   bool fory2 )
{
    if ( !x_.axis_ || !y_.axis_ || (fory2 && !y2_.axis_)
	 || !isSelectionValid(rid) )
	return mUdf(float);

    if ( mIsUdf(curselgrp_) || !selgrpset_.validIdx(curselgrp_) )
	return mUdf(float);

    const SelectionGrp* selgrp = selgrpset_[curselgrp_];
    if ( !selgrp ) return mUdf(float);

    for ( int selidx=0; selidx<selgrp->size(); selidx++ )
    {
	const SelectionArea& selarea = selgrp->getSelectionArea( selidx );
	if ( (fory2 && selarea.axistype_ == SelectionArea::Y1) ||
	     (!fory2 && selarea.axistype_ == SelectionArea::Y2) )
	    continue;

	const uiDataPointSetCrossPlotter::AxisData& yad = fory2 ? y2_ : y_;

	uiWorld2Ui w2ui( uiSize(rgbarr_.getSize(true), rgbarr_.getSize(false)),
			 uiWorldRect((double)arrarea_.left(),
				     (double)arrarea_.top(),
				     (double)arrarea_.right(),
				     (double)arrarea_.bottom()) );
	const float xval = uidps_.getVal( x_.colid_, rid, true );
	const float yval = uidps_.getVal( yad.colid_, rid, true );
	const uiPoint pt( x_.axis_->getPix(xval), yad.axis_->getPix(yval) );
	const uiWorldPoint wpt( (double)x_.axis_->getPix(xval),
				(double)yad.axis_->getPix(yval) );
	const bool itmselected = selarea.isInside( pt );

	if ( itmselected )
	    return selarea.selectedness( pt );
    }

    return mUdf(float);
}

mStartAllowDeprecatedSection
void uiDataPointSetCrossPlotter::checkSelection( uiDataPointSet::DRowID rid,
	     uiGraphicsItem* item, bool isy2,
	     const uiDataPointSetCrossPlotter::AxisData& yad, bool removesel )
{
    if ( !x_.axis_ || isy2 ? !y2_.axis_ : !y_.axis_ )
	return;

    uiWorld2Ui w2ui( uiSize(rgbarr_.getSize(true), rgbarr_.getSize(false)),
		     uiWorldRect((double)arrarea_.left(),(double)arrarea_.top(),
				 (double)arrarea_.right(),
				 (double)arrarea_.bottom()) );
    const float xval = uidps_.getVal( x_.colid_, rid, true );
    const float yval = uidps_.getVal( yad.colid_, rid, true );
    const uiPoint pt( x_.axis_->getPix(xval), yad.axis_->getPix(yval) );
    const uiWorldPoint wpt( (double)x_.axis_->getPix(xval),
			    (double)yad.axis_->getPix(yval) );

    mDynamicCastGet(uiMarkerItem*,markeritem,item)
    if ( markeritem )
    {
	OD::Color pencolor( 50, 50, 50 );
	OD::Color fillcolor = yad.axis_->setup().style_.color_;
	if ( !isy2 && showy3_ )
	{
	    fillcolor = getOverlayColor( rid, true );
	}
	else if ( isy2 && showy4_ && isY2Shown() )
	{
	    fillcolor = getOverlayColor( rid, false );
	}
	else if ( multclron_ )
	{
	    unsigned short grpid = dps_->group(rid)-1;
	    if ( !isy2 && y1grpcols_.validIdx(grpid) )
		fillcolor = y1grpcols_[grpid];
	    if ( isy2 && y2grpcols_.validIdx(grpid) )
		fillcolor = y2grpcols_[grpid];
	}

	markeritem->setMarkerStyle( setup_.markerstyle_ );
	markeritem->setPenColor( pencolor );
	markeritem->setFillColor( fillcolor );
    }

    bool ptselected = false;
    for ( int idx=0; idx<selgrpset_.size(); idx++ )
    {
	const SelectionGrp* selgrp = selgrpset_[idx];
	for ( int selidx=0; selidx<selgrp->size(); selidx++ )
	{
	    const SelectionArea& selarea = selgrp->getSelectionArea( selidx );
	    const bool itmselected = selarea.isInside( pt );

	    if ( (selarea.axistype_ ==SelectionArea::Y1 ||
		  selarea.axistype_ ==SelectionArea::Both) && !isy2 )
	    {
		if ( itmselected )
		{
		    if ( !isSelectionValid(rid) )
			continue;

		    if ( removesel )
		    {
			if ( item )
			    item->setVisible( false );
			BinIDValueSet::SPos pos = dps_->bvsPos(rid);
			float* vals = dps_->bivSet().getVals( pos );
			vals[ dps_->nrFixedCols()+y_.colid_ ] = mUdf(float);
			yrowidxs_->set( rid, '0' );
			return;
		    }

		    dps_->setSelected( rid, getSelGrpIdx(selarea.id_) );
		    dpsselrowcols_ += RowCol( rid, yad.colid_ );
		    selrowcols_ += RowCol( uidps_.tRowID(rid),
					   uidps_.tColID(yad.colid_) );

		    if ( item )
			item->setPenColor( selgrp->col_ );
		    selyitems_++;
		    ptselected = true;
		}
	    }

	    if ( (selarea.axistype_ ==SelectionArea::Y2 ||
		  selarea.axistype_ ==SelectionArea::Both)
		  && y2ptitems_ && isY2Shown() && isy2 )
	    {
		if ( itmselected )
		{
		    if ( !isSelectionValid(rid) )
			continue;

		    if ( removesel )
		    {
			if ( item )
			    item->setVisible( false );
			BinIDValueSet::SPos pos = dps_->bvsPos(rid);
			float* vals = dps_->bivSet().getVals( pos );
			vals[ dps_->nrFixedCols()+y2_.colid_ ] = mUdf(float);
			y2rowidxs_->set( rid, '0' );
			return;
		    }

		    if ( item )
			item->setPenColor( selgrp->col_ );

		    dps_->setSelected( rid, getSelGrpIdx(selarea.id_) );
		    dpsselrowcols_ += RowCol( rid, yad.colid_ );
		    selrowcols_ += RowCol( uidps_.tRowID(rid),
					   uidps_.tColID(yad.colid_) );

		    sely2items_++;
		    ptselected = true;
		}
	    }
	}
    }

    if ( !ptselected )
    {
	if ( isy2 && dps_->isSelected(rid) )
	    return;

	dps_->setSelected( rid, -1 );
    }
}
mStopAllowDeprecatedSection

void uiDataPointSetCrossPlotter::setNrBins( OD::Pair<int,int> nrbins )
{
    nrbins_ = nrbins;
}


OD::Pair<int,int> uiDataPointSetCrossPlotter::nrBins() const
{
    return nrbins_;
}


void uiDataPointSetCrossPlotter::setDensityPlot( bool yn, bool showy2 )
{
    isdensityplot_ = yn;
    if ( y2ptitems_ && !y2ptitems_->isVisible() )
	y2ptitems_->setVisible( showy2 );
}


void uiDataPointSetCrossPlotter::drawDensityPlot()
{
    if ( !x_.axis_ || !y_.axis_ )
	return;

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    isdensityplot_ = true;
    drawTypeChanged.trigger( true );
    const uiAxisHandler& xah = *x_.axis_;
    const uiAxisHandler& yah = *y_.axis_;
    border_ = uiBorder( xah.pixBefore(), yah.pixAfter(),
			xah.pixAfter(), yah.pixBefore() );
    beforeDraw();

    if ( arrarea_.left()>arrarea_.right() || arrarea_.top()>arrarea_.bottom() )
	return;

    DensityCalc densitycalc( uidps_, x_, y_, nrbins_ );
    if ( !densitycalc.execute() )
	return;

    const float maxval = densitycalc.maxValue();
    Interval<float> mapperrg( 1, maxval );
    if ( ctmapper_.range().start<1 || ctmapper_.range().stop>maxval )
	ctmapper_.setRange( mapperrg );
    else
	mapperrg.limitTo( ctmapper_.range() );
    coltabRgChanged.trigger( mapperrg );

    const Array2D<int>& densitydata = densitycalc.getDensityData();
    Density2RGBArray d2a( densitydata, rgbarr_, ctab_, ctmapper_ );
    d2a.setUndefColor( OD::Color(240,240,240) );
    if ( !d2a.execute() )
	return;

    createPixmap();
    updatePixmap();
    pixmapitm_->show();
}


bool uiDataPointSetCrossPlotter::drawRID( uiDataPointSet::DRowID rid,
	uiGraphicsItemGroup* curitmgrp,
	const uiDataPointSetCrossPlotter::AxisData& yad, bool isy2,
	MarkerStyle2D& mstyle, int itmidx, bool remsel )
{
    uiAxisHandler& xah = *x_.axis_;
    uiAxisHandler& yah = *yad.axis_;
    if ( !x_.axis_ || !yad.axis_ ) return false;

    const Interval<int> xpixrg( xah.pixRange() ), ypixrg( yah.pixRange() );
    if ( dps_->isInactive(rid) ||
	    (curgrp_>0 && dps_->group(rid)!=curgrp_) )
	return false;

    const float xval = uidps_.getVal( x_.colid_, rid, true );
    const float yval = uidps_.getVal( yad.colid_, rid, true );
    if ( mIsUdf(xval) || mIsUdf(yval) )
	return false;

    const uiPoint pt( xah.getPix(xval), yah.getPix(yval) );
    if ( !xpixrg.includes(pt.x,true) || !ypixrg.includes(pt.y,true) )
	return false;

    addItemIfNew( itmidx, mstyle, curitmgrp, yah, rid, isy2 );
    setItem( curitmgrp->getUiItem(itmidx), isy2, pt );
    checkSelection( rid, curitmgrp->getUiItem(itmidx), isy2, yad, remsel );

    if ( itmidx > 1 )
	usedxpixrg_.include( pt.x );
    else
	usedxpixrg_ = Interval<int>( pt.x, pt.x );

    return true;
}


bool uiDataPointSetCrossPlotter::drawPoints( uiGraphicsItemGroup* curitmgrp,
	const uiDataPointSetCrossPlotter::AxisData& yad, bool isy2,
	MarkerStyle2D& mstyle, bool removesel )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    isdensityplot_ = false;
    drawTypeChanged.trigger( false );
    if ( pixmapitm_ )
	pixmapitm_->hide();

    int itmidx = 0;
    usedxpixrg_ = Interval<int>(0,0);
    Array1D<char>* rowidx = isy2 ? y2rowidxs_ : yrowidxs_;
    for ( int idx=0; idx<rowidx->info().getSize(0); idx++ )
    {
	if ( rowidx->get(idx) == '0' )
	    continue;

	uiDataPointSet::DRowID rid = idx;
	if ( !drawRID(rid,curitmgrp,yad,isy2,mstyle,itmidx,removesel) )
	    continue;

	itmidx++;
    }

    if ( itmidx < curitmgrp->size() )
    {
	for ( int idx=itmidx; idx<curitmgrp->size(); idx++ )
	    curitmgrp->getUiItem(idx)->setVisible(false);
    }

    if ( itmidx < 1 )
	return false;

    return true;
}


void uiDataPointSetCrossPlotter::drawData( const AxisData& yad, bool isy2,
					   bool rempts )
{
    MouseCursorChanger ms( MouseCursor::Wait );
    if ( !x_.axis_ || !yad.axis_ )
	return;

    uiAxisHandler& yah = *yad.axis_;

    MarkerStyle2D mstyle( setup_.markerstyle_ );
    mstyle.color_.setRgb( yah.setup().style_.color_.rgb() );

    prepareItems( isy2 );
    uiGraphicsItemGroup* curitmgrp = isy2 ? y2ptitems_ : yptitems_ ;
    if ( isy2 && (!curitmgrp || !curitmgrp->isVisible()) )
	return;

    if ( isdensityplot_ )
    {
	curitmgrp->setVisible( false );
	if ( !isy2 )
	    drawDensityPlot();
    }
    else if ( !drawPoints(curitmgrp,yad,isy2,mstyle,rempts) )
	return;

    setAnnotEndTxt( yah );

    drawRegrLine( &yah, usedxpixrg_ );

    drawUserDefPolyLine( true );
    drawUserDefPolyLine( false );
}


void uiDataPointSetCrossPlotter::drawRegrLine( uiAxisHandler* yah,
					       const Interval<int>& xpixrg )
{
    if ( !x_.axis_ || !yah )
	return;

    const bool isy1 = y_.axis_ == yah;
    const bool showline =
	isy1 ? setup().showy1regrline_ : setup().showy2regrline_;
    if ( !showline )
    {
	if ( isy1 && y1regrlineitm_ )
	    y1regrlineitm_->hide();
	if ( !isy1 && y2regrlineitm_ )
	    y2regrlineitm_->hide();
	return;
    }

    const uiAxisHandler* xah = x_.axis_;
    const LinStats2D& ls = isy1 ? lsy1_ : lsy2_;
    const Interval<int> ypixrg( yah->pixRange() );
    Interval<float> xvalrg( xah->getVal(xpixrg.start),
			    xah->getVal(xpixrg.stop) );
    Interval<float> yvalrg( yah->getVal(ypixrg.start),
			    yah->getVal(ypixrg.stop) );

    uiLineItem* lineitm = isy1 ? y1regrlineitm_ : y2regrlineitm_;
    if ( lineitm )
    {
	setLine( lineitm, ls.lp, xah, yah, &xvalrg );
	lineitm->show();
	return;
    }

    lineitm = new uiLineItem;
    if ( isy1 )
	y1regrlineitm_ = lineitm;
    else
	y2regrlineitm_ = lineitm;

    scene().addItem( lineitm );
    lineitm->setZValue( sLineZValue );
    const OD::Color pencol = OD::Color::stdDrawColor( isy1 ? 2 : 3 );
    OD::LineStyle linestyle;
    linestyle.color_ = pencol;
    linestyle.width_ = 2;
    lineitm->setPenStyle( linestyle );
    setLine( lineitm, ls.lp, xah, yah, &xvalrg );
}

mStartAllowDeprecatedSection
const TypeSet<RowCol>& uiDataPointSetCrossPlotter::getSelectedCells()
{
    return selrowcols_;
}
mStopAllowDeprecatedSection
