/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewer2d.cc,v 1.6 2011-05-04 15:20:02 cvsbruno Exp $";

#include "uipsviewer2d.h"

#include "flatposdata.h"
#include "psviewer2dgatherpainter.h"
#include "uiflatviewer.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uiobjectitemview.h"
#include "uirgbarraycanvas.h"

namespace PreStackView
{

uiGatherDisplay::uiGatherDisplay( uiParent* p, bool havehandpan )
    : uiGroup(p, "Pre-stack gather Display" )
    , bid_(-1,-1)
    , zrg_(0)
    , fixedoffset_( false )
    , offsetrange_( mUdf(float), mUdf(float) )
    , displayannotation_(true)					      
{
    viewer_ = new uiFlatViewer( this, havehandpan );
    viewer_->appearance().setGeoDefaults( true );
    viewer_->appearance().setDarkBG( false );
    viewer_->appearance().annot_.color_ = Color::Black();
    viewer_->appearance().annot_.x1_.showannot_ = false;
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
    setWidth( sz.width() );
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
    offsetrange_.set( pd.range(true).start, pd.range(false).stop );
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

    uiWorldRect cur = viewer_->curView();
    if ( !fixedoffset_ )
    {
	const uiWorldRect& bbox = viewer_->boundingBox();
	cur.setLeft( bbox.left() );
	cur.setRight( bbox.right() );
    }

    updateViewRange( cur );
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

}; //namepsace
