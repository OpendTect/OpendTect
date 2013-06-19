/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiflatviewer.h"

#include "uiflatauxdatadisplay.h"
#include "uiflatbitmapdisplay.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uiworld2ui.h"

#include "bufstringset.h"
#include "drawaxis2d.h"
#include "flatposdata.h"
#include "flatviewaxesdrawer.h"
#include "threadwork.h"
#include "mousecursor.h"

#define mBitMapZ	0
#define mAuxDataZ	100
#define mAxisZStart	200
#define mAnnotZ		300

uiFlatViewer::uiFlatViewer( uiParent* p )
    : uiGroup(p,"Flat viewer")
    , view_( new uiGraphicsView( this, "Flatview" ) )
    , axesdrawer_(*new FlatView::AxesDrawer(*this,*view_))
    , extfac_(0.5f)
    , extraborders_(0,0,5,0)
    , worldgroup_( new uiGraphicsItemGroup( true ) )
    , bitmapdisp_( new FlatView::uiBitMapDisplay( *this ) )
    , annotwork_( mCB(this,uiFlatViewer,updateAnnotCB) )
    , auxdatawork_( mCB(this,uiFlatViewer,updateAuxDataCB) )
    , bitmapwork_( mCB(this,uiFlatViewer,updateBitmapCB) )
    , control_(0)
    , xseldatarange_(mUdf(float),mUdf(float))		 
    , yseldatarange_(mUdf(float),mUdf(float))
    , useseldataranges_(false)	 
    , viewChanged(this)
    , dataChanged(this)
    , dispParsChanged(this)
{
    updatequeueid_ =
	Threads::WorkManager::twm().addQueue( Threads::WorkManager::Manual );

    view_->preDraw.notify( mCB(this,uiFlatViewer,updateCB ) );
    view_->disableScrollZoom();
    view_->scene().setMouseEventActive( true );
    view_->scene().addItem( worldgroup_ );
    view_->setScrollBarPolicy( false, uiGraphicsViewBase::ScrollBarAlwaysOff );
    view_->setScrollBarPolicy( true, uiGraphicsViewBase::ScrollBarAlwaysOff );
    view_->reSize.notify( mCB(this,uiFlatViewer,reSizeCB) );
    setStretch( 2, 2 ); view_->setStretch( 2, 2 );

    reportedchanges_ += All;
    bitmapdisp_->getDisplay()->setZValue( mBitMapZ );
    bitmapdisp_->setExtraFactor( extfac_ );
    worldgroup_->add( bitmapdisp_->getDisplay() );
    axesdrawer_.setZvalue( mAxisZStart );
    mAttachCB( axesdrawer_.layoutChanged(), uiFlatViewer, reSizeCB );
}


uiFlatViewer::~uiFlatViewer()
{
    Threads::WorkManager::twm().removeQueue( updatequeueid_, false );

    delete &axesdrawer_;

    bitmapdisp_->removeDisplay();
    delete bitmapdisp_;

    delete view_->scene().removeItem( worldgroup_ );

    deepErase( auxdata_ );
}


void uiFlatViewer::reSizeCB( CallBacker* cb )
{
    axesdrawer_.setViewRect( getViewRect() );
    bitmapdisp_->setViewRect( getViewRect() );
    updateTransforms();
}


uiRect uiFlatViewer::getViewRect() const
{
    const FlatView::Annotation& annot = appearance().annot_;
    int l = extraborders_.left();
    int r = extraborders_.right();
    int t = extraborders_.top();
    int b = extraborders_.bottom();

    const int axisheight = axesdrawer_.getNeededHeight();
    const int axiswidth = axesdrawer_.getNeededWidth();
    
    if ( annot.haveTitle() ) t += axisheight;
    if ( annot.haveAxisAnnot(false) ) 
	{ l += axiswidth; r += 2; }
    if ( annot.haveAxisAnnot(true) )
	{ b += axisheight;  t += axisheight; }
    
    const uiBorder annotborder(l,t,r,b);

    return annotborder.getRect( view_->getSceneRect() );
}


void uiFlatViewer::updateAuxDataCB( CallBacker* )
{
    for ( int idx=0; idx<auxdata_.size(); idx++ )
    {
	auxdata_[idx]->touch();
    }
}


void uiFlatViewer::updateAnnotCB( CallBacker* )
{
    axesdrawer_.update();
    reSizeCB(0); // Needed as annotation changes may make view-area
    		 // larger or smaller.
}


void uiFlatViewer::updateTransforms() 
{
    const uiRect viewrect = getViewRect();

    uiWorldRect wr = wr_;
    if ( wr.left() > wr.right() ) 
	wr.swapHor();
    if ( wr.bottom() < wr.top() ) 
	wr.swapVer();

    if ( mIsZero(wr_.width(),mDefEps) || mIsZero(wr_.height(),mDefEps) )
        return;

    const double xscale = viewrect.width()/wr.width();
    const double yscale = viewrect.height()/wr.height();
    const double xpos = viewrect.left()-xscale*wr.left();
    const double ypos = viewrect.top()-yscale*wr.top();

    worldgroup_->setPos( uiWorldPoint( xpos, ypos ) );
    worldgroup_->setScale( (float) xscale, (float) yscale );
}


void uiFlatViewer::setRubberBandingOn( bool yn )
{
    view_->setDragMode( yn ? uiGraphicsView::RubberBandDrag
	   		    : uiGraphicsView::NoDrag );
    view_->scene().setMouseEventActive( true );
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
    view_->setPrefWidth( sz.width() ); view_->setPrefHeight( sz.height() );
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

    rg0.widen( extfac_ * rg0.step, true );
    if ( !wva && !isVisible(wva) )
	rg1.widen( extfac_ * rg1.step, true );
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
    if ( wr.topLeft() == wr.bottomRight() )
	return;

    wr_ = wr;
 	   if ( (wr_.left() > wr.right()) != appearance().annot_.x1_.reversed_ )
	wr_.swapHor();
    if ( (wr_.bottom() > wr.top()) != appearance().annot_.x2_.reversed_ )
	wr_.swapVer();

    axesdrawer_.setWorldCoords( wr_ );
    updateTransforms();

    viewChanged.trigger();
}


void uiFlatViewer::setViewToBoundingBox()
{
    setView( pack(true) ? getBoundingBox(true) : getBoundingBox(false) );
}



#define mAddToQueue( work ) \
    Threads::WorkManager::twm().addWork( work, 0, updatequeueid_, false, true )

void uiFlatViewer::handleChange( DataChangeType dct, bool dofill )
{
    if ( dct==Auxdata || dct==All )
	mAddToQueue( auxdatawork_ );

    if ( dct==Annot || dct==All )
	mAddToQueue( annotwork_ );

    if ( dct!=Annot && dct!=Auxdata )
	mAddToQueue( bitmapwork_ );

    view_->rePaint();
}


void uiFlatViewer::updateCB( CallBacker* )
{
    Threads::WorkManager::twm().executeQueue( updatequeueid_ );
}


void uiFlatViewer::updateBitmapCB( CallBacker* )
{
    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    dataChanged.trigger();
    bitmapdisp_->update();
    dispParsChanged.trigger();
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
    w2u.set( getViewRect(), wr_ );
}


void uiFlatViewer::reGenerate( FlatView::AuxData& ad )
{
    mDynamicCastGet( FlatView::uiAuxDataDisplay*, uiad, &ad );
    if ( !uiad )
    {
	pErrMsg("Invalid auxdata added");
	return;
    }

    uiad->updateCB( 0 );
}


FlatView::AuxData* uiFlatViewer::createAuxData(const char* nm) const
{ return new FlatView::uiAuxDataDisplay(nm); }


FlatView::AuxData* uiFlatViewer::getAuxData(int idx)
{ return auxdata_[idx]; }


const FlatView::AuxData* uiFlatViewer::getAuxData(int idx) const
{ return auxdata_[idx]; }


int uiFlatViewer::nrAuxData() const
{ return auxdata_.size(); }


void uiFlatViewer::addAuxData( FlatView::AuxData* a )
{
    mDynamicCastGet( FlatView::uiAuxDataDisplay*, uiad, a );
    if ( !uiad )
    {
	pErrMsg("Invalid auxdata added");
	return;
    }

    uiad->getDisplay()->setZValue( mAuxDataZ );
    worldgroup_->add( uiad->getDisplay() );
    uiad->setViewer( this );
    auxdata_ += uiad;
}


FlatView::AuxData* uiFlatViewer::removeAuxData( FlatView::AuxData* a )
{
    return removeAuxData( auxdata_.indexOf( (FlatView::uiAuxDataDisplay*) a ) );
}


FlatView::AuxData* uiFlatViewer::removeAuxData( int idx )
{
    if ( idx==-1 )
	return 0;

    worldgroup_->remove( auxdata_[idx]->getDisplay(), true );
    auxdata_[idx]->removeDisplay();
    return auxdata_.removeSingle(idx);
}



void uiFlatViewer::setSelDataRanges( Interval<double> xrg,Interval<double> yrg)
{
    useseldataranges_ = true;
    xseldatarange_ = xrg;
    yseldatarange_ = yrg;
    viewChanged.trigger();
}
