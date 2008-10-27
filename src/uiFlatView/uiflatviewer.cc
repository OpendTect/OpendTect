/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewer.cc,v 1.62 2008-10-27 11:21:08 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiflatviewer.h"
#include "uiflatviewcontrol.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uirgbarraycanvas.h"
#include "uirgbarray.h"
#include "flatposdata.h"
#include "flatviewbitmapmgr.h"
#include "flatviewbmp2rgb.h"
#include "flatviewaxesdrawer.h"
#include "pixmap.h"
#include "datapackbase.h"
#include "bufstringset.h"
#include "draw.h"
#include "drawaxis2d.h"
#include "geometry.h"
#include "uiworld2ui.h"
#include "uimsg.h"
#include "linerectangleclipper.h"
#include "debugmasks.h"

#define mStdInitItem \
      titletxtitem_(0) \
    , axis1nm_(0) \
    , axis2nm_(0) \
    , addatanm_(0) \
    , rectitem_(0) \
    , arrowitem1_(0) \
    , arrowitem2_(0) \
    , polyitem_(0) \
    , lineitem_(0) \
    , marketitem_(0)


float uiFlatViewer::bufextendratio_ = 0.4; // 0.5 = 50% means 3 times more area

uiFlatViewer::uiFlatViewer( uiParent* p )
    : uiGroup(p,"Flat viewer")
    , mStdInitItem
    , canvas_(*new uiRGBArrayCanvas( this,uiRGBArrayCanvas::Setup(true),
				     *new uiRGBArray(false)) )
    , axesdrawer_(*new FlatView::AxesDrawer(*this,canvas_))
    , dim0extfac_(0.5)
    , wvabmpmgr_(0)
    , vdbmpmgr_(0)
    , anysetviewdone_(false)
    , extraborders_(0,0,0,0)
    , annotsz_(50,20) //TODO: should be dep on font size
    , viewChanged(this)
    , dataChanged(this)
    , dispParsChanged(this)
    , control_(0)
{
    bmp2rgb_ = new FlatView::BitMap2RGB( appearance(), canvas_.rgbArray() );
    //canvas_.newFillNeeded.notify( mCB(this,uiFlatViewer,canvasNewFill) );
    //canvas_.postDraw.notify( mCB(this,uiFlatViewer,canvasPostDraw) );
    canvas_.reSize.notify( mCB(this,uiFlatViewer,reDraw) );
    //canvas_.rubberBandUsed.notify( mCB(this,uiFlatViewer,rubberBandZoom) );
    reportedchanges_ += All;

    mainObject()->finaliseDone.notify( mCB(this,uiFlatViewer,onFinalise) );
}


uiFlatViewer::~uiFlatViewer()
{
    delete bmp2rgb_;
    delete wvabmpmgr_;
    delete vdbmpmgr_;
    delete &canvas_.rgbArray();
    delete &axesdrawer_;
}


void uiFlatViewer::rubberBandZoom( CallBacker* )
{
    const uiRect* selarea = canvas_.getSelectedArea();
    Geom::Point2D<double> centre( (double)selarea->centre().x,
	    			  (double)selarea->centre().y );
    Geom::Size2D<double> viewarea( selarea->width(), selarea->height() );
    control_->setNewView( centre, viewarea );
}


void uiFlatViewer::reDraw( CallBacker* )
{
    drawBitMaps();
    drawAnnot();
}


void uiFlatViewer::setRubberBandingOn( bool yn )
{
    canvas_.setRubberBandingOn( yn );
}


void uiFlatViewer::onFinalise( CallBacker* )
{
    canvas_.setBGColor( color(false) );
}


uiRGBArray& uiFlatViewer::rgbArray()
{
    return canvas_.rgbArray();
}


Color uiFlatViewer::color( bool foreground ) const
{
    return appearance().darkBG() == foreground ? Color::White : Color::Black;
}


void uiFlatViewer::setExtraBorders( const uiSize& lfttp, const uiSize& rghtbt )
{
    extraborders_.setLeft( lfttp.width() );
    extraborders_.setRight( rghtbt.width() );
    extraborders_.setTop( lfttp.height() );
    extraborders_.setBottom( rghtbt.height() );
}


void uiFlatViewer::setInitialSize( uiSize sz )
{
    canvas_.setPrefWidth( sz.width() );
    canvas_.setPrefHeight( sz.height() );
}


void uiFlatViewer::canvasNewFill( CallBacker* )
{
    drawBitMaps();
}


void uiFlatViewer::canvasPostDraw( CallBacker* )
{
    drawAnnot();
}


uiWorldRect uiFlatViewer::getBoundingBox( bool wva ) const
{
    const FlatDataPack* dp = pack( wva );
    if ( !dp ) dp = pack( !wva );
    if ( !dp ) return uiWorldRect(0,0,1,1);

    const FlatPosData& pd = dp->posData();
    StepInterval<double> rg0( pd.range(true) ); rg0.sort( true );
    StepInterval<double> rg1( pd.range(false) ); rg1.sort( true );
    rg0.start -= dim0extfac_ * rg0.step; rg0.stop += dim0extfac_ * rg0.step;
    return uiWorldRect( rg0.start, rg1.stop, rg0.stop, rg1.start );
}


uiWorldRect uiFlatViewer::boundingBox() const
{
    const bool wvavisible = isVisible( true );
    uiWorldRect wr1 = getBoundingBox( wvavisible );
    if ( wvavisible && isVisible(false) )
    {
	uiWorldRect wr2 = getBoundingBox( false );
	if ( wr1.left() > wr2.left() )
	    wr1.setLeft( wr2.left() );
	if ( wr1.right() < wr2.right() )
	    wr1.setRight( wr2.right() );
	if ( wr1.top() < wr2.top() )
	    wr1.setTop( wr2.top() );
	if ( wr1.bottom() > wr2.bottom() )
	    wr1.setBottom( wr2.bottom() );
    }

    return wr1;
}


void uiFlatViewer::setView( const uiWorldRect& wr )
{
    anysetviewdone_ = true;

    wr_ = wr;
    if ( (wr_.left() > wr.right()) != appearance().annot_.x1_.reversed_ )
	wr_.swapHor();
    if ( (wr_.bottom() > wr.top()) != appearance().annot_.x2_.reversed_ )
	wr_.swapVer();

    viewChanged.trigger();
}


void uiFlatViewer::handleChange( DataChangeType dct )
{
    if ( dct == None )
	return;
    else if ( dct == All )
	reset();

    reportedchanges_ += dct;

    const FlatView::Annotation& annot = appearance().annot_;
    int l = extraborders_.left(); int r = extraborders_.right();
    int t = extraborders_.top(); int b = extraborders_.bottom();
    if ( annot.haveTitle() )
	t += annotsz_.height();
    if ( annot.haveAxisAnnot(false) )
	l += annotsz_.width();
    if ( annot.haveAxisAnnot(true) )
	{ b += annotsz_.height(); t += annotsz_.height(); }

    canvas_.setBorder( uiBorder(l,t,r,b) );

}


void uiFlatViewer::reset()
{
    delete wvabmpmgr_; wvabmpmgr_ = 0;
    delete vdbmpmgr_; vdbmpmgr_ = 0;
    anysetviewdone_ = false;
}


void uiFlatViewer::drawBitMaps()
{
    canvas_.beforeDraw();
    if ( !anysetviewdone_ )
	setView( boundingBox() );

    canvas_.setBGColor( color(false) );

    bool datachgd = false;
    for ( int idx=0; idx<reportedchanges_.size(); idx++ )
    {
	DataChangeType dct = reportedchanges_[idx];
	if ( dct == All || dct == WVAData || dct == VDData )
	{ 
	    datachgd = true;
	    dispParsChanged.trigger();
	    break;
	}
    }
    reportedchanges_.erase();
    if ( datachgd )
	dataChanged.trigger();

    const bool hasdata = packID(false)!=DataPack::cNoID ||
			 packID(true)!=DataPack::cNoID;
    uiPoint offs( mUdf(int), mUdf(int) );
    if ( !wvabmpmgr_ )
    {
	delete wvabmpmgr_; delete vdbmpmgr_;
	wvabmpmgr_ = new FlatView::BitMapMgr(*this,true);
	vdbmpmgr_ = new FlatView::BitMapMgr(*this,false);
    }
    else if ( !datachgd && hasdata )
    {
	const uiRGBArray& rgbarr = canvas_.rgbArray();
	const uiSize uisz( uiSize(rgbarr.getSize(true),rgbarr.getSize(false)) );
	offs = wvabmpmgr_->dataOffs( wr_, uisz );
	if ( mIsUdf(offs.x) )
	    offs = vdbmpmgr_->dataOffs( wr_, uisz );
    }

    if ( hasdata && (datachgd || mIsUdf(offs.x)) )
    {
	wvabmpmgr_->setupChg(); vdbmpmgr_->setupChg();
	if ( !mkBitmaps(offs) )
	{
	    delete wvabmpmgr_; wvabmpmgr_ = 0; delete vdbmpmgr_; vdbmpmgr_ = 0;
	    uiMSG().error( "No memory for bitmaps" );
	    return;
	}
	if ( vdbmpmgr_->bitMapGen() )
	{
	    appearance().ddpars_.vd_.rg_ =
		vdbmpmgr_->bitMapGen()->getScaleRange();
	    dispParsChanged.trigger();
	}
	if ( wvabmpmgr_->bitMapGen() )
	    appearance().ddpars_.wva_.rg_ =
		wvabmpmgr_->bitMapGen()->getScaleRange();
    }

    if ( mIsUdf(offs.x) )
    {
	if ( hasdata )
	    ErrMsg( "Internal error during bitmap generation" );
	return;
    }

   /* if ( appearance().ddpars_.vd_.histeq_ )
	bmp2rgb_->setClipperData( vdbmpmgr_->bitMapGen()->data().
				  clipperDataPts() );*/
    bmp2rgb_->setRGBArr( canvas_.rgbArray() );
    bmp2rgb_->draw( wvabmpmgr_->bitMap(), vdbmpmgr_->bitMap(), offs );
    ioPixmap* pixmap = new ioPixmap( canvas_.arrArea().width(),
	    			     canvas_.arrArea().height() );
    pixmap->convertFromRGBArray( bmp2rgb_->rgbArray() );
    canvas_.setPixmap( *pixmap );
    canvas_.draw();
}


bool uiFlatViewer::mkBitmaps( uiPoint& offs )
{
    uiRGBArray& rgbarr = canvas_.rgbArray();
    const uiSize uisz( rgbarr.getSize(true), rgbarr.getSize(false) );
    const Geom::Size2D<double> wrsz = wr_.size();

    // Worldrect per pixel. Must be preserved.
    const double xratio = wrsz.width() / uisz.width();
    const double yratio = wrsz.height() / uisz.height();

    // Extend the viewed worldrect; 
    // Don't limit here - the generators should fill with udf if outiside
    // available range.
    uiWorldRect bufwr( wr_.grownBy(1+bufextendratio_) );

    // Calculate buffer size, snap buffer world rect
    Geom::Size2D<double> bufwrsz( bufwr.size() );
    const uiSize bufsz( (int)(bufwrsz.width() / xratio + .5),
			(int)(bufwrsz.height() / yratio + .5) );
    bufwrsz.setWidth( bufsz.width() * xratio );
    bufwrsz.setHeight( bufsz.height() * yratio );
    const bool xrev = bufwr.left() > bufwr.right();
    const bool yrev = bufwr.bottom() > bufwr.top();
    bufwr.setRight( bufwr.left() + (xrev?-1:1) * bufwrsz.width() );
    bufwr.setTop( bufwr.bottom() + (yrev?-1:1) * bufwrsz.height() );

    if ( DBG::isOn(DBG_DBG) )
	DBG::message( "Re-generating bitmaps" );
    if ( !wvabmpmgr_->generate(bufwr,bufsz)
      || !vdbmpmgr_->generate(bufwr,bufsz) )
	return false;

    offs = wvabmpmgr_->dataOffs( wr_, uisz );
    if ( mIsUdf(offs.x) )
	offs = vdbmpmgr_->dataOffs( wr_, uisz );
    return true;
}


void uiFlatViewer::drawAnnot()
{
    const FlatView::Annotation& annot = appearance().annot_;

    if ( annot.color_.isVisible() )
    {
	drawGridAnnot();
    }

    for ( int idx=0; idx<annot.auxdata_.size(); idx++ )
	drawAux( *annot.auxdata_[idx] );

    if ( !annot.title_.isEmpty() )
    {
	if ( !titletxtitem_ )
	    titletxtitem_ = canvas_.scene().addText( annot.title_ );
	else
	    titletxtitem_->setText( annot.title_ );
	titletxtitem_->setZValue(1);
	titletxtitem_->setPos( canvas_.arrArea().centre().x, 2 );
	titletxtitem_->setPenColor( color(true) );
	Alignment al( OD::AlignHCenter, OD::AlignTop );
	titletxtitem_->setAlignment( al );
    }
}


int uiFlatViewer::getAnnotChoices( BufferStringSet& bss ) const
{
    const FlatDataPack* fdp = pack( false );
    if ( !fdp ) fdp = pack( true );
    if ( fdp )
	fdp->getAltDim0Keys( bss );
    if ( !bss.isEmpty() )
	bss.addIfNew( appearance().annot_.x1_.name_ );
    if ( !mIsUdf(axesdrawer_.altdim0_ ) )
	return axesdrawer_.altdim0_;

    const int res = bss.indexOf( appearance().annot_.x1_.name_ );
    return res<0 ? mUdf(int) : res;

}


void uiFlatViewer::setAnnotChoice( int sel )
{
    BufferStringSet bss; getAnnotChoices( bss );
    if ( sel >= 0 && sel < bss.size()
      && bss.get(sel) == appearance().annot_.x1_.name_ )
	mSetUdf(sel);
    axesdrawer_.altdim0_ = sel;
}


void uiFlatViewer::getWorld2Ui( uiWorld2Ui& w2u ) const
{
    w2u.set( canvas_.arrArea(), wr_ );
}


void uiFlatViewer::drawGridAnnot()
{
    const FlatView::Annotation& annot = appearance().annot_;
    const FlatView::Annotation::AxisData& ad1 = annot.x1_;
    const FlatView::Annotation::AxisData& ad2 = annot.x2_;
    const bool showanyx1annot = ad1.showannot_ || ad1.showgridlines_;
    const bool showanyx2annot = ad2.showannot_ || ad2.showgridlines_;
    if ( !showanyx1annot && !showanyx2annot )
	return;

    const uiRect datarect( canvas_.arrArea() );
    axesdrawer_.draw( datarect, wr_ );
    if ( !rectitem_ )
	rectitem_ = canvas_.scene().addRect( datarect.left(),
						datarect.top(),
						datarect.width(),
						datarect.height() );
    else
	rectitem_->setRect( datarect.left(), datarect.top(),
			    datarect.width(), datarect.height() );
    rectitem_->setPenStyle( LineStyle(LineStyle::Solid, 3, Color::Black) );
    rectitem_->setZValue(1);

    const uiSize totsz( canvas_.width(), canvas_.height() );
    const int ynameannpos = datarect.bottom() - 2;
    ArrowStyle arrowstyle( 1 );
    arrowstyle.headstyle_.type_ = ArrowHeadStyle::Triangle;
    if ( showanyx1annot && !ad1.name_.isEmpty() )
    {
	uiPoint from( datarect.right()-12, ynameannpos + 15 );
	uiPoint to( datarect.right()-2, ynameannpos  + 15);
	if ( ad1.reversed_ ) Swap( from, to );
	if ( arrowitem1_ )
	    delete arrowitem1_;
	arrowitem1_ = canvas_.scene().addArrow( from, to, arrowstyle );
	arrowitem1_->setZValue(1);
	if ( !axis1nm_ )
	    axis1nm_ = canvas_.scene().addText( ad1.name_ );
	else
	    axis1nm_->setText( ad1.name_ );
	axis1nm_->setZValue(1);
	Alignment al( OD::AlignRight, OD::AlignVCenter );
	axis1nm_->setAlignment( al );
	axis1nm_->setPos( datarect.right() - 20, ynameannpos );
    }
    if ( showanyx2annot && !ad2.name_.isEmpty() )
    {
	const int left = datarect.left();
	uiPoint from( left , ynameannpos + 15 );
	uiPoint to( left, ynameannpos + 25 );
	if ( ad2.reversed_ ) Swap( from, to );
	if ( arrowitem2_ )
	    delete arrowitem2_;
	arrowitem2_ = canvas_.scene().addArrow(
		from, to, arrowstyle );
	arrowitem2_->setZValue(1); 
	if ( !axis2nm_ )
	    axis2nm_ = canvas_.scene().addText( ad2.name_ );
	else
	    axis2nm_->setText( ad2.name_ );
	axis2nm_->setZValue(1);
	Alignment al( OD::AlignLeft, OD::AlignVCenter );
	axis2nm_->setAlignment( al );
	axis2nm_->setPos( left+10, ynameannpos );
    }
}


void uiFlatViewer::drawAux( const FlatView::Annotation::AuxData& ad )
{
    if ( !ad.enabled_ || ad.isEmpty() ) return;

    const FlatView::Annotation& annot = appearance().annot_;
    const uiRect datarect( canvas_.arrArea() );
    uiWorldRect auxwr( wr_ );
    if ( ad.x1rg_ )
    {
	auxwr.setLeft( ad.x1rg_->start );
	auxwr.setRight( ad.x1rg_->stop );
    }

    if ( ad.x2rg_ )
    {
	auxwr.setTop( ad.x2rg_->start );
	auxwr.setBottom( ad.x2rg_->stop );
    }

    const uiWorld2Ui w2u( auxwr, canvas_.arrArea().size() );

    TypeSet<uiPoint> ptlist;
    const int nrpoints = ad.poly_.size();
    for ( int idx=0; idx<nrpoints; idx++ )
	ptlist += w2u.transform( ad.poly_[idx] ) + datarect.topLeft();

    const bool drawfill = ad.close_ && ad.fillcolor_.isVisible();
    if ( ad.linestyle_.isVisible() || drawfill )
    {

	if ( drawfill )
	{
	    if ( !polyitem_ )
		polyitem_ = canvas_.scene().addPolygon( ptlist, true );
	    else
		polyitem_->setPolygon( ptlist );
	    polyitem_->setZValue(1);
	    polyitem_->setFillColor(  ad.fillcolor_ );
	    polyitem_->setPenStyle( ad.linestyle_ );
	    //TODO clip polygon
	}
	else
	{
	    if ( ad.close_ && nrpoints>3 )
		ptlist += ptlist[0]; // close poly

	    ObjectSet<TypeSet<uiPoint> > lines;
	    clipPolyLine( datarect, ptlist, lines );

	    for ( int idx=0; idx<ptlist.size()-1; idx++ )
	    {
		if ( !lineitem_ )
		    lineitem_ = canvas_.scene().addLine( ptlist[idx],
			    				    ptlist[idx+1] );
		else
		    lineitem_->setLine( ptlist[idx].x, ptlist[idx].y,
			    		ptlist[idx+1].x, ptlist[idx+1].y );
		lineitem_->setZValue(1);
		lineitem_->setPenStyle( ad.linestyle_ );
	    }
	    //for ( int idx=lines.size()-1; idx>=0; idx-- )
		//dt.drawLine( *lines[idx], false );

	    deepErase( lines );
	}
    }

    const int nrmarkerstyles = ad.markerstyles_.size();
    if ( nrmarkerstyles )
    {
	for ( int idx=nrpoints-1; idx>=0; idx-- )
	{
	    const int styleidx = mMIN(idx,nrmarkerstyles-1);
	    if ( !ad.markerstyles_[styleidx].isVisible() ||
		 datarect.isOutside(ptlist[idx] ) )
		continue;

	    if ( !marketitem_ )
		marketitem_ =
		    canvas_.scene().addMarker( ad.markerstyles_[styleidx] );
	    else
		marketitem_->setMarkerStyle( ad.markerstyles_[styleidx] );
	    marketitem_->setZValue(1);
	    marketitem_->setPos( ptlist[idx].x, ptlist[idx].y );
	}
    }

    if ( !ad.name_.isEmpty() && !mIsUdf(ad.namepos_) )
    {
	int listpos = ad.namepos_;
	if ( listpos < 0 ) listpos=0;
	if ( listpos > nrpoints ) listpos = nrpoints-1;

	if ( !addatanm_ )
	    addatanm_ = canvas_.scene().addText( ad.name_.buf() );
	else
	    addatanm_->setText( ad.name_.buf() );
	addatanm_->setZValue(1);
	//ad.namealignment_ );
	addatanm_->setPos( ptlist[listpos].x, ptlist[listpos].y );
    }
}


Interval<float> uiFlatViewer::getDataRange( bool iswva ) const
{
    Interval<float> rg( mUdf(float), mUdf(float) );
    FlatView::BitMapMgr* mgr = iswva ? wvabmpmgr_ : vdbmpmgr_;
    if ( mgr && mgr->bitMapGen() )
	rg = mgr->bitMapGen()->data().scale( appearance().ddpars_.vd_.clipperc_
					     , mUdf(float));

    return rg;
}

