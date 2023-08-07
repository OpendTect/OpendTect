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
#include "uimsg.h"

#include "hiddenparam.h"
static HiddenParam<MPE::uiPreviewGroup,RefMan<FlatDataPack>> hp_fdp_(nullptr);

class HP_uiPreviewGroup
{
public:
    Interval<float>		zintv_;
    Interval<float>		winintv_;
    Interval<float>		manipwinintv_;
};

static HiddenParam<MPE::uiPreviewGroup,HP_uiPreviewGroup*> hp_members(nullptr);

namespace MPE
{

uiPreviewGroup::uiPreviewGroup( uiParent* p )
    : uiGroup(p,"Preview")
    , windowChanged_(this)
    , seedpos_(TrcKeyValue::udf())
    , nrtrcs_(mUdf(int))
    , mousedown_(false)
{
    hp_fdp_.setParam( this, nullptr );
    hp_members.setParam( this, new HP_uiPreviewGroup );

    wvafld_ = new uiCheckList( this, uiCheckList::OneMinimum,
			       OD::Horizontal );
    wvafld_->addItem( tr("WVA") ).addItem( tr("VD") );
    wvafld_->changed.notify( mCB(this,uiPreviewGroup,wvavdChgCB) );

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

    OD::LineStyle ls( OD::LineStyle::Solid, 3, OD::Color(0,255,0) );
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
    hp_fdp_.setParam( this, nullptr );
    hp_fdp_.removeParam( this );
    hp_members.removeAndDeleteParam( this );
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


void uiPreviewGroup::setDisplaySize( int nrtrcs, const Interval<int>& zintv )
{
    pErrMsg("Should not be used");
    nrtrcs_ = nrtrcs;
    zintv_ = zintv;
    updateViewer();
    updateWindowLines();
}


void uiPreviewGroup::setWindow( const Interval<int>& winsz )
{
    pErrMsg("Should not be used");
    winintv_ = winsz;
    updateWindowLines();
}


Interval<int> uiPreviewGroup::getManipWindow() const
{
    pErrMsg("Should not be used");
    return manipwinintv_;
}


void uiPreviewGroup::setDisplaySize( int nrtrcs, const Interval<float>& zintv )
{
    nrtrcs_ = nrtrcs;
    hp_members.getParam(this)->zintv_ = zintv;
    updateViewer();
    updateWindowLines();
}


void uiPreviewGroup::setWindow( const Interval<float>& winsz )
{
    hp_members.getParam(this)->winintv_ = winsz;
    updateWindowLines();
}


Interval<float> uiPreviewGroup::getManipWindowF() const
{
    return hp_members.getParam(this)->manipwinintv_;
}


void uiPreviewGroup::updateViewer()
{
    if ( seedpos_.isUdf() )
	return;

    const TrcKey& tk = seedpos_.tk_;
    const float z = seedpos_.val_;

    StepInterval<float> zintv = hp_members.getParam(this)->zintv_;
    zintv.scale( 1.f/SI().zDomain().userFactor() );
    zintv.step = SI().zStep();

    auto dp = MPE::engine().getSeedPosDataPackRM( tk, z, nrtrcs_, zintv );
    hp_fdp_.setParam( this, dp );

    const bool canupdate = vwr_->enableChange( false );
    vwr_->setPack( FlatView::Viewer::Both, dp );
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

    StepInterval<float> zintv = hp_members.getParam(this)->winintv_;
    zintv.scale( 1.f/float(SI().zDomain().userFactor()) );

    const int so = nrtrcs_/2+1;
    minline_->poly_[0] = FlatView::Point( tk.trcNr()-so, z+zintv.start );
    minline_->poly_[1] = FlatView::Point( tk.trcNr()+so, z+zintv.start );
    maxline_->poly_[0] = FlatView::Point( tk.trcNr()-so, z+zintv.stop );
    maxline_->poly_[1] = FlatView::Point( tk.trcNr()+so, z+zintv.stop );

    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiPreviewGroup::mousePressed( CallBacker* )
{
    if ( !calcNewWindow() )
	return;

    mousedown_ = true;
    windowChanged_.trigger();
}


void uiPreviewGroup::mouseMoved( CallBacker* )
{
    if ( !mousedown_ || !calcNewWindow() )
	return;

    windowChanged_.trigger();
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

    Interval<float>& intv = hp_members.getParam(this)->manipwinintv_;
    intv.setUdf();
    const MouseEvent& ev = meh.event();
    const Geom::Point2D<int>& pt = ev.pos();
    uiWorldPoint wpt = vwr_->getWorld2Ui().transform( pt );
    const double diff = (wpt.y - seedpos_.val_) * SI().zDomain().userFactor();
    if ( wpt.y < seedpos_.val_ )
	intv.start = float( diff );
    else
	intv.stop = float( diff );

    return true;
}

} // namespace MPE
