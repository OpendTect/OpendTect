/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpepreviewgrp.h"

#include "draw.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "survinfo.h"

#include "uichecklist.h"
#include "uiflatviewer.h"
#include "uigraphicsview.h"


namespace MPE
{

uiPreviewGroup::uiPreviewGroup( uiParent* p )
    : uiGroup(p,"Preview")
    , windowChanged(this)
{
    wvafld_ = new uiCheckList( this, uiCheckList::OneMinimum,
			       OD::Horizontal );
    wvafld_->addItem( tr("WVA") ).addItem( tr("VD") );
    mAttachCB( wvafld_->changed, uiPreviewGroup::wvavdChgCB );

    vwr_ = new uiFlatViewer( this );
    vwr_->attach( alignedBelow, wvafld_ );
    vwr_->setPrefWidth( 150 );
    vwr_->setPrefHeight( 200 );
    vwr_->setStretch( 0, 0 );
    vwr_->rgbCanvas().setDragMode( uiGraphicsView::NoDrag );
    vwr_->rgbCanvas().setSceneBorder( 0 );
    vwr_->appearance().ddpars_.wva_.mappersetup_.cliprate_ =
				Interval<float>(0.01f,0.01f);
    vwr_->appearance().setGeoDefaults( true );
    vwr_->appearance().annot_.x1_.hasannot_ = false;
    vwr_->appearance().annot_.x2_.hasannot_ = false;

    const OD::LineStyle ls( OD::LineStyle::Solid, 3, OD::Color(0,255,0) );
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
    seeditm_->markerstyles_[0].color_ = OD::Color(0,255,0);
    vwr_->addAuxData( seeditm_ );

    seedline_ = vwr_->createAuxData( "Seed line" );
    seedline_->linestyle_ =
	OD::LineStyle( OD::LineStyle::Dash, 1, OD::Color(0,255,0) );
    seedline_->poly_ += FlatView::Point(0,0);
    seedline_->poly_ += FlatView::Point(0,0);
    vwr_->addAuxData( seedline_ );

    mAttachCB( vwr_->getMouseEventHandler().buttonPressed,
	       uiPreviewGroup::mousePressed );
    mAttachCB( vwr_->getMouseEventHandler().buttonReleased,
	       uiPreviewGroup::mouseReleased );
    mAttachCB( vwr_->getMouseEventHandler().movement,
	       uiPreviewGroup::mouseMoved );

    wvafld_->setChecked( 0, true );
    wvafld_->setChecked( 1, false );

    setHAlignObj( wvafld_ );
}


uiPreviewGroup::~uiPreviewGroup()
{
    detachAllNotifiers();
}


void uiPreviewGroup::wvavdChgCB( CallBacker* )
{
    vwr_->appearance().ddpars_.show( wvafld_->isChecked(0),
				     wvafld_->isChecked(1) );
    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
}


void uiPreviewGroup::setSeedPos( const TrcKeyValue& tkv )
{
    seedpos_ = tkv;
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


Interval<float> uiPreviewGroup::getManipWindow() const
{
    return manipwinintv_;
}


void uiPreviewGroup::updateViewer()
{
    if ( seedpos_.isUdf() )
	return;

    const TrcKey& tk = seedpos_.tk_;
    const float z = seedpos_.val_;

    ZSampling zintv;
    zintv.setFrom( zintv_ );
    zintv.scale( 1.f/float(SI().zDomain().userFactor()) );
    zintv.step_ = SI().zStep();

    fdp_ = MPE::engine().getSeedPosDataPack( tk, z, nrtrcs_, zintv );

    const bool canupdate = vwr_->enableChange( false );
    vwr_->setPack( FlatView::Viewer::Both, fdp_.ptr(), false );
    vwr_->appearance().ddpars_.wva_.mappersetup_.setAutoScale( true );
    vwr_->appearance().ddpars_.vd_.mappersetup_.setAutoScale( true );
    vwr_->appearance().ddpars_.show( wvafld_->isChecked(0),
				     wvafld_->isChecked(1) );
    vwr_->setViewToBoundingBox();

    seeditm_->poly_[0] = FlatView::Point( tk.trcNr(), z );

    const int so = nrtrcs_/2+1;
    seedline_->poly_[0] = FlatView::Point( tk.trcNr()-so, z );
    seedline_->poly_[1] = FlatView::Point( tk.trcNr()+so, z );

    vwr_->enableChange( canupdate );
    vwr_->handleChange( sCast(od_uint32,FlatView::Viewer::All) );
}


void uiPreviewGroup::updateWindowLines()
{
    if ( seedpos_.isUdf() )
	return;

    const TrcKey& tk = seedpos_.tk_;
    const float z = seedpos_.val_;

    StepInterval<float> zintv;
    zintv.setFrom( winintv_ );
    zintv.scale( 1.f/float(SI().zDomain().userFactor()) );

    const int so = nrtrcs_/2+1;
    minline_->poly_[0] = FlatView::Point( tk.trcNr()-so, z+zintv.start_ );
    minline_->poly_[1] = FlatView::Point( tk.trcNr()+so, z+zintv.start_ );
    maxline_->poly_[0] = FlatView::Point( tk.trcNr()-so, z+zintv.stop_ );
    maxline_->poly_[1] = FlatView::Point( tk.trcNr()+so, z+zintv.stop_ );

    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiPreviewGroup::mousePressed( CallBacker* )
{
    if ( !calcNewWindow() )
	return;

    mousedown_ = true;
    windowChanged.trigger();
}


void uiPreviewGroup::mouseMoved( CallBacker* )
{
    if ( !mousedown_ || !calcNewWindow() )
	return;

    windowChanged.trigger();
}


void uiPreviewGroup::mouseReleased( CallBacker* )
{
    mousedown_ = false;
}


bool uiPreviewGroup::calcNewWindow()
{
    MouseEventHandler& meh = vwr_->getMouseEventHandler();
    if ( seedpos_.isUdf() )
	return false;

    manipwinintv_.setUdf();
    const MouseEvent& ev = meh.event();
    const Geom::Point2D<int>& pt = ev.pos();
    uiWorldPoint wpt = vwr_->getWorld2Ui().transform( pt );
    const double diff = (wpt.y_ - seedpos_.val_) * SI().zDomain().userFactor();
    if ( wpt.y_ < seedpos_.val_ )
	manipwinintv_.start_ = float( diff );
    else
	manipwinintv_.stop_ = float( diff );

    return true;
}

} // namespace MPE
