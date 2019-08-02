/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uidatapointsetcrossplot.h"

#include "uidatapointset.h"
#include "uicoltabgraphicsitem.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uipixmap.h"
#include "uirgbarray.h"
#include "uitaskrunner.h"
#include "uimsg.h"

#include "arrayndimpl.h"
#include "coltabseqmgr.h"
#include "datadistributionextracter.h"
#include "densitycalc.h"
#include "linear.h"
#include "polyfit.h"
#include "statrand.h"
#include "timer.h"


uiDataPointSetCrossPlotter::uiDataPointSetCrossPlotter( uiParent* p,
			    uiDataPointSet& uidp,
			    const uiDataPointSetCrossPlotter::Setup& su )
    : uiRGBArrayCanvas(p,*new uiRGBArray(false))
    , dps_(uidp.pointSet())
    , uidps_(uidp)
    , setup_(su)
    , mincolid_(1-uidp.pointSet().nrFixedCols())
    , selrow_(-1)
    , x_(*this,uiRect::Bottom)
    , y_(*this,uiRect::Left)
    , y2_(*this,uiRect::Right)
    , y3colid_(mUdf(int))
    , y4colid_(mUdf(int))
    , ctab_(ColTab::SeqMGR().getDefault(false))
    , y3ctab_(ColTab::SeqMGR().getDefault(false))
    , y4ctab_(ColTab::SeqMGR().getDefault(false))
    , lineDrawn( this )
    , mouseReleased( this )
    , dataChgd( this )
    , selectionChanged( this )
    , pointsSelected( this )
    , removeRequest( this )
    , drawTypeChanged( this )
    , coltabRgChanged( this )
    , plotperc_(1)
    , curgrp_(0)
    , curselgrp_(0)
    , selrowisy2_(false)
    , rectangleselection_(true)
    , lsy1_(*new LinStats2D)
    , lsy2_(*new LinStats2D)
    , mathobj_(0)
    , userdefy1lp_(*new LinePars)
    , userdefy2lp_(*new LinePars)
    , userdefy1str_(*new BufferString)
    , userdefy2str_(*new BufferString)
    , y1rmserr_(*new BufferString)
    , y2rmserr_(*new BufferString)
    , yptitems_(0)
    , y2ptitems_(0)
    , selectionpolygonitem_(0)
    , selectionrectitem_(0)
    , regrlineitm_(0)
    , y1userdefpolylineitm_(0)
    , y2userdefpolylineitm_(0)
    , selyitems_(0)
    , sely2items_(0)
    , selrectitems_(0)
    , selpolyitems_(0)
    , y1overlayctitem_(0)
    , y2overlayctitem_(0)
    , yrowidxs_(0)
    , y2rowidxs_(0)
    , ctmapper_(new ColTab::Mapper)
    , y3mapper_(new ColTab::Mapper)
    , y4mapper_(new ColTab::Mapper)
    , cellsize_(1)
    , showy3_(false)
    , showy4_(false)
    , selectable_(false)
    , isy1selectable_(true)
    , isy2selectable_(false)
    , mousepressed_(false)
    , isdensityplot_(false)
    , multclron_(false)
    , drawuserdefline_(false)
    , drawy1userdefpolyline_(false)
    , drawy2userdefpolyline_(false)
    , drawy2_(false)
    , timer_(*new Timer())
    , trmsg_(tr("Calculating Density"))
{
    setup_.showy1userdefpolyline(false).showy2userdefpolyline(false);

    enableImageSave();
    enableScrollZoom();
    x_.defaxsu_.style_ = setup_.xstyle_;
    y_.defaxsu_.style_ = setup_.ystyle_;
    y2_.defaxsu_.style_ = setup_.y2style_;
    x_.defaxsu_.gridlinestyle_ = setup_.xstyle_;
    y_.defaxsu_.gridlinestyle_ = setup_.ystyle_;
    y2_.defaxsu_.gridlinestyle_ = setup_.y2style_;
    x_.defaxsu_.border_ = setup_.minborder_;
    y_.defaxsu_.border_ = setup_.minborder_;
    y2_.defaxsu_.border_ = setup_.minborder_;

    y3mapper_->setup().setNoClipping();
    y4mapper_->setup().setNoClipping();

    reSize.notify( mCB(this,uiDataPointSetCrossPlotter,reSizeDrawCB) );
    reDrawNeeded.notify( mCB(this,uiDataPointSetCrossPlotter,reDrawCB) );
    getMouseEventHandler().buttonPressed.notify(
	    mCB(this,uiDataPointSetCrossPlotter,mouseClickedCB) );
    getMouseEventHandler().movement.notify(
	    mCB(this,uiDataPointSetCrossPlotter,mouseMoveCB) );
    getMouseEventHandler().buttonReleased.notify(
	    mCB(this,uiDataPointSetCrossPlotter,mouseReleasedCB) );

    timer_.tick.notify( mCB(this,uiDataPointSetCrossPlotter,reDrawCB) );
    setStretch( 2, 2 );
    setSceneBorder( 2 );
    setDragMode( uiGraphicsView::ScrollHandDrag );

    yrowidxs_ = new Array1DImpl<char>( dps_.size() );
    y2rowidxs_ = new Array1DImpl<char>( dps_.size() );
    selgrpset_ += new SelectionGrp( "No 1", Color::DgbColor() );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
}


uiDataPointSetCrossPlotter::~uiDataPointSetCrossPlotter()
{
    timer_.tick.remove( mCB(this,uiDataPointSetCrossPlotter,reDrawCB) );

    if ( yptitems_ ) scene().removeItem( yptitems_ );
    if ( y2ptitems_ ) scene().removeItem( y2ptitems_ );
    if ( selrectitems_ ) scene().removeItem( selrectitems_ );
    if ( selrectitems_ ) scene().removeItem( selrectitems_ );

    delete &lsy1_;
    delete &lsy2_;
}

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
    if ( isdensityplot_ )
    {
	trmsg_ = tr("Calculating Density");
	timer_.start( 1200, true );
	return;
    }

    reDrawCB( 0 );
}


void uiDataPointSetCrossPlotter::reDrawCB( CallBacker* )
{
    uiUserShowWait usw( parent(), uiStrings::sUpdatingDisplay() );
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
    const int totalrows = mNINT32( plotperc_/(float)100 * dps_.nrActive() );
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
    const int totalrows = mNINT32( plotperc_/(float)100 * dps_.nrActive() );
    if ( mIsEqual(plotperc_,100,mDefEps) )
    {
	yrowidxs_->ArrayND<char>::setAll( '1' );
	y2rowidxs_->ArrayND<char>::setAll( '1' );
	return;
    }

    Stats::RandGen randgen;
    for ( bool doy1 : {true,false} )
    {
	int rowcount =0;

	Array1D<char>* rowidxs = doy1 ? yrowidxs_ : y2rowidxs_;
	const bool highperc = plotperc_ > 50;
	rowidxs->ArrayND<char>::setAll( highperc ? '1' : '0' );
	const float nrrowneeded = mCast( float, highperc ?
				   dps_.nrActive() - totalrows : totalrows );
	while ( rowcount < nrrowneeded )
	{
	    int randrow = randgen.getIndex( dps_.size() );
	    if ( ((!highperc && rowidxs->get(randrow) == '0') ||
		  (highperc && rowidxs->get(randrow) == '1')) &&
		 !dps_.isInactive(randrow) )
		rowidxs->set( randrow, highperc ? '0' : '1' );
	    else
		continue;
	    rowcount++;
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
    uiUserShowWait usw( parent(), uiStrings::sUpdatingDisplay() );

    uidps_.setUnsavedChg( true );
    if ( isdensityplot_ )
    {
	trmsg_ = tr("Deleting Selected Points");
	drawDensityPlot( true );
    }
    else
    {
	for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
	{
	    checkSelection( rid, 0, false, y_, true );
	    if ( y2_.axis_ )
		checkSelection( rid, 0, true, y2_, true );
	}

	drawData( y_, false, true );
	if ( y2_.axis_ )
	    drawData( y2_ , true, true );
    }

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


TypeSet<Color> uiDataPointSetCrossPlotter::selGrpCols() const
{
    TypeSet<Color> selcols;
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
	for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
	{
	    if ( dps_.isInactive(rid) ) continue;

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


void uiDataPointSetCrossPlotter::removeSelections( bool remfrmselgrp )
{
    removeSelectionItems();
    selrowcols_.erase();
    selyitems_ = 0;
    sely2items_ = 0;

    if ( remfrmselgrp )
    {
	for ( int idx=0; idx<selgrpset_.size(); idx++ )
	    selgrpset_[idx]->removeAll();
    }

    for ( int idx=0; idx<dps_.size(); idx++ )
	dps_.setSelected( idx, -1 );
}


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
    if ( !xah || !yah ) return;

    const int size = pts.size();
    if ( !size ) return;

    TypeSet<uiPoint> pixpts;
    for ( int pixvar = 0; pixvar < size; pixvar++ )
    {
	uiWorldPoint pt = pts[pixvar];
	if ( mIsUdf(pt.x_) || mIsUdf(pt.y_) ) continue;
	if ( mIsUdf(xah->getPix(pt.x_)) || mIsUdf(yah->getPix(pt.y_)) )
	    continue;

	if ( !mousepressed_ )
	{
	    if (!xah->pixRange().includes(xah->getPix(pt.x_),false) ) continue;
	    if (!yah->pixRange().includes(yah->getPix(pt.y_),false) ) continue;
	}

	pixpts += uiPoint( xah->getPix(pt.x_), yah->getPix(pt.y_) );
    }

    curpolylineitem->setPolyLine( pixpts );
    OD::LineStyle ls = yah->setup().style_;
    ls.width_ = 3;
    curpolylineitem->setPenStyle( ls );
    curpolylineitem->setZValue( 4 );
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
		selpolyitems_->setZValue( 5 );
	    }

	    selpolyitems_->add( selectionpolygonitem_ );
	}
	selectionpolygonitem_->setPenColor( selgrpset_[curselgrp_]->col_ );
    }

    curselarea_ = selgrp->size() - 1;
    SelectionArea& selarea = getCurSelArea();
    if ( !axisHandler(0) || !axisHandler(1) )
	return;
    selarea.xaxisnm_ = toString( axisHandler(0)->getCaption() );
    selarea.yaxisnm_ = toString(
	axisHandler( isy1selectable_ ? 1 : 2 )->getCaption() );
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
	    xah.getVal(stoppos.x_) - xah.getVal(startpos_.x_);
	const float perpendicular =
	    yah.getVal(stoppos.y_) - yah.getVal(startpos_.y_);

	if ( !mIsZero(base,1e-6) )
	{
	    linepar.ax_ = perpendicular/base;
	    linepar.a0_ = yah.getVal(startpos_.y_) -
			( linepar.ax_ * xah.getVal(startpos_.x_) );
	}

	BufferString& linestr = drawy2_ ? userdefy2str_ : userdefy1str_;
	BufferString& rmserr = drawy2_ ? y2rmserr_ : y1rmserr_;
	linestr.setEmpty(); rmserr.setEmpty();

	if ( !mIsZero(base,1e-6) )
	{
	    if ( linepar.ax_ )
	    {
		linestr += linepar.ax_;
		linestr += "*x";
		if ( linepar.a0_ > 0 ) linestr += "+";
	    }
	    linestr += linepar.a0_;
	}

	TypeSet<uiWorldPoint> linepts;
	linepts += uiWorldPoint( xah.getVal(startpos_.x_),
				 yah.getVal(startpos_.y_) );
	linepts += uiWorldPoint( xah.getVal(stoppos.x_),
				 yah.getVal(stoppos.y_) );
	setUserDefPolyLine( linepts,drawy2_ );
	drawUserDefPolyLine( !drawy2_ );
	lineDrawn.trigger();
	return;
    }

    if ( !selectable_ || rectangleselection_ ) return;

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
	    selpolyitems_->setZValue( 5 );
	}

	selpolyitems_->add( selectionpolygonitem_ );
    }
    selectionpolygonitem_->setPolygon( selarea.poly_ );
}


float uiDataPointSetCrossPlotter::getVal( int colid, int rid ) const
{
    return uidps_.getVal( colid, rid, true );
}


void uiDataPointSetCrossPlotter::setCTMapper( const ColTab::Mapper& mpr )
{
    *ctmapper_ = mpr;
}


void uiDataPointSetCrossPlotter::setShowY3( bool yn )
{
    showy3_ = yn;
    if ( y1overlayctitem_ )
	y1overlayctitem_->setMapper( y3mapper_ );
}


void uiDataPointSetCrossPlotter::setShowY4( bool yn )
{
    showy4_ = yn;
    if ( y2overlayctitem_ )
	y2overlayctitem_->setMapper( y4mapper_ );
}


void uiDataPointSetCrossPlotter::drawColTabItem( bool isy1 )
{
    if ( !axisHandler(0) || !axisHandler(isy1? 1:2) )
	return;

    if ( (isy1 && !showy3_) || (!isy1 && !showy4_) )
    {
	if ( y1overlayctitem_ ) y1overlayctitem_->setVisible( showy3_ );
	if ( y2overlayctitem_ )
	    y2overlayctitem_->setVisible( showy4_ && isY2Shown() );
	if ( x_.axis_ && !showy3_ )
	    x_.axis_->setup().border_ = setup_.minborder_;
	return;
    }

    uiColTabItem* coltabitem = isy1 ? y1overlayctitem_ : y2overlayctitem_;
    uiColTabItem::Setup ctsu( true );

    if ( !coltabitem )
    {
	coltabitem = new uiColTabItem( ctsu );
	if ( isy1 )
	    y1overlayctitem_ = coltabitem;
	else
	    y2overlayctitem_ = coltabitem;
	scene().addItem( coltabitem );
    }

    uiBorder extraborder = setup_.minborder_;
    extraborder.setBottom( extraborder.bottom() + ctsu.sz_.height() );
    x_.axis_->setup().border_ = extraborder;
    y_.axis_->setup().border_ = extraborder;
    if ( y2_.axis_ )
	y2_.axis_->setup().border_ = extraborder;
    setDraw();

    const int xpos = isy1 ? x_.axis_->pixBefore()
			  : width() - x_.axis_->pixAfter() - ctsu.sz_.width();
    const int ypos = height() - extraborder.bottom() - ctsu.sz_.height();
    coltabitem->setPos( Geom::Point2D<float>(mCast(float,xpos),
					     mCast(float,ypos)) );
    ConstRefMan<ColTab::Sequence> ctab = isy1 ? y3ctab_ : y4ctab_;
    coltabitem->setSequence( *ctab );
    coltabitem->setMapper( isy1 ? y3mapper_ : y4mapper_ );
    coltabitem->setVisible( isy1 ? showy3_ : ( showy4_ && isY2Shown() ) );
}


void uiDataPointSetCrossPlotter::updateOverlayMapper( bool isy1 )
{
    if ( mIsUdf(isy1 ? y3colid_ : y4colid_) )
	return;

    TypeSet<float> ydata;
    for ( int idx=0; idx<dps_.size(); idx++ )
	ydata += uidps_.getVal( isy1 ? y3colid_ : y4colid_ , idx, true );

    ColTab::Mapper& mapper = *(isy1 ? y3mapper_ : y4mapper_);
    RangeLimitedDataDistributionExtracter<float> extr( ydata,
						SilentTaskRunnerProvider() );
    mapper.distribution() = *extr.getDistribution();
}


void uiDataPointSetCrossPlotter::setOverlayY1AttMapr( const ColTab::Mapper& m )
{ *y3mapper_ = m; }

void uiDataPointSetCrossPlotter::setOverlayY2AttMapr( const ColTab::Mapper& m )
{ *y4mapper_ = m; }

void uiDataPointSetCrossPlotter::setOverlayY1AttSeq( const ColTab::Sequence& ct)
{ y3ctab_ = &ct; }

void uiDataPointSetCrossPlotter::setOverlayY2AttSeq( const ColTab::Sequence& ct)
{ y4ctab_ = &ct; }


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
	    worldpoly.add( uiWorldPoint(xah.getVal(polypts[idx].x_),
					yah.getVal(polypts[idx].y_)) );
	    if ( selarea.axistype_ == SelectionArea::Both )
	    {
		altworldpoly.add( uiWorldPoint(xah.getVal(polypts[idx].x_),
				  y2_.axis_->getVal(polypts[idx].y_)) );
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
	    const Color& col = selgrp->col_;
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
		    selrectitems_->setZValue( 5 );
		}

		if ( nrrect >= selrectitems_->size() )
		{
		    selrectitem = new uiRectItem();
		    selrectitems_->add( selrectitem );
		}

		uiGraphicsItem* itm = selrectitems_->getUiItem( nrrect );
		if ( !itm ) continue;
		mDynamicCast( uiRectItem*, selrectitem, itm );
		if ( !selrectitem ) continue;
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
		    poly.add( uiPoint(xah.getPix(polypts[nrpts].x_),
				      yah.getPix(polypts[nrpts].y_)) );
		selarea.poly_= poly;

		if ( !selpolyitems_ || selpolyitems_->size() <= nrpoly )
		{
		    if ( !selpolyitems_ )
		    {
			selpolyitems_ = new uiGraphicsItemGroup();
			scene().addItemGrp( selpolyitems_ );
			selpolyitems_->setZValue( 5 );
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
	    selrectitems_->setZValue( 5 );
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
	for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
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
    if ( inpsz < 2 )
	{ ls = LinStats2D(); return; }

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
	if ( dps_.isInactive(rid) || (curgrp_>0 && dps_.group(rid) != curgrp_) )
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

    polyfit_.setEmpty();
    if ( polyfitorder_ > 1 )
	polyfit_ = polyFit( xvals.arr(), yvals.arr(), xvals.size(),
			    polyfitorder_ );
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

    const float xpix = x_.axis_->getRelPos( x_.axis_->getVal(pt.x_) );
    const float ypix = ad.axis_->getRelPos( ad.axis_->getVal(pt.y_) );
    int row = -1; float mindistsq = 1e30;
    for ( uiDataPointSet::DRowID rid=0; rid<dps_.size(); rid++ )
    {
	if ( (curgrp_ > 0 && dps_.group(rid) != curgrp_) )
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
	    PtrMan<uiPixmap> pixmap =
		new uiPixmap( arrarea_.width(),arrarea_.height());
	    pixmap->fill( Color::White() );
	    setPixmap( *pixmap );
	    updatePixmap();
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
	ptitems->setZValue( 4 );
    }
}


void uiDataPointSetCrossPlotter::addItemIfNew( int itmidx,
					       OD::MarkerStyle2D& mstyle,
				       uiGraphicsItemGroup* curitmgrp,
				       uiAxisHandler& yah,
				       uiDataPointSet::DRowID rid,
				       bool isy2 )
{
    if ( itmidx >= curitmgrp->size() )
    {
	uiGraphicsItem* itm = 0;
	if ( mstyle.type_ == OD::MarkerStyle2D::None )
	    itm = new uiPointItem();
	else
	{
	    mstyle.size_ = 2;
	    itm = new uiMarkerItem( mstyle, false );
	}

	itm->setZValue( 4 );
	curitmgrp->add( itm );
    }
}


void uiDataPointSetCrossPlotter::setItem( uiGraphicsItem* item, bool isy2,
	const uiPoint& pt )
{
    item->setPos( mCast(float,pt.x_), mCast(float,pt.y_) );
    item->setVisible( isy2 ? isY2Shown() : true );
}


void uiDataPointSetCrossPlotter::setAnnotEndTxt( uiAxisHandler& yah )
{
    if ( setup_.showcc_ )
    {
	float fr100 = (y_.axis_ == &yah ? lsy1_ : lsy2_).corrcoeff * 100;
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
	yah.annotAtEnd( uiString::empty() );
}


bool uiDataPointSetCrossPlotter::isSelectionValid( uiDataPointSet::DRowID rid )
{
    if ( (curgrp_ > 0 && dps_.group(rid) != curgrp_) )
	return false;

    if ( modcolidxs_.size() && mathobj_ )
    {
	for ( int idx=0; idx<modcolidxs_.size(); idx++ )
	{
	    const float yval = uidps_.getVal( modcolidxs_[idx], rid, true );
	    mathobj_->setVariableValue( idx, yval );
	}

	const float result = mCast(float,mathobj_->getValue());
	if ( mIsZero(result,mDefEps) || mIsUdf(result) )
	    return false;
    }

    return true;
}


Color uiDataPointSetCrossPlotter::getOverlayColor( uiDataPointSet::DRowID rid,
						   bool isy1 )
{
    const float yval = uidps_.getVal( isy1 ? y3colid_ : y4colid_ ,
				      rid, true );

    ConstRefMan<ColTab::Sequence> seq = isy1 ? y3ctab_ : y4ctab_;
    if ( mIsUdf(yval) ) return seq->undefColor();

    const ColTab::Mapper& mapper = *(isy1 ? y3mapper_ : y4mapper_);
    return seq->color( mapper.seqPosition(yval) );
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


void uiDataPointSetCrossPlotter::checkSelection( uiDataPointSet::DRowID rid,
	     uiGraphicsItem* item, bool isy2,
	     const uiDataPointSetCrossPlotter::AxisData& yad, bool removesel )
{
    if ( !x_.axis_ || isy2 ? !y2_.axis_ : !y_.axis_ ) return;
    uiWorld2Ui w2ui( uiSize(rgbarr_.getSize(true), rgbarr_.getSize(false)),
		     uiWorldRect((double)arrarea_.left(),(double)arrarea_.top(),
				 (double)arrarea_.right(),
				 (double)arrarea_.bottom()) );
    const float xval = uidps_.getVal( x_.colid_, rid, true );
    const float yval = uidps_.getVal( yad.colid_, rid, true );
    const uiPoint pt( x_.axis_->getPix(xval), yad.axis_->getPix(yval) );
    const uiWorldPoint wpt( (double)x_.axis_->getPix(xval),
			    (double)yad.axis_->getPix(yval) );

    unsigned short grpid = dps_.group(rid)-1;
    if ( item && !isy2 && showy3_ )
    {
	mDynamicCastGet(uiMarkerItem*,markeritem,item)
	if ( markeritem ) markeritem->setFillColor( getOverlayColor(rid,true) );
    }
    else if ( item && isy2 && showy4_ && isY2Shown() )
    {
	mDynamicCastGet(uiMarkerItem*,markeritem,item)
	if ( markeritem ) markeritem->setFillColor( getOverlayColor(rid,false));
    }
    else
    {
	mDynamicCastGet(uiMarkerItem*,markeritem,item)
	if ( markeritem ) markeritem->setFill( false );
    }

    if ( item )
    {
	const Color& multicol = isy2 ? !y2grpcols_.validIdx(grpid)
				     ? Color::White() : y2grpcols_[grpid]
				     : !y1grpcols_.validIdx(grpid)
				     ? Color::White() : y1grpcols_[grpid];
	Color overlaycol = yad.axis_->setup().style_.color_;
	if ( showy3_ && !isy2 )
	    overlaycol = getOverlayColor(rid,true);
	else if ( showy4_ && isy2 && isY2Shown() )
	    overlaycol = getOverlayColor(rid,false);

	OD::MarkerStyle2D mstyle( setup_.markerstyle_ );
	mstyle.color_.setRgb( yad.axis_->setup().style_.color_.rgb() );
	mDynamicCastGet(uiMarkerItem*,markeritem,item)
	if ( markeritem ) markeritem->setMarkerStyle( mstyle );

	item->setPenColor(!multclron_ ? overlaycol : multicol );
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
			BinnedValueSet::SPos pos = dps_.bvsPos(rid);
			float* vals = dps_.bivSet().getVals( pos );
			vals[ dps_.nrFixedCols()+y_.colid_ ] = mUdf(float);
			yrowidxs_->set( rid, '0' );
			return;
		    }

		    dps_.setSelected( rid, getSelGrpIdx(selarea.id_) );
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
			BinnedValueSet::SPos pos = dps_.bvsPos(rid);
			float* vals = dps_.bivSet().getVals( pos );
			vals[ dps_.nrFixedCols()+y2_.colid_ ] = mUdf(float);
			y2rowidxs_->set( rid, '0' );
			return;
		    }

		    if ( item )
			item->setPenColor( selgrp->col_ );

		    dps_.setSelected( rid, getSelGrpIdx(selarea.id_) );
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
	if ( isy2 && dps_.isSelected(rid) )
	    return;

	dps_.setSelected( rid, -1 );
    }

}


int uiDataPointSetCrossPlotter::calculateDensity( Array2D<float>* data,
						  bool chgdps, bool removesel )
{ return calcDensity( data, chgdps, removesel, false, 0 ); }


int uiDataPointSetCrossPlotter::calcDensity( Array2D<float>* data, bool chgdps,
					     bool removesel, bool isy2,
					     int areatype,
					     Interval<int>* nrbins,
					     Array2D<float>* freqdata )
{
    AxisData& yad = isy2 ? y2_ : y_;
    if ( !x_.axis_ || !yad.axis_ )
	return -1;

    const Interval<int> xpixrg( x_.axis_->pixRange() ),
			ypixrg( yad.axis_->pixRange() );
    uiWorld2Ui w2ui( uiSize(rgbarr_.getSize(true), rgbarr_.getSize(false)),
		     uiWorldRect((double)arrarea_.left(),(double)arrarea_.top(),
				 (double)arrarea_.right(),
				 (double)arrarea_.bottom()) );
    DensityCalc densitycalc( uidps_, data, x_, yad, selgrpset_,
			     toString(trmsg_) );
    densitycalc.setWorld2Ui( w2ui );
    densitycalc.setMathObj( mathobj_ );
    densitycalc.setModifiedColIds( modcolidxs_ );
    densitycalc.setDPSChangeable( chgdps );
    densitycalc.setRemSelected( removesel );
    densitycalc.setCurGroup( curgrp_ );
    if ( nrbins )
    {
	densitycalc.setNrBins( nrbins->start, nrbins->stop );
	densitycalc.setCellXSize( (float)arrarea_.width()/(float)nrbins->start);
	densitycalc.setCellYSize( (float)arrarea_.height()/(float)nrbins->stop);
    }
    else
	densitycalc.setCellSize( (float)cellsize_ );

    densitycalc.setAreaType( areatype );
    uiTaskRunnerProvider trprov( parent() );
    trprov.execute( densitycalc );

    usedxpixrg_ = densitycalc.usedXPixRg();
    selrowcols_ = densitycalc.selRCs();
    selyitems_ = selrowcols_.size();
    if ( freqdata )
	densitycalc.getFreqData( *freqdata );

    RangeLimitedDataDistributionExtracter<float> extr( *data, trprov );
    ctmapper_->distribution() = *extr.getDistribution();

    return densitycalc.indexSize();
}


void uiDataPointSetCrossPlotter::setDensityPlot( bool yn, bool showy2 )
{
    isdensityplot_ = yn;
    if ( y2ptitems_ && !y2ptitems_->isVisible() )
	y2ptitems_->setVisible( showy2 );
}


void uiDataPointSetCrossPlotter::drawDensityPlot( bool withremovesel )
{
    if ( !x_.axis_ || !y_.axis_ )
	return;

    uiUserShowWait usw( parent(), uiStrings::sUpdatingDisplay() );
    isdensityplot_ = true;
    drawTypeChanged.trigger( true );
    const uiAxisHandler& xah = *x_.axis_;
    const uiAxisHandler& yah = *y_.axis_;
    border_ = uiBorder( xah.pixBefore(), yah.pixAfter(),
			xah.pixAfter(), yah.pixBefore() );
    beforeDraw();
    if ( arrarea_.left()>arrarea_.right() || arrarea_.top()>arrarea_.bottom() )
	return;

    Array2D<float>* data = new Array2DImpl<float>( arrarea_.width()+ 1,
						   arrarea_.height() + 1);

    data->setAll( (float)0 );

    int indexsz = calcDensity( data, false, withremovesel, false, 0 );
    if ( indexsz < 1 )
	return;

    const Interval<float> rawrg( ctmapper_->getRange() );
    Interval<float> mapperrg =
	Interval<float>(rawrg.start<1 ? 1 : rawrg.start,
			rawrg.stop > indexsz ? indexsz : rawrg.stop );
    coltabRgChanged.trigger( mapperrg );

    rgbarr_.clear( Color::White() );

    for( int idx=0; idx<data->getSize(0); idx++ )
    {
	for( int idy=0; idy<data->getSize(1); idy++ )
	{
	    if ( !data->info().validPos(idx,idy) )
		continue;

	    const float val = data->get( idx, idy );
	    const float mappedval = ctmapper_->seqPosition( val );
	    Color col = ( val == 0 ) ? Color::White() : ctab_->color(mappedval);
	    if ( col.isVisible() )
		rgbarr_.set( idx, idy, col );
	}
    }

    PtrMan<uiPixmap> pixmap =
	new uiPixmap( arrarea_.width(), arrarea_.height() );
    pixmap->convertFromRGBArray( rgbarr_ );
    setPixmap( *pixmap );
    updatePixmap();
}


bool uiDataPointSetCrossPlotter::drawRID( uiDataPointSet::DRowID rid,
	uiGraphicsItemGroup* curitmgrp,
	const uiDataPointSetCrossPlotter::AxisData& yad, bool isy2,
	OD::MarkerStyle2D& mstyle, int itmidx, bool remsel )
{
    uiAxisHandler& xah = *x_.axis_;
    uiAxisHandler& yah = *yad.axis_;
    if ( !x_.axis_ || !yad.axis_ ) return false;

    const Interval<int> xpixrg( xah.pixRange() ), ypixrg( yah.pixRange() );
    if ( dps_.isInactive(rid) ||
	    (curgrp_>0 && dps_.group(rid)!=curgrp_) )
	return false;

    const float xval = uidps_.getVal( x_.colid_, rid, true );
    const float yval = uidps_.getVal( yad.colid_, rid, true );
    if ( mIsUdf(xval) || mIsUdf(yval) )
	return false;

    const uiPoint pt( xah.getPix(xval), yah.getPix(yval) );

    if ( !xpixrg.includes(pt.x_,true) || !ypixrg.includes(pt.y_,true) )
	return false;

    addItemIfNew( itmidx, mstyle, curitmgrp, yah, rid, isy2 );
    setItem( curitmgrp->getUiItem(itmidx), isy2, pt );
    checkSelection( rid, curitmgrp->getUiItem(itmidx), isy2, yad, remsel );

    if ( itmidx > 1 )
	usedxpixrg_.include( pt.x_ );
    else
	usedxpixrg_ = Interval<int>( pt.x_, pt.x_ );

    return true;
}


bool uiDataPointSetCrossPlotter::drawPoints( uiGraphicsItemGroup* curitmgrp,
	const uiDataPointSetCrossPlotter::AxisData& yad, bool isy2,
	OD::MarkerStyle2D& mstyle, bool removesel )
{
    uiUserShowWait usw( parent(), uiStrings::sUpdatingDisplay() );
    isdensityplot_ = false;
    drawTypeChanged.trigger( false );
    PtrMan<uiPixmap> pixmap = new uiPixmap( arrarea_.width(),arrarea_.height());
    pixmap->fill( Color::White() );
    setPixmap( *pixmap );
    updatePixmap();

    int itmidx = 0;
    usedxpixrg_ = Interval<int>(0,0);
    Array1D<char>* rowidx = isy2 ? y2rowidxs_ : yrowidxs_;
    for ( int idx=0; idx<rowidx->getSize(0); idx++ )
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


void uiDataPointSetCrossPlotter::drawData(
    const uiDataPointSetCrossPlotter::AxisData& yad, bool isy2, bool rempts )
{
    if ( !x_.axis_ || !yad.axis_ )
	return;

    uiUserShowWait usw( parent(), uiStrings::sUpdatingDisplay() );
    uiAxisHandler& yah = *yad.axis_;

    OD::MarkerStyle2D mstyle( setup_.markerstyle_ );
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
    if ( setup_.showregrline_ )
	drawRegrLine( yah, usedxpixrg_ );
    else if ( regrlineitm_ )
	regrlineitm_->setLine( 0, 0, 0, 0 );

    drawUserDefPolyLine( true );
    drawUserDefPolyLine( false );
}


void uiDataPointSetCrossPlotter::drawRegrLine( uiAxisHandler& yah,
					       const Interval<int>& xpixrg )
{
    if ( !x_.axis_ )
	return;

    const uiAxisHandler& xah = *x_.axis_;
    const LinStats2D& ls = y_.axis_ == &yah ? lsy1_ : lsy2_;
    const Interval<int> ypixrg( yah.pixRange() );
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    Interval<float> yvalrg( yah.getVal(ypixrg.start), yah.getVal(ypixrg.stop) );
    if ( !regrlineitm_ )
    {
	regrlineitm_ = new uiLineItem();
	scene().addItem( regrlineitm_ );
	regrlineitm_->setZValue( 4 );
    }

    setLine( *regrlineitm_, ls.lp, xah, yah, &xvalrg );
}
