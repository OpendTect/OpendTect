/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiflatviewer.h"

#include "uiflatviewcontrol.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uimain.h"
#include "uimsg.h"
#include "uirgbarraycanvas.h"
#include "uirgbarray.h"
#include "uiworld2ui.h"

#include "bufstringset.h"
#include "datapackbase.h"
#include "debugmasks.h"
#include "draw.h"
#include "drawaxis2d.h"
#include "geometry.h"
#include "flatposdata.h"
#include "flatviewbitmapmgr.h"
#include "flatviewbmp2rgb.h"
#include "flatviewaxesdrawer.h"
#include "flatviewzoommgr.h"
#include "linerectangleclipper.h"
#include "mousecursor.h"
#include "pixmap.h"


#define mStdInitItem \
      titletxtitem_(0) \
    , axis1nm_(0) \
    , axis2nm_(0) \
    , rectitem_(0) \
    , arrowitem1_(0) \
    , arrowitem2_(0) \
    , pointitem_(0) \
    , polylineitemset_(0) \
    , markeritemset_(0) \
    , adnameitemset_(0) 


float uiFlatViewer::bufextendratio_ = 0.4; // 0.5 = 50% means 3 times more area

uiFlatViewer::uiFlatViewer( uiParent* p, bool enabhanddrag )
    : uiGroup(p,"Flat viewer")
    , mStdInitItem
    , canvas_(*new uiRGBArrayCanvas(this,*new uiRGBArray(false)))
    , axesdrawer_(*new FlatView::AxesDrawer(*this,canvas_))
    , dim0extfac_(0.5)
    , wvabmpmgr_(0)
    , vdbmpmgr_(0)
    , anysetviewdone_(false)
    , enabhaddrag_(enabhanddrag)
    , initview_(true)
    , extraborders_(0,0,5,0)
    , annotborder_(0,0,0,0)
    , viewborder_(0,0,0,0)
    , annotsz_(50,20) //TODO: should be dep on font size
    , viewChanging(this)
    , viewChanged(this)
    , dataChanged(this)
    , dispParsChanged(this)
    , control_(0)
    , xseldatarange_(mUdf(float),mUdf(float))		 
    , yseldatarange_(mUdf(float),mUdf(float))
    , useseldataranges_(false)	 
{
    canvas_.scene().setMouseEventActive( true );
    canvas_.setScrollBarPolicy( true, uiGraphicsViewBase::ScrollBarAlwaysOff );
    canvas_.setScrollBarPolicy( false, uiGraphicsViewBase::ScrollBarAlwaysOff );
    setStretch( 2, 2 ); canvas_.setStretch( 2, 2 );
    bmp2rgb_ = new FlatView::BitMap2RGB( appearance(), canvas_.rgbArray() );
    canvas_.reSize.notify( mCB(this,uiFlatViewer,reSizeDraw) );
    reportedchanges_ += All;

    postFinalise().notify( mCB(this,uiFlatViewer,onFinalise) );
}


#define mRemoveAnnotItem( item ) { delete item; item = 0; }

uiFlatViewer::~uiFlatViewer()
{
    if ( polylineitemset_ )
	canvas_.scene().removeItems( *polylineitemset_ );

    if ( markeritemset_ )
	canvas_.scene().removeItems( *markeritemset_ );

    delete polylineitemset_; delete markeritemset_;
    delete bmp2rgb_;
    delete wvabmpmgr_;
    delete vdbmpmgr_;
    delete &axesdrawer_;
    delete &canvas_.rgbArray();

    mRemoveAnnotItem( rectitem_ )
    mRemoveAnnotItem( arrowitem1_ )
    mRemoveAnnotItem( axis1nm_ )
    mRemoveAnnotItem( arrowitem2_ )
    mRemoveAnnotItem( axis2nm_ )
}


void uiFlatViewer::reDrawAll()
{
    drawBitMaps();
    drawAnnot();
}


void uiFlatViewer::reSizeDraw( CallBacker* cb )
{
    anysetviewdone_ = true;
    if ( !initview_ && enabhaddrag_ && 
	    ( control() && !control()->zoomMgr().atStart() ) )
    {
	uiSize newsize( canvas_.width(), canvas_.height() );
	mCBCapsuleUnpack(uiSize,oldsize,cb);
	const float widthfac = (float)newsize.width()/(float)oldsize.width();
	const float heightfac = (float)newsize.height()/(float)oldsize.height();
	uiRect prevscenerect = canvas_.getSceneRect();
	uiRect scenerect( uiPoint(0,0),
		uiSize(mNINT32((canvas_.getSceneRect().width()-5)*widthfac),
		       mNINT32((canvas_.getSceneRect().height()-5)*heightfac)) );
	canvas_.setSceneRect( scenerect ); 
	uiRect viewrect = canvas_.getViewArea();

	viewborder_ = uiBorder( viewrect.left() - scenerect.left(),
			        viewrect.top() - scenerect.top(),
			        scenerect.right() - viewrect.right(),
			        scenerect.bottom() - viewrect.bottom() );
	uiBorder actborder = viewborder_;
	actborder += annotborder_;
	canvas_.setBorder( actborder );
    }

    drawBitMaps();
    drawAnnot();
    initview_ = false;
}


void uiFlatViewer::setRubberBandingOn( bool yn )
{
    canvas_.setDragMode( yn ? uiGraphicsView::RubberBandDrag
	   		    : uiGraphicsView::NoDrag );
    canvas_.scene().setMouseEventActive( true );
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
    return appearance().darkBG() == foreground ? Color::White() : Color::Black();
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
    setPrefWidth( sz.width() ); setPrefHeight( sz.height() );
    canvas_.setPrefWidth( sz.width() ); canvas_.setPrefHeight( sz.height() );
}


uiWorldRect uiFlatViewer::getBoundingBox( bool wva ) const
{
    const FlatDataPack* dp = pack( wva );
    if ( !dp ) dp = pack( !wva );
    if ( !dp ) return uiWorldRect(0,0,1,1);

    const FlatPosData& pd = dp->posData();
    StepInterval<double> rg0( pd.range(true) ); 
    StepInterval<double> rg1( pd.range(false) );
    if (useseldataranges_ && !xseldatarange_.isUdf() && !yseldatarange_.isUdf())
    { 
	rg0.limitTo( xseldatarange_ ); 
	rg1.limitTo( yseldatarange_ );
    }
    rg0.sort( true ); 
    rg1.sort( true );

    rg0.start -= dim0extfac_ * rg0.step; rg0.stop += dim0extfac_ * rg0.step;
    if ( !wva )
	rg1.start -= dim0extfac_ * rg1.step; rg1.stop += dim0extfac_ * rg1.step;
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


uiBorder uiFlatViewer::getActBorder( const uiWorldRect& wr )
{
    uiWorldRect br = boundingBox();
    if ( (br.top()>br.bottom() && wr.top()<wr.bottom()) ||
	 (br.top()<br.bottom() && wr.top()>wr.bottom()) )
	br.swapVer();
    if ( (br.left()>br.right() && wr.left()<wr.right()) ||
	 (br.left()<br.right() && wr.left()>wr.right()) )
	br.swapHor();
    
    uiWorldRect prevwr = curView();
    const float widthfac = (float)prevwr.width() / (float)wr.width();
    const float heightfac = (float)prevwr.height() / (float)wr.height();
    uiRect scenerect;
    if ( !mIsEqual(widthfac,1,mDefEps) || !mIsEqual(heightfac,1,mDefEps) )
    {
	uiRect prevscrect = canvas_.getSceneRect();
	scenerect = uiRect( uiPoint(0,0),
	uiSize(mNINT32(prevscrect.width()*widthfac),
	mNINT32(prevscrect.height()*heightfac)) );
	canvas_.setSceneRect( scenerect );
    }
    else
	scenerect = canvas_.getSceneRect();

    const uiWorld2Ui w2u( scenerect.size(), br );
    uiPoint lefttop = w2u.transform( wr.topLeft() );
    uiPoint rightbottom = w2u.transform( wr.bottomRight() );
    uiPoint brdrlefttop = lefttop - scenerect.topLeft();
    uiPoint brdrrightbottom = scenerect.bottomRight() - rightbottom;
    uiBorder actborder( brdrlefttop.x,brdrlefttop.y,
    brdrrightbottom.x, brdrrightbottom.y);
    viewborder_ = actborder;
    actborder += annotborder_;
    return actborder;
}


void uiFlatViewer::setView( const uiWorldRect& wr )
{
    uiRect viewarea;
    uiWorldRect br = boundingBox();
    if ( enabhaddrag_ && anysetviewdone_ )
    {
	uiBorder actborder = getActBorder( wr );
	viewarea = viewborder_.getRect( canvas_.getSceneRect().size() );
	canvas_.setBorder( actborder );
    }

    if ( wr.topLeft() == wr.bottomRight() )
	return;
    wr_ = wr;
    if ( (wr_.left() > wr.right()) != appearance().annot_.x1_.reversed_ )
	wr_.swapHor();
    if ( (wr_.bottom() > wr.top()) != appearance().annot_.x2_.reversed_ )
	wr_.swapVer();

    if ( anysetviewdone_ )
    {
	reportedchanges_.erase();
	reportedchanges_ += FlatView::Viewer::WVAPars;
	reportedchanges_ += FlatView::Viewer::VDPars;
    }

    anysetviewdone_ = true;
    if ( drawBitMaps() )
    {
	if ( enabhaddrag_ )
	    canvas_.centreOn( viewarea.centre() );
	drawAnnot();
	viewChanged.trigger();
    }
}


void uiFlatViewer::handleChange( DataChangeType dct, bool dofill )
{
    if ( dct == None )
	return;
    else if ( dct == All )
	reset();

    reportedchanges_ += dct;

    const FlatView::Annotation& annot = appearance().annot_;
    int l = extraborders_.left(); int r = extraborders_.right() + 2;
    int t = extraborders_.top(); int b = extraborders_.bottom();
    if ( annot.haveTitle() )
	t += annotsz_.height();
    if ( annot.haveAxisAnnot(false) )
	l += annotsz_.width();
    if ( annot.haveAxisAnnot(true) )
	{ b += annotsz_.height(); t += annotsz_.height(); }

    annotborder_ =  uiBorder(l,t,r,b);
    uiBorder actborder = enabhaddrag_ ? viewborder_ : uiBorder(0,0,0,0);
    actborder += annotborder_;
    canvas_.setBorder( actborder );
    if ( dofill )
    {
	if ( dct == Annot )
	    drawAnnot();
	else
	    reDrawAll();
    }
}


void uiFlatViewer::reset()
{
    delete wvabmpmgr_; wvabmpmgr_ = 0;
    delete vdbmpmgr_; vdbmpmgr_ = 0;
    if ( enabhaddrag_ )
    {
	uiRect scenerect( 0, 0, canvas_.width(), canvas_.height() );
	canvas_.setSceneRect( scenerect );
    }
    viewborder_ = uiBorder( 0, 0, 0 ,0 );
    anysetviewdone_ = false;
}


bool uiFlatViewer::drawBitMaps()
{
    if ( !isMainThreadCurrent() )
	return false;

    if ( enabhaddrag_ )
    {
	if ( initview_ || ( control() && control()->zoomMgr().atStart() ) )
	{
	    uiRect scenerect( uiPoint(0,0), uiSize(mNINT32(canvas_.width()),
						   mNINT32(canvas_.height())) );
	    canvas_.setSceneRect( scenerect );
	}

	uiRect scenerect =  rgbCanvas().getSceneRect();
	canvas_.beforeDraw( scenerect.width(), scenerect.height() );
    }
    else
	canvas_.beforeDraw();
    if ( !anysetviewdone_ )
	setView( boundingBox() );

    canvas_.setBGColor( color(false) );

    bool datachgd = false;
    bool parschanged = false;
    bool annotchanged = false;
    for ( int idx=0; idx<reportedchanges_.size(); idx++ )
    {
	DataChangeType dct = reportedchanges_[idx];
	if ( dct == All || dct == WVAData || dct == VDData )
	{ 
	    datachgd = true;
	    break;
	}
	else if ( dct == WVAPars || dct == VDPars )
	    parschanged = true;
	else if ( dct == Annot )
	    annotchanged = true;
    }
    reportedchanges_.erase();

    if ( !datachgd && !parschanged && annotchanged )
	return false;


    const bool hasdata = packID(false)!=DataPack::cNoID() ||
			 packID(true)!=DataPack::cNoID();
    
    if ( datachgd && hasdata )
	dataChanged.trigger();
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

    if ( (hasdata && (datachgd || mIsUdf(offs.x))) || parschanged  )
    {
	MouseCursorChanger cursorchgr( MouseCursor::Wait );
	wvabmpmgr_->setupChg(); vdbmpmgr_->setupChg();
	if ( !mkBitmaps(offs) )
	{
	    delete wvabmpmgr_; wvabmpmgr_ = 0; delete vdbmpmgr_; vdbmpmgr_ = 0;
	    return false;
	}
	if ( vdbmpmgr_->bitMapGen() )
	{
	    if ( datachgd )
		appearance().ddpars_.vd_.mappersetup_.range_ =
		    vdbmpmgr_->bitMapGen()->getScaleRange();
	    dispParsChanged.trigger();
	}
	if ( wvabmpmgr_->bitMapGen() && datachgd )
	    appearance().ddpars_.wva_.mappersetup_.range_ =
		wvabmpmgr_->bitMapGen()->getScaleRange();
    }

    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    PtrMan<ioPixmap> pixmap = new ioPixmap( canvas_.arrArea().width(),
	    				    canvas_.arrArea().height() );
    if ( !hasdata || mIsUdf(offs.x) )
	pixmap->fill( color(false) );
    else
    {
	bmp2rgb_->setRGBArr( canvas_.rgbArray() );
	bmp2rgb_->draw( wvabmpmgr_->bitMap(), vdbmpmgr_->bitMap(), offs );
	pixmap->convertFromRGBArray( bmp2rgb_->rgbArray() );
    }

    canvas_.setPixmap( *pixmap );
    canvas_.draw();
    return true;
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
    if ( !wvabmpmgr_->generate(bufwr,bufsz,uisz)
      || !vdbmpmgr_->generate(bufwr,bufsz,uisz) )
	return false;

    offs = wvabmpmgr_->dataOffs( wr_, uisz );
    if ( mIsUdf(offs.x) )
	offs = vdbmpmgr_->dataOffs( wr_, uisz );
    return true;
}


bool uiFlatViewer::drawAnnot()
{ return drawAnnot( canvas_.arrArea(), wr_ ); }


bool uiFlatViewer::drawAnnot( const uiRect& drawarea, const uiWorldRect& wr )
{
    if ( !isMainThreadCurrent() )
	return false;

    if ( (mainwin() && !mainwin()->finalised()) )
	return false;

    for ( int idx=0; idx<reportedchanges_.size(); idx++ )
    {
	if (reportedchanges_[idx] == Annot )
	{
	    reportedchanges_.remove(idx);
	    idx--;
	}
    }

    BufferStringSet bss; getAnnotChoices( bss );

    if ( bss.indexOf(appearance().annot_.x1_.name_) >= 0 )
	axesdrawer_.altdim0_ = bss.indexOf( appearance().annot_.x1_.name_ );

    FlatView::Annotation& annot = appearance().annot_;

    drawGridAnnot( annot.color_.isVisible(), drawarea, wr );

    if ( polylineitemset_ )
    {
	canvas_.scene().removeItems( *polylineitemset_ );
	while ( polylineitemset_->size()>500 ) // Hack! >500 items unlikely
	    polylineitemset_->remove( 0, false );
    }

    if ( markeritemset_ )
    {
	canvas_.scene().removeItems( *markeritemset_ );
	while ( markeritemset_->size()>500 ) // Hack! >500 items unlikely
	    markeritemset_->remove( 0, false );
    }

    if ( adnameitemset_ )
    {
	canvas_.scene().removeItems( *adnameitemset_ );
	while ( adnameitemset_->size()>500 ) // Hack! >500 items unlikely
	    adnameitemset_->remove( 0, false );
    }

    if ( pointitem_ )
    {
	canvas_.scene().removeItem( pointitem_ );
	delete pointitem_;
	pointitem_ = 0;
    }
    for ( int idx=0; idx<annot.auxdata_.size(); idx++ )
	drawAux( *annot.auxdata_[idx], drawarea, wr );

    if ( annot.title_.isEmpty() )
	mRemoveAnnotItem( titletxtitem_ )
    else
    {
	mDeclAlignment( al, HCenter, Top );
	if ( !titletxtitem_ )
	{
	    titletxtitem_ =
		canvas_.scene().addItem( new uiTextItem(annot.title_,al) );
	    titletxtitem_->setZValue( 1 );
	    titletxtitem_->setPenColor( color(true) );
	}
	else
	    titletxtitem_->setText( annot.title_ );
	titletxtitem_->setPos( uiPoint(drawarea.centre().x,drawarea.top()-35) );
    }
    return true;
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
    if ( mIsUdf(sel) || bss.isEmpty() || ( !bss.validIdx(sel) ||
	 bss.get(sel) == appearance().annot_.x1_.name_) )
	return;

    if ( !mIsUdf(sel) )
	appearance().annot_.x1_.name_ = bss.get( sel );
    axesdrawer_.altdim0_ = sel;
}


void uiFlatViewer::getWorld2Ui( uiWorld2Ui& w2u ) const
{
    w2u.set( canvas_.arrArea(), wr_ );
}


void uiFlatViewer::drawGridAnnot( bool isvisble, const uiRect& drawarea,
				  const uiWorldRect& wr )
{
    if ( rectitem_ )
	rectitem_->setVisible( isvisble );
    if ( arrowitem1_ )
	arrowitem1_->setVisible( isvisble );
    if ( axis1nm_ )
	axis1nm_->setVisible( isvisble );
    if ( arrowitem2_ )
	arrowitem2_->setVisible( isvisble );
    if ( axis2nm_ )
	axis2nm_->setVisible( isvisble );
    if ( !isvisble )
	return;

    const FlatView::Annotation& annot = appearance().annot_;
    const FlatView::Annotation::AxisData& ad1 = annot.x1_;
    const FlatView::Annotation::AxisData& ad2 = annot.x2_;
    const bool showanyx1annot = ad1.showannot_ || ad1.showgridlines_;
    const bool showanyx2annot = ad2.showannot_ || ad2.showgridlines_;
    
    const uiRect datarect( drawarea );
    LineStyle ls( LineStyle::Solid, 1, annot.color_ );
    axesdrawer_.setXLineStyle( ls );
    axesdrawer_.setYLineStyle( ls );
    axesdrawer_.setGridLineStyle( ls );
    axesdrawer_.draw( datarect, wr );
   
    if ( (!showanyx1annot && !showanyx2annot) )
    {
	mRemoveAnnotItem( rectitem_ )
	mRemoveAnnotItem( arrowitem1_ )
	mRemoveAnnotItem( axis1nm_ )
	mRemoveAnnotItem( arrowitem2_ )
	mRemoveAnnotItem( axis2nm_ )
	return;
    }

    if ( !rectitem_ )
	rectitem_ = canvas_.scene().addRect( datarect.left(),
					     datarect.top(),
					     datarect.width(),
					     datarect.height() );
    else
	rectitem_->setRect( datarect.left(), datarect.top(),
			    datarect.width(), datarect.height() );
    rectitem_->setPenStyle( LineStyle(LineStyle::Solid, 3, annot.color_) );
    rectitem_->setZValue(1);

    const uiSize totsz( canvas_.width(), canvas_.height() );
    const int ynameannpos = datarect.bottom() - 2;
    ArrowStyle arrowstyle( 1 );
    arrowstyle.headstyle_.type_ = ArrowHeadStyle::Triangle;
    if ( showanyx1annot && !ad1.name_.isEmpty() && ad1.name_ != " " )
    {
	uiPoint from( datarect.right()-12, ynameannpos + 15 );
	uiPoint to( datarect.right()-2, ynameannpos  + 15);
	if ( ad1.reversed_ ) Swap( from, to );
	if ( !arrowitem1_ )
	{
	    arrowitem1_ = canvas_.scene().addItem(
		    new uiArrowItem(from,to,arrowstyle) );
	    arrowitem1_->setZValue( 1 );
	}
	arrowitem1_->setPenColor( annot.color_ );
	arrowitem1_->setTailHeadPos( from, to );

	if ( !axis1nm_ )
	{
	    axis1nm_ = canvas_.scene().addItem(
		    new uiTextItem(ad1.name_,mAlignment(Right,Top)) );
	    axis1nm_->setZValue(1);
	}
	else
	    axis1nm_->setText( ad1.name_ );

	axis1nm_->setTextColor( annot.color_ );
	axis1nm_->setPos( uiPoint(datarect.right()-20,ynameannpos) );
    }

    if ( showanyx2annot && !ad2.name_.isEmpty() && ad2.name_ != " " )
    {
	const int left = datarect.left();
	uiPoint from( left , ynameannpos + 15 );
	uiPoint to( left, ynameannpos + 25 );
	if ( ad2.reversed_ ) Swap( from, to );
	if ( !arrowitem2_ )
	{
	    arrowitem2_ = canvas_.scene().addItem(
		    new uiArrowItem(from,to,arrowstyle) );
	    arrowitem2_->setZValue( 1 ); 
	}
	arrowitem2_->setPenColor( annot.color_ );
	arrowitem2_->setTailHeadPos( from, to );

	if ( !axis2nm_ )
	{
	    axis2nm_ = canvas_.scene().addItem(
		    new uiTextItem(ad2.name_,mAlignment(Left,Top)) );
	    axis2nm_->setZValue( 1 );
	}
	else
	    axis2nm_->setText( ad2.name_ );
	axis2nm_->setTextColor( annot.color_ );
	axis2nm_->setPos( uiPoint(left+10,ynameannpos) );
    }
}


void uiFlatViewer::drawAux( FlatView::Annotation::AuxData& ad,
			    const uiRect& drawarea, const uiWorldRect& wr )
{
    if ( !ad.enabled_ || ad.isEmpty() ) return;

    ad.dispids_.erase();

    const FlatView::Annotation& annot = appearance().annot_;
    const uiRect datarect( drawarea );
    uiWorldRect auxwr( wr );
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
    if ( (ad.linestyle_.isVisible() || drawfill) && ptlist.size()>1 )
    {
	if ( pointitem_ )
	{
	    canvas_.scene().removeItem( pointitem_ );
	    delete pointitem_;
	    pointitem_ = 0;
	}

	if ( !polylineitemset_ )
	    polylineitemset_ = new uiGraphicsItemSet();

	if ( drawfill )
	{
	    uiPolygonItem* polyitem = 
		canvas_.scene().addPolygon( ptlist, true );
	    polyitem->setZValue( ad.zvalue_ );
	    polyitem->setFillColor( ad.fillcolor_, true );
	    polyitem->fill();
	    polyitem->setPenStyle( ad.linestyle_ );
	    polylineitemset_->add( polyitem );
	    ad.dispids_ += polyitem->id();
	    polyitem->setVisible( ad.displayed_ );
	    //TODO clip polygon
	}
	else
	{
	    if ( ad.close_ && nrpoints>3 )
		ptlist += ptlist[0]; // close poly

	    ObjectSet<TypeSet<uiPoint> > lines;
	    clipPolyLine( datarect, ptlist, lines );

	    for ( int idx=lines.size()-1; idx>=0; idx-- )
	    {
		uiPolyLineItem* polyitem =
		    canvas_.scene().addItem( new uiPolyLineItem() );
		polyitem->setPolyLine( *lines[idx] );
		polyitem->setPenStyle( ad.linestyle_ );
		polyitem->setZValue(ad.zvalue_);
		polylineitemset_->add( polyitem );
		ad.dispids_ += polyitem->id();
		polyitem->setVisible( ad.displayed_ );
	    }
	    
	    deepErase( lines );
	}
    }
    else if ( (ptlist.size()==1) && (ad.markerstyles_.size()==0) )
    {
	const Color usecol = color( true );
	if ( !pointitem_ )
	{
	    pointitem_ = new uiMarkerItem(
		    ptlist[0], MarkerStyle2D(MarkerStyle2D::Square,4,usecol) );
	    canvas_.scene().addItem( pointitem_ );
	    ad.dispids_ += pointitem_->id();
	    pointitem_->setVisible( ad.displayed_ );
	}
	else
	    pointitem_->setPos( ptlist[0] );
	pointitem_->setPenColor( usecol );
	pointitem_->setZValue( 2 );
    }

    const int nrmarkerstyles = ad.markerstyles_.size();
    if ( nrmarkerstyles > 0 )
    {
	if ( !markeritemset_ )
	    markeritemset_ = new uiGraphicsItemSet();

	for ( int idx=nrpoints-1; idx>=0; idx-- )
	{
	    const int styleidx = mMIN(idx,nrmarkerstyles-1);
	    if ( !ad.markerstyles_[styleidx].isVisible() ||
		 datarect.isOutside(ptlist[idx] ) )
		continue;
	    uiMarkerItem* markeritem = canvas_.scene().addItem(
		new uiMarkerItem( ad.markerstyles_[styleidx] ) );
	    markeritem->rotate( ad.markerstyles_[styleidx].rotation_ );
	    markeritem->setPenColor( ad.markerstyles_[styleidx].color_ );
	    markeritem->setFillColor( ad.markerstyles_[styleidx].color_ );
	    markeritem->setPos( ptlist[idx] );
	    markeritem->setZValue( 2 );
	    markeritemset_->add( markeritem );
	    ad.dispids_ += markeritem->id();
	    markeritem->setVisible( ad.displayed_ );
	}
    }

    if ( !ad.name_.isEmpty() && !mIsUdf(ad.namepos_) )
    {
	if ( !adnameitemset_ )
	    adnameitemset_ = new uiGraphicsItemSet();

	int listpos = ad.namepos_;
	if ( listpos < 0 ) listpos=0;
	if ( listpos > nrpoints ) listpos = nrpoints-1;

	uiTextItem* adnm = canvas_.scene().addItem(
		new uiTextItem(ad.name_,ad.namealignment_) );
	adnm->setZValue( 1 );
	adnm->setTextColor( ad.linestyle_.color_ );
	adnm->setPos( ptlist[listpos] );
	adnameitemset_->add( adnm );
	ad.dispids_ += adnm->id();
	adnm->setVisible( ad.displayed_ );
    }
}


void uiFlatViewer::showAuxDataObjects( FlatView::Annotation::AuxData& ad,
				       bool yn )
{
    for ( int idx=0; idx<ad.dispids_.size(); idx++ )
    {
	uiGraphicsItem* itm = canvas_.scene().getItem( ad.dispids_[idx] );
	if ( itm )
	    itm->setVisible( yn );
	ad.displayed_ = yn;
    }
}


void uiFlatViewer::updateProperties( const FlatView::Annotation::AuxData& ad )
{
    for ( int idx=0; idx<ad.dispids_.size(); idx++ )
    {
	uiGraphicsItem* itm = canvas_.scene().getItem( ad.dispids_[idx] );
	if ( !itm ) continue;

	mDynamicCastGet(uiPolygonItem*,polygon,itm);
	if ( polygon )
	{
	    polygon->setFillColor( ad.fillcolor_, true );
	    polygon->setPenStyle( ad.linestyle_ );
	    continue;
	}

	mDynamicCastGet(uiPolyLineItem*,polyln,itm);
	if ( polyln )
	{
	    polyln->setPenStyle( ad.linestyle_ );
	    continue;
	}

	mDynamicCastGet(uiMarkerItem*,marker,itm);
	{
	    if ( !ad.markerstyles_.size() ) continue;

	    marker->setPenColor( ad.markerstyles_[0].color_ );
	    marker->setFillColor( ad.markerstyles_[0].color_ );
	    continue;
	}

	mDynamicCastGet(uiTextItem*,adnm,itm);
	{
	    adnm->setTextColor( ad.linestyle_.color_ );
	}
    }
}


void uiFlatViewer::remove( const FlatView::Annotation::AuxData& ad )
{
    for ( int idx=0; idx<ad.dispids_.size(); idx++ )
    {
	uiGraphicsItem* itm = canvas_.scene().getItem( ad.dispids_[idx] );
	if ( !itm ) continue;

	mDynamicCastGet(uiPolygonItem*,polygon,itm);
	if ( polygon )
	{
	    (*polylineitemset_) -= canvas_.scene().removeItem( itm );
	    continue;
	}

	mDynamicCastGet(uiPolyLineItem*,polyln,itm);
	if ( polyln )
	{
	    (*polylineitemset_) -= canvas_.scene().removeItem( itm );
	    continue;
	}

	mDynamicCastGet(uiMarkerItem*,marker,itm);
	if ( marker )
	{
	    (*markeritemset_) -= canvas_.scene().removeItem( itm );
	    continue;
	}

	mDynamicCastGet(uiTextItem*,adnm,itm);
	if ( adnm )
	{
	    (*adnameitemset_) -= canvas_.scene().removeItem( itm );
	    continue;
	}
    }
}


void uiFlatViewer::reGenerate( FlatView::Annotation::AuxData& ad )
{
    remove( ad );
    drawAux( ad, canvas_.arrArea(), wr_ );
}


Interval<float> uiFlatViewer::getDataRange( bool iswva ) const
{
    Interval<float> rg( mUdf(float), mUdf(float) );
    const ColTab::MapperSetup mapper =
	iswva ? appearance().ddpars_.wva_.mappersetup_
	      : appearance().ddpars_.vd_.mappersetup_;
    Interval<float> mapperrange = mapper.range_;
    if ( mapper.type_ == ColTab::MapperSetup::Fixed )
	return mapperrange;

    FlatView::BitMapMgr* mgr = iswva ? wvabmpmgr_ : vdbmpmgr_;
    if ( mgr && mgr->bitMapGen() )
	rg = mgr->bitMapGen()->data().scale( mapper.cliprate_, mUdf(float));

    return rg;
}


void uiFlatViewer::setSelDataRanges( Interval<double> xrg,Interval<double> yrg)
{
    useseldataranges_ = true;
    xseldatarange_ = xrg;
    yseldatarange_ = yrg;
    viewChanged.trigger();
}


void uiFlatViewer::disableReSizeDrawNotifier()
{
    canvas_.reSize.remove( mCB(this,uiFlatViewer,reSizeDraw) );
}
