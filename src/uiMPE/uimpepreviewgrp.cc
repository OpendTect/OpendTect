/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uihorizontracksetup.cc 38749 2015-04-02 19:49:51Z nanne.hemstra@dgbes.com $";

#include "uimpepreviewgrp.h"

#include "draw.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "survinfo.h"

#include "uichecklist.h"
#include "uiflatviewer.h"
#include "uigraphicsview.h"
#include "uimsg.h"


namespace MPE
{

uiPreviewGroup::uiPreviewGroup( uiParent* p )
    : uiGroup(p,"Preview")
    , windowChanged_(this)
    , seedpos_(Coord3::udf())
    , nrtrcs_(mUdf(int))
    , mousedown_(false)
{
    wvafld_ = new uiCheckList( this, uiCheckList::OneMinimum,
			       OD::Horizontal );
    wvafld_->addItem( "WVA" ).addItem( "VD" );
    wvafld_->changed.notify( mCB(this,uiPreviewGroup,wvavdChgCB) );

    vwr_ = new uiFlatViewer( this );
    vwr_->attach( alignedBelow, wvafld_ );
    vwr_->setPrefWidth( 150 );
    vwr_->setPrefHeight( 200 );
    vwr_->setStretch( 0, 0 );
    vwr_->rgbCanvas().setDragMode( uiGraphicsView::NoDrag );
    vwr_->appearance().ddpars_.wva_.mappersetup_.cliprate_ =
				Interval<float>(0.01f,0.01f);
    vwr_->appearance().setGeoDefaults( true );

    LineStyle ls( LineStyle::Solid, 3, Color(0,255,0) );
    minline_ = vwr_->createAuxData( "Min line" );
    minline_->cursor_.shape_ = MouseCursor::SizeVer;
    minline_->linestyle_ = ls;
    minline_->poly_ += FlatView::Point(0,0);
    minline_->poly_ += FlatView::Point(0,0);
    vwr_->addAuxData( minline_ );

    maxline_ = vwr_->createAuxData( "Max line" );
    maxline_->cursor_.shape_ = MouseCursor::SizeVer;
    maxline_->linestyle_ = ls;
    maxline_->poly_ += FlatView::Point(0,0);
    maxline_->poly_ += FlatView::Point(0,0);
    vwr_->addAuxData( maxline_ );

    seeditm_ = vwr_->createAuxData( "Seed" );
    seeditm_->poly_ += FlatView::Point(0,0);
    seeditm_->markerstyles_ += MarkerStyle2D(MarkerStyle2D::Square,3);
    seeditm_->markerstyles_[0].color_ = Color(0,255,0);
    vwr_->addAuxData( seeditm_ );

    seedline_ = vwr_->createAuxData( "Seed line" );
    seedline_->linestyle_ = LineStyle( LineStyle::Dash, 1, Color(0,255,0) );
    seedline_->poly_ += FlatView::Point(0,0);
    seedline_->poly_ += FlatView::Point(0,0);
    vwr_->addAuxData( seedline_ );

    vwr_->getMouseEventHandler().buttonPressed.notify(
			     mCB(this,uiPreviewGroup,mousePressed) );
    vwr_->getMouseEventHandler().buttonReleased.notify(
			     mCB(this,uiPreviewGroup,mouseReleased) );
    vwr_->getMouseEventHandler().movement.notify(
			     mCB(this,uiPreviewGroup,mouseMoved) );

    wvafld_->setChecked( 0, true );
    wvafld_->setChecked( 1, false );

    setHAlignObj( wvafld_ );
}


uiPreviewGroup::~uiPreviewGroup()
{
}


void uiPreviewGroup::wvavdChgCB( CallBacker* )
{
    vwr_->appearance().ddpars_.show( wvafld_->isChecked(0),
					    wvafld_->isChecked(1) );
    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
}


void uiPreviewGroup::setSeedPos( const Coord3& crd )
{
    seedpos_ = crd;
    updateViewer();
    updateWindowLines();
}


void uiPreviewGroup::setDisplaySize( int nrtrcs, const Interval<float>& zintv )
{
    nrtrcs_ = nrtrcs;
    zintv_ = zintv;
    updateViewer();
    updateWindowLines();
}


void uiPreviewGroup::setWindow( const Interval<float>& winsz )
{
    winintv_ = winsz;
    updateWindowLines();
}


Interval<float> uiPreviewGroup::getWindow() const
{ return winintv_; }


void uiPreviewGroup::updateViewer()
{
    if ( seedpos_.isUdf() )
	return;

    const BinID& bid = SI().transform( seedpos_.coord() );
    const float z = (float)seedpos_.z;

    const DataPack::ID dpid =
	MPE::engine().getSeedPosDataPack( bid, z, nrtrcs_, zintv_ );

    vwr_->setPack( true, dpid );
    vwr_->setPack( false, dpid );
    vwr_->appearance().ddpars_.show( wvafld_->isChecked(0),
				     wvafld_->isChecked(1) );
    vwr_->setViewToBoundingBox();

    FlatView::Point& pt = seeditm_->poly_[0];
    pt = FlatView::Point( bid.crl(), z );

    seedline_->poly_[0] = FlatView::Point( bid.crl()-nrtrcs_/2-1, z );
    seedline_->poly_[1] = FlatView::Point( bid.crl()+nrtrcs_/2+1, z );

    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
}


void uiPreviewGroup::updateWindowLines()
{
    if ( seedpos_.isUdf() )
	return;

    const BinID& bid = SI().transform( seedpos_.coord() );
    const float z = (float)seedpos_.z;

    minline_->poly_[0] = FlatView::Point( bid.crl()-nrtrcs_/2, z+winintv_.start );
    minline_->poly_[1] = FlatView::Point( bid.crl()+nrtrcs_/2, z+winintv_.start );
    maxline_->poly_[0] = FlatView::Point( bid.crl()-nrtrcs_/2, z+winintv_.stop );
    maxline_->poly_[1] = FlatView::Point( bid.crl()+nrtrcs_/2, z+winintv_.stop );

    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiPreviewGroup::mousePressed( CallBacker* )
{
    MouseEventHandler& meh = vwr_->getMouseEventHandler();
    if ( seedpos_.isUdf() || meh.isHandled() )
	return;

    const MouseEvent& ev = meh.event();
    const Geom::Point2D<int>& pt = ev.pos();
    uiWorldPoint wpt = vwr_->getWorld2Ui().transform( pt );
    const double diff = Math::Abs(seedpos_.z-wpt.y)*SI().zDomain().userFactor();
    if ( wpt.y < seedpos_.z )
	compwinfld_->setValue( mNINT32(-diff), 0 );
    else
	compwinfld_->setValue( mNINT32(diff), 1 );

    mousedown_ = true;
}


void uiPreviewGroup::mouseMoved( CallBacker* )
{
    if ( !mousedown_ ) return;

    MouseEventHandler& meh = vwr_->getMouseEventHandler();
    const MouseEvent& ev = meh.event();
    const Geom::Point2D<int>& pt = ev.pos();
    uiWorldPoint wpt = vwr_->getWorld2Ui().transform( pt );
    const double diff = Math::Abs(seedpos_.z-wpt.y)*SI().zDomain().userFactor();
    if ( wpt.y < seedpos_.z )
	compwinfld_->setValue( mNINT32(-diff), 0 );
    else
	compwinfld_->setValue( mNINT32(diff), 1 );
}


void uiPreviewGroup::mouseReleased( CallBacker* )
{
    mousedown_ = false;
}

} //namespace MPE
