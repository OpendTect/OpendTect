/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipsviewer2d.h"

#include "flatposdata.h"
#include "flatviewzoommgr.h"
#include "psviewer2dgatherpainter.h"

#include "uiflatviewer.h"
#include "uiflatviewcontrol.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uiobjectitemview.h"
#include "uirgbarraycanvas.h"

namespace PreStackView
{

uiGatherDisplay::uiGatherDisplay( uiParent* p  )
    : uiGroup(p, "Prestack gather Display" )
{
    viewer_ = new uiFlatViewer( this );
    viewer_->appearance().setGeoDefaults( true );
    viewer_->appearance().setDarkBG( false );
    viewer_->appearance().annot_.color_ = OD::Color::Black();
    viewer_->appearance().annot_.x1_.showannot_ = true;
    viewer_->appearance().annot_.x1_.name_ = "Offset";
    viewer_->appearance().annot_.x2_.showannot_ = false;
    viewer_->appearance().annot_.x2_.name_ = "Depth";
    viewer_->appearance().ddpars_.wva_.show_ = false;
    viewer_->appearance().ddpars_.vd_.show_ = true;
    viewer_->appearance().ddpars_.vd_.lininterp_ = true;
    viewer_->appearance().ddpars_.vd_.mappersetup_.symmidval_ = 0;

    gatherpainter_ = new Viewer2DGatherPainter( *viewer_ );
}


uiGatherDisplay::~uiGatherDisplay()
{
    NotifyStopper ns( viewer_->dataChanged );
    delete gatherpainter_;
}


void uiGatherDisplay::setInitialSize( const uiSize& sz )
{
    setPrefWidth( sz.width() );
    setPrefHeight( sz.height() );
    viewer_->setInitialSize( sz );
}


void uiGatherDisplay::setWidth( int width )
{
    viewer_->rgbCanvas().setMinimumWidth( width );
    viewer_->rgbCanvas().setMaximumWidth( width );
}


void uiGatherDisplay::setVDGather( const PreStack::Gather* vddp )
{
    gatherpainter_->setVDGather( vddp );
    ConstRefMan<FlatDataPack> dp = viewer_->getPack( false ).get();
    if ( !dp )
	return;

    const FlatPosData& pd = dp->posData();
    offsetrange_.set( (float)pd.range(true).start_,
		      (float)pd.range(true).stop_ );
    zdatarange_.set( (float)pd.range(false).start_,
		     (float)pd.range(false).stop_ );
}


void uiGatherDisplay::setWVAGather( const PreStack::Gather* wvadp )
{
    gatherpainter_->setWVAGather( wvadp );
    ConstRefMan<FlatDataPack> dp = viewer_->getPack( true ).get();
    if ( !dp )
	return;

    const FlatPosData& pd = dp->posData();
    offsetrange_.set( (float)pd.range(true).start_,
		      (float)pd.range(true).stop_ );
    zdatarange_.set( (float)pd.range(false).start_,
		     (float)pd.range(false).stop_ );
}


TrcKey uiGatherDisplay::getTrcKey() const
{
    return gatherpainter_->getTrcKey();
}


void uiGatherDisplay::setZRange( const Interval<double>* zrg )
{
    if ( zrg )
    {
	if ( !zrg_ )
	    zrg_ = new Interval<double>( *zrg );
	else
	    *zrg_ = *zrg;
    }
    else if ( zrg_ )
	deleteAndNullPtr( zrg_ );
}


void uiGatherDisplay::displayAnnotation( bool yn )
{
    displayannotation_ = yn;
    viewer_->appearance().annot_.x1_.hasannot_ = yn;
    viewer_->appearance().annot_.x2_.hasannot_ = yn;
    viewer_->appearance().annot_.x1_.showannot_ = yn;
    viewer_->appearance().annot_.x2_.showannot_ = yn;
    viewer_->handleChange( FlatView::Viewer::Annot );
}


bool uiGatherDisplay::displaysAnnotation() const
{
    return displayannotation_;
}


void uiGatherDisplay::setFixedOffsetRange( bool yn, const Interval<float>& rg )
{
    if ( yn==fixedoffset_ &&
	 mIsEqual(offsetrange_.start_,rg.start_,1e-3) &&
	 mIsEqual(offsetrange_.stop_,rg.stop_,1e-3) )
	return;

    fixedoffset_ = yn;
    offsetrange_ = rg;

    if ( viewer_->control() )
	viewer_->control()->zoomMgr().toStart();

    Interval<double> offrg;
    offrg.setFrom( rg );
    const uiWorldRect& bbox = viewer_->boundingBox();
    Interval<double> zrg( bbox.top(), bbox.bottom() );
    viewer_->setSelDataRanges( offrg, zrg );
    viewer_->setUseSelDataRanges( yn );

    const uiWorldRect& newbbox = viewer_->boundingBox();
    updateViewRange( newbbox );

    if ( viewer_->control() )
	viewer_->control()->setNewView(
		newbbox.centre(), newbbox.size(), viewer_ );
}


bool uiGatherDisplay::getFixedOffsetRange() const
{
    return fixedoffset_;
}


const Interval<float>& uiGatherDisplay::getOffsetRange() const
{
    return offsetrange_;
}


void uiGatherDisplay::updateViewRange()
{
    updateViewRange( viewer_->boundingBox() );
}


void uiGatherDisplay::updateViewRange( const uiWorldRect& cur )
{
    uiWorldRect view = cur;
    if ( zrg_ )
    {
	view.setTop( zrg_->start_ );
	view.setBottom( zrg_->stop_ );
    }

    if ( viewer_ && fixedoffset_ && !mIsUdf(offsetrange_.start_) &&
	 !mIsUdf(offsetrange_.stop_) )
    {
	view.setLeft( offsetrange_.start_ );
	view.setRight( offsetrange_.stop_ );
    }
    viewer_->setView( view );
}


// uiViewer2D
uiViewer2D::uiViewer2D( uiParent* p )
    : uiObjectItemView(p)
    , resizedraw_(false)
{
    disableScrollZoom();
}


uiViewer2D::~uiViewer2D()
{
    enableReSizeDraw( false );
}


void uiViewer2D::enableReSizeDraw( bool yn )
{
    if ( yn && !resizedraw_ )
	reSize.notify( mCB(this,uiViewer2D,reSized) );
    else if ( !yn && resizedraw_ )
	reSize.remove(  mCB(this,uiViewer2D,reSized) );

    resizedraw_ = yn;
}


void uiViewer2D::enableScrollBars( bool yn )
{
    ScrollBarPolicy pol = yn ? ScrollBarAsNeeded : ScrollBarAlwaysOff;
    setScrollBarPolicy( true, pol );
    setScrollBarPolicy( false, pol );
}


uiGatherDisplay* uiViewer2D::addGatherDisplay( PreStack::Gather* vd,
					       PreStack::Gather* wva )
{
    auto* gatherdisp = new uiGatherDisplay( nullptr );
    gatherdisp->setVDGather( vd );
    gatherdisp->setWVAGather( wva );
    addGatherDisplay( gatherdisp );

    return gatherdisp;
}


void uiViewer2D::addGatherDisplay( uiGatherDisplay* gatherview )
{
    addItem( new uiObjectItem( gatherview ) );
}


void uiViewer2D::removeGatherDisplay( const uiGatherDisplay* disp )
{
    for ( int idx=objectitems_.size()-1; idx>=0; idx-- )
    {
	uiObjectItem* objitm = objectitems_[idx];
	if ( objitm->getGroup() == disp )
	{
	    removeItem( objitm );
	    delete objitm;
	}
    }
}


void uiViewer2D::removeAllGatherDisplays()
{
    for ( int idx=objectitems_.size()-1; idx>=0; idx-- )
	removeItem( objectitems_[idx] );
}


uiGatherDisplay* uiViewer2D::getGatherDisplay( const TrcKey& tk )
{
    for ( int idx=0; idx<objectitems_.size(); idx++ )
    {
	uiGatherDisplay& gdisp = getGatherDisplay( idx );
	if ( gdisp.getTrcKey() == tk )
	    return &gdisp;
    }

    return nullptr;
}


uiGatherDisplay& uiViewer2D::getGatherDisplay( int idx )
{
    return sCast(uiGatherDisplay&,*getItem(idx)->getGroup());
}


void uiViewer2D::reSized( CallBacker* )
{
    doReSize( uiSize(viewWidth(),viewHeight()) );
}


void uiViewer2D::doReSize( const uiSize& sz )
{
    if ( objectitems_.isEmpty() )
	return;

    const uiSize objsz( sz.width() / objectitems_.size(), sz.height() );
    for ( int idx=0; idx<objectitems_.size() ; idx++ )
    {
	reSizeItem( idx, objsz );
	getGatherDisplay( idx ).setWidth( objsz.width() );
    }

    resetViewArea( nullptr );
}


void uiGatherDisplay::setVDGather( DataPackID vdid )
{
    auto gather = DPM(DataPackMgr::FlatID()).get<PreStack::Gather>( vdid );
    setVDGather( gather );
}


void uiGatherDisplay::setWVAGather( DataPackID wvaid )
{
    auto gather = DPM(DataPackMgr::FlatID()).get<PreStack::Gather>( wvaid );
    setWVAGather( gather );
}


uiGatherDisplay* uiViewer2D::addGatherDisplay( DataPackID vdid,
					       DataPackID wvaid )
{
    auto vdgather = DPM(DataPackMgr::FlatID()).get<PreStack::Gather>( vdid );
    auto wvagather = DPM(DataPackMgr::FlatID()).get<PreStack::Gather>( wvaid );
    return addGatherDisplay( vdgather, wvagather );
}

} // namespace PreStackView
