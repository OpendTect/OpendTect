/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uipsviewer2d.cc,v 1.15 2012-09-13 19:00:23 cvsnanne Exp $";

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
    : uiGroup(p, "Pre-stack gather Display" )
    , bid_(-1,-1)
    , zrg_(0)
    , fixedoffset_( false )
    , offsetrange_( mUdf(float), mUdf(float) )
    , displayannotation_(true)					      
{
    viewer_ = new uiFlatViewer( this );
    viewer_->appearance().setGeoDefaults( true );
    viewer_->appearance().setDarkBG( false );
    viewer_->appearance().annot_.color_ = Color::Black();
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


void uiGatherDisplay::setInitialSize( const uiSize& sz )
{
    setPrefWidth( sz.width() ); setPrefHeight( sz.height() );
    viewer_->setInitialSize( sz );
}


void uiGatherDisplay::setWidth( int width )
{
    viewer_->rgbCanvas().setMinimumWidth( width );
    viewer_->rgbCanvas().setMaximumWidth( width );
}


uiGatherDisplay::~uiGatherDisplay()
{
    NotifyStopper ns( viewer_->dataChanged );
    delete gatherpainter_;
}


void uiGatherDisplay::setGather( int id )
{
    gatherpainter_->setGather( id );
    const FlatDataPack* dp = viewer_->pack( true );
    if ( !dp ) dp = viewer_->pack( false );
    if ( !dp ) return;
    const FlatPosData& pd = dp->posData();
    offsetrange_.set( (float)pd.range(true).start,
		      (float)pd.range(false).stop );
    zdatarange_.set( (float)pd.range(false).start,
		     (float)pd.range(false).stop );
}


BinID uiGatherDisplay::getBinID() const
{ return gatherpainter_->getBinID(); }


void uiGatherDisplay::setPosition( const BinID& bid, const Interval<double>* zrg )
{
    bid_ = bid;
    if ( zrg )
    {
	if ( !zrg_ ) zrg_ = new Interval<double>( *zrg );
	else *zrg_ = *zrg;
    }
    else if ( zrg_ ) { delete zrg_; zrg_ = 0; }
}


void uiGatherDisplay::displayAnnotation( bool yn )
{
    displayannotation_ = yn;
    viewer_->appearance().annot_.x1_.showannot_ = yn;
    viewer_->appearance().annot_.x2_.showannot_ = yn;
    viewer_->handleChange( FlatView::Viewer::Annot );
}


bool uiGatherDisplay::displaysAnnotation() const
{ return displayannotation_; }



void uiGatherDisplay::setFixedOffsetRange( bool yn, const Interval<float>& rg )
{
    if ( yn==fixedoffset_ && 
	mIsEqual(offsetrange_.start,rg.start,1e-3) &&
	mIsEqual(offsetrange_.stop,rg.stop,1e-3) )
	return;

    fixedoffset_ = yn;
    offsetrange_ = rg;

    if ( viewer_->control() )
	viewer_->control()->zoomMgr().toStart();

    Interval<double> offrg( rg.start, rg.stop );
    const uiWorldRect& bbox = viewer_->boundingBox();
    Interval<double> zrg( bbox.top(), bbox.bottom() );
    viewer_->setSelDataRanges( offrg, zrg );
    viewer_->setUseSelDataRanges( yn );

    const uiWorldRect& newbbox = viewer_->boundingBox();
    updateViewRange( newbbox );

    if ( viewer_->control() )
    {
	Geom::Point2D<double> centre = newbbox.centre();
	Geom::Size2D<double> newsz = newbbox.size();
	viewer_->control()->setNewView( centre, newsz );
    }
}


bool uiGatherDisplay::getFixedOffsetRange() const
{ return fixedoffset_; }


const Interval<float>& uiGatherDisplay::getOffsetRange() const
{ return offsetrange_; }


void uiGatherDisplay::updateViewRange( const uiWorldRect& cur )
{
    uiWorldRect view = cur;
    if ( zrg_ )
    {
	view.setTop( zrg_->start );
	view.setBottom( zrg_->stop );
    }

    if ( viewer_ && fixedoffset_ && !mIsUdf(offsetrange_.start) &&
	 !mIsUdf(offsetrange_.stop) )
    {
	view.setLeft( offsetrange_.start );
	view.setRight( offsetrange_.stop );
    }
    viewer_->setView( view );
}




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


uiGatherDisplay* uiViewer2D::addGatherDisplay( int id  )
{
    uiGatherDisplay* gatherdisp = new uiGatherDisplay( 0 );
    gatherdisp->setGather( id );
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
    {
	removeItem( objectitems_[idx] );
    }
}


uiGatherDisplay* uiViewer2D::getGatherDisplay( const BinID& bid )
{
    for ( int idx=0; idx<objectitems_.size(); idx++ )
    {
	uiGatherDisplay& gdisp = getGatherDisplay( idx );
	if ( gdisp.getBinID() == bid )
	    return &gdisp;
    }
    return 0;
}


uiGatherDisplay& uiViewer2D::getGatherDisplay( int idx )
{ return (uiGatherDisplay&)(*getItem( idx )->getGroup()); }


void uiViewer2D::reSized( CallBacker* )
{
    doReSize( uiSize( width(), height() ) );
}


void uiViewer2D::doReSize( const uiSize& sz )
{
    if ( !objectitems_.size() ) return;
    uiSize objsz = uiSize( sz.width() / objectitems_.size() , sz.height() );
    for ( int idx=0; idx<objectitems_.size() ; idx++ )
    {
	reSizeItem( idx, objsz ); 
	getGatherDisplay( idx ).setWidth( objsz.width() );
    }
    resetViewArea(0);
}


}; //namepsace
