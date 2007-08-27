/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewer.cc,v 1.32 2007-08-27 11:01:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewer.h"
#include "uirgbarraycanvas.h"
#include "uirgbarray.h"
#include "flatviewbitmapmgr.h"
#include "flatviewbmp2rgb.h"
#include "flatviewaxesdrawer.h"
#include "datapackbase.h"
#include "bufstringset.h"
#include "iodrawtool.h"
#include "drawaxis2d.h"
#include "uiworld2ui.h"
#include "uimsg.h"
#include "linerectangleclipper.h"


uiFlatViewer::uiFlatViewer( uiParent* p )
    : uiGroup(p,"Flat viewer")
    , canvas_(*new uiRGBArrayCanvas(this,*new uiRGBArray))
    , axesdrawer_(*new FlatView::AxesDrawer(*this,canvas_))
    , dim0extfac_(0.5)
    , wvabmpmgr_(0)
    , vdbmpmgr_(0)
    , anysetviewdone_(false)
    , extraborders_(0,0,0,0)
    , annotsz_(50,20) //TODO: should be dep on font size
    , viewChanged(this)
    , dataChanged(this)
    , control_(0)
{
    bmp2rgb_ = new FlatView::BitMap2RGB( appearance(), canvas_.rgbArray() );
    canvas_.newFillNeeded.notify( mCB(this,uiFlatViewer,canvasNewFill) );
    canvas_.postDraw.notify( mCB(this,uiFlatViewer,canvasPostDraw) );
    reportedchanges_ += All;
}


uiFlatViewer::~uiFlatViewer()
{
    delete &canvas_.rgbArray();
    delete &canvas_;
    delete &axesdrawer_;
    delete wvabmpmgr_;
    delete vdbmpmgr_;
    delete bmp2rgb_;
}


uiRGBArray& uiFlatViewer::rgbArray()
{
    return canvas_.rgbArray();
}


Color uiFlatViewer::color( bool foreground ) const
{
    return appearance().darkBG() == foreground ? Color::White : Color::Black;
}


void uiFlatViewer::initView()
{
    setView( boundingBox() );
}


void uiFlatViewer::setDarkBG( bool yn )
{
    appearance().setDarkBG( yn );
    canvas_.setBGColor( color(false) );
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
    const FlatPosData& pd = data().pos( wva );
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


void uiFlatViewer::setView( uiWorldRect wr )
{
    anysetviewdone_ = true;
    wr_ = wr;
    if ( wr_.left() > wr.right() != appearance().annot_.x1_.reversed_ )
	wr_.swapHor();
    if ( wr_.bottom() > wr.top() != appearance().annot_.x2_.reversed_ )
	wr_.swapVer();
    handleChange( All );
    viewChanged.trigger();
}


void uiFlatViewer::handleChange( DataChangeType dct )
{
    if ( dct == None ) return;

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
    canvas_.setBorders( uiSize(l,t), uiSize(r,b) );
    canvas_.forceNewFill();
}


void uiFlatViewer::drawBitMaps()
{
    if ( !anysetviewdone_ ) initView();

    bool datachgd = false;
    for ( int idx=0; idx<reportedchanges_.size(); idx++ )
    {
	DataChangeType dct = reportedchanges_[idx];
	if ( dct == All || dct == WVAData || dct == VDData )
	    { datachgd = true; break; }
    }
    reportedchanges_.erase();

    uiRGBArray& rgbarr = canvas_.rgbArray();
    uiSize uisz( rgbarr.getSize(true), rgbarr.getSize(false) );

    bool neednewbitmaps = datachgd;
    if ( datachgd )
	dataChanged.trigger();
    else
	neednewbitmaps = prevwr_ != wr_ || prevsz_ != uisz;
    prevwr_ = wr_; prevsz_ = uisz;

    //TODO: better optimisation
    if ( neednewbitmaps )
    {
	delete wvabmpmgr_; wvabmpmgr_ = new FlatView::BitMapMgr(*this,true);
	delete vdbmpmgr_; vdbmpmgr_ = new FlatView::BitMapMgr(*this,false);

	if ( !wvabmpmgr_->generate(wr_,uisz) || !vdbmpmgr_->generate(wr_,uisz) )
	{
	    uiMSG().error( "No memory for bitmaps" );
	    delete wvabmpmgr_; wvabmpmgr_ = 0; delete vdbmpmgr_; vdbmpmgr_ = 0;
	    return;
	}
    }

    bmp2rgb_->draw( wvabmpmgr_->bitMap(), vdbmpmgr_->bitMap() );
}


void uiFlatViewer::drawAnnot()
{
    const FlatView::Annotation& annot = appearance().annot_;
    ioDrawTool& dt = canvas_.drawTool();

    if ( annot.color_.isVisible() )
    {
	dt.setLineStyle( LineStyle(LineStyle::Solid, 2, annot.color_) );
	drawGridAnnot();
    }

    for ( int idx=0; idx<annot.auxdata_.size(); idx++ )
	drawAux( *annot.auxdata_[idx] );

    if ( !annot.title_.isEmpty() )
    {
	dt.setPenColor( color(true) );
	dt.drawText( uiPoint(canvas_.arrArea().centre().x,2), annot.title_,
		     mAlign(Middle,Start) );
    }
}


int uiFlatViewer::getAnnotChoices( BufferStringSet& bss ) const
{
    const FlatDataPack* fdp = getPack( false );
    if ( !fdp ) fdp = getPack( true );
    if ( fdp )
	fdp->getAltDim0Keys( bss );
    if ( !bss.isEmpty() )
	bss.addIfNew( appearance().annot_.x1_.name_ );
    return axesdrawer_.altdim0_;
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

    ioDrawTool& dt = canvas_.drawTool();
    const uiRect datarect( canvas_.arrArea() );
    dt.drawRect( datarect );
    axesdrawer_.draw( datarect, wr_ );

    const uiSize totsz( canvas_.width(), canvas_.height() );
    const int ynameannpos = totsz.height() - 2;
    ArrowStyle arrowstyle( 1 );
    arrowstyle.headstyle_.type_ = ArrowHeadStyle::Triangle;
    if ( showanyx1annot && !ad1.name_.isEmpty() )
    {
	uiPoint from( totsz.width()-12, ynameannpos-6 );
	uiPoint to( totsz.width()-2, ynameannpos-6 );
	if ( ad1.reversed_ ) Swap( from, to );
	dt.drawArrow( from, to, arrowstyle );
	dt.drawText( uiPoint(totsz.width()-14,ynameannpos), ad1.name_,
		     mAlign(Stop,Stop) );
    }
    if ( showanyx2annot && !ad2.name_.isEmpty() )
    {
	const int left = datarect.left();
	uiPoint from( left, ynameannpos-1 );
	uiPoint to( left, ynameannpos-13 );
	if ( ad2.reversed_ ) Swap( from, to );
	dt.drawArrow( from, to, arrowstyle );
	dt.drawText( uiPoint(left+10,ynameannpos), ad2.name_,
		     mAlign(Start,Stop) );
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

    ioDrawTool& dt = canvas_.drawTool();
    TypeSet<uiPoint> ptlist;
    for ( int idx=ad.poly_.size()-1; idx>=0; idx-- )
	ptlist += w2u.transform( ad.poly_[idx] ) + datarect.topLeft();

    const bool drawfill = ad.close_ && ad.fillcolor_.isVisible();
    if ( ad.linestyle_.isVisible() || drawfill )
    {
	dt.setFillColor( ad.fillcolor_ );
	dt.setLineStyle( ad.linestyle_ );

	if ( ad.close_ )
	    //TODO clip polygon
	    dt.drawLine( ptlist, true );
	else
	{
	    ObjectSet<TypeSet<uiPoint> > lines;
	    clipPolyLine( datarect, ptlist, lines );

	    for ( int idx=lines.size()-1; idx>=0; idx-- )
		dt.drawLine( *lines[idx], false );

	    deepErase( lines );
	}
    }

    if ( ad.markerstyle_.isVisible() )
    {
	for ( int idx=ptlist.size()-1; idx>=0; idx-- )
	{
	    if ( datarect.isOutside(ptlist[idx] ) )
		continue;

	    dt.drawMarker( ptlist[idx], ad.markerstyle_ );
	}
    }

    if ( !ad.name_.isEmpty() && !mIsUdf(ad.namepos_) )
    {
	int listpos = ad.namepos_;
	if ( listpos < 0 ) listpos=0;
	if ( listpos > ptlist.size() ) listpos = ptlist.size()-1;

	dt.drawText( ptlist[listpos], ad.name_.buf(), mAlign(Middle,Middle) );
    }
}

