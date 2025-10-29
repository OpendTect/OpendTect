/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uislicepos.h"

#include "uilabel.h"
#include "uishortcutsmgr.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "trckeyzsampling.h"
#include "ioman.h"
#include "survinfo.h"


uiSlicePos::uiSlicePos( uiParent* p )
    : positionChg(this)
    , sliderReleased(this)
{
    toolbar_ = new uiToolBar( p, uiStrings::phrJoinStrings(uiStrings::sSlice(),
			      uiStrings::sPosition()) );

    boxlabels_.add( uiStrings::sInline() );
    boxlabels_.add( uiStrings::sCrossline() );
    boxlabels_.add( uiStrings::sZ() );

    label_ = new uiLabel( toolbar_, boxlabels_[1] );
    sliceposbox_ = new uiSpinBox( toolbar_, 0, "Slice position" );
    mAttachCB( sliceposbox_->valueChanging, uiSlicePos::slicePosChg );

    auto* steplabel = new uiLabel( toolbar_, uiStrings::sStep() );

    slicestepbox_ = new uiSpinBox( toolbar_, 0, "Slice step" );
    slicestepbox_->valueChanging.notify( mCB(this,uiSlicePos,sliceStepChg) );

    BufferString prevscstr, nextscstr;
    const uiShortcutsList& scl = SCMgr().getList( "ODScene" );
    const uiKeyDesc* keydesc = scl.keyDescOf( "Move slice forward" );
    if ( keydesc )
	prevscstr = keydesc->getKeySequenceStr();

    keydesc = scl.keyDescOf( "Move slice backward" );
    if ( keydesc )
	nextscstr = keydesc->getKeySequenceStr();

    uiString prevttip = tr( "Previous position \n\nShortcut: \"%1\"" )
			    .arg( prevscstr.isEmpty() ? "z" : prevscstr.buf() );
    uiString nextttip = tr( "Next position \n\nShortcut: \"%1\"" )
			    .arg( nextscstr.isEmpty() ? "x" : nextscstr.buf() );
    prevbut_ = new uiToolButton( toolbar_, "prevpos", prevttip,
				mCB(this,uiSlicePos,prevCB) );
    sliceslider_ = new uiSlider( nullptr, uiSlider::Setup(), "Slice slider" );
    mAttachCB( sliceslider_->sliderMoved, uiSlicePos::sliderSlicePosChg );
    mAttachCB( sliceslider_->sliderReleased, uiSlicePos::sliderReleasedCB );
    nextbut_ = new uiToolButton( toolbar_, "nextpos", nextttip,
				mCB(this,uiSlicePos,nextCB) );

    toolbar_->addObject( label_ );
    toolbar_->addObject( sliceposbox_ );
    toolbar_->addObject( steplabel );
    toolbar_->addObject( slicestepbox_ );
    toolbar_->addObject( prevbut_ );
    toolbar_->addObject( sliceslider_->slider() );
    toolbar_->addObject( nextbut_ );

    mAttachCB( IOM().surveyChanged, uiSlicePos::initSteps );
    mAttachCB( SCMgr().shortcutsChanged, uiSlicePos::shortcutsChg );
    initSteps();
    shortcutsChg( nullptr );
}


uiSlicePos::~uiSlicePos()
{
    detachAllNotifiers();
}


void uiSlicePos::shortcutsChg( CallBacker* )
{
    const uiShortcutsList& scl = SCMgr().getList( "ODScene" );
    const uiKeyDesc* keydesc = scl.keyDescOf( "Move slice forward" );
    if ( keydesc )
    {
	const BufferString keystr = keydesc->getKeySequenceStr();
	nextbut_->setShortcut( keystr.buf() );
	nextbut_->setToolTip( tr("Next position \n\nShortcut: \"%1\"")
				.arg(keystr) );
    }

    keydesc = scl.keyDescOf( "Move slice backward" );
    if ( keydesc )
    {
	const BufferString keystr = keydesc->getKeySequenceStr();
	prevbut_->setShortcut( keystr.buf() );
	prevbut_->setToolTip( tr("Previous position \n\nShortcut: \"%1\"")
				.arg(keystr) );

    }
}


void uiSlicePos::initSteps( CallBacker* )
{
    laststeps_[0] = SI().inlStep();
    laststeps_[1] = SI().crlStep();
    laststeps_[2] = SI().zStep()*SI().zDomain().userFactor();
}


int uiSlicePos::getStep( SliceDir dir ) const
{
    const float fstep = laststeps_[ sCast(int,dir) ];
    return mNINT32( fstep );
}


void uiSlicePos::setStep( SliceDir dir, int step )
{
    laststeps_[ sCast(int,dir) ] = step;
}


void uiSlicePos::setSteps( int inl, int crl, float z )
{
    laststeps_[0] = inl>0 ? inl : SI().inlStep();
    laststeps_[1] = crl>0 ? crl : SI().crlStep();
    const float zfac = mIsUdf(zfactor_) ? 1.f : zfactor_;
    laststeps_[2] = z>0 ? z : SI().zStep()*zfac;
}


float uiSlicePos::getZStep() const
{
    return laststeps_[2];
}


void uiSlicePos::setZStep( float step )
{
    const float zfac = mIsUdf(zfactor_) ? 1.f : zfactor_;
    laststeps_[2] = step>0 ? step : SI().zStep()*zfac;
}


void uiSlicePos::setLabels( const uiString& inl, const uiString& crl,
			    const uiString& z )
{
    boxlabels_[0] = inl;
    boxlabels_[1] = crl;
    boxlabels_[2] = z;
}


void uiSlicePos::setBoxLabel( uiSlicePos::SliceDir orientation )
{
    if ( orientation == OD::SliceType::Inline )
	label_->setText( boxlabels_[0] );
    else if ( orientation == OD::SliceType::Crossline )
	label_->setText( boxlabels_[1] );
    else
	label_->setText( boxlabels_[2] );
}


void uiSlicePos::updatePos( CallBacker* )
{
    doUpdatePos();
}


void uiSlicePos::doUpdatePos()
{
    setPosBoxValue();
}


bool uiSlicePos::isSliderActive() const
{
    return isslideractive_;
}


void uiSlicePos::sliderReleasedCB( CallBacker* )
{
    isslideractive_ = false;
    uiSpinBox* posbox = sliceposbox_;
    OD::SliceType type = getOrientation();
    if ( type == OD::SliceType::Inline || type == OD::SliceType::Crossline )
	posbox->setValue( sliceslider_->getIntValue() );
    else
	posbox->setValue( sliceslider_->getFValue() );

    sliderReleased.trigger();
}


void uiSlicePos::sliderSlicePosChg( CallBacker* )
{
    sliderPosChanged();
}


void uiSlicePos::sliderPosChanged()
{
    if ( !isslideractive_ )
	isslideractive_ = true;

    uiSpinBox* posbox = sliceposbox_;
    OD::SliceType type = getOrientation();
    if ( type == OD::SliceType::Inline || type == OD::SliceType::Crossline )
	posbox->setValue( sliceslider_->getIntValue() );
    else
	posbox->setValue( sliceslider_->getFValue() );
}


void uiSlicePos::slicePosChanged( uiSlicePos::SliceDir orientation,
				  const TrcKeyZSampling& oldcs )
{
    uiSpinBox* posbox = sliceposbox_;
    curcs_ = oldcs;
    NotifyStopper ns( sliceslider_->sliderMoved );
    if ( orientation == OD::SliceType::Inline )
    {
	const int posboxval = posbox->getIntValue();
	curcs_.hsamp_.start_.inl() =
		curcs_.hsamp_.stop_.inl() = posboxval;
	sliceslider_->setValue( posboxval );
    }
    else if ( orientation == OD::SliceType::Crossline )
    {
	const int posboxval = posbox->getIntValue();
	curcs_.hsamp_.start_.crl() =
		curcs_.hsamp_.stop_.crl() = posboxval;
	sliceslider_->setValue( posboxval );
    }
    else
    {
	const float zfac = mIsUdf(zfactor_) ? 1.f : zfactor_;
	const float posboxval = posbox->getIntValue();
        curcs_.zsamp_.start_ = curcs_.zsamp_.stop_
			     = posboxval/zfac;
	sliceslider_->setValue( posboxval );
    }

    if ( oldcs == curcs_ )
	return;

    positionChg.trigger();
}


void uiSlicePos::sliceStepChanged( uiSlicePos::SliceDir orientation )
{
    laststeps_[(int)orientation] = slicestepbox_->getFValue();
    sliceposbox_->setStep( laststeps_[(int)orientation] );
    sliceslider_->setStep( laststeps_[(int)orientation] );
}


void uiSlicePos::setBoxRg( uiSlicePos::SliceDir orientation,
				const TrcKeyZSampling& curcs,
				const TrcKeyZSampling& survcs )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    NotifyStopper posstop( posbox->valueChanging );
    NotifyStopper stepstop1( stepbox->valueChanged );
    NotifyStopper stepstop2( stepbox->valueChanging );
    NotifyStopper sliderstop( sliceslider_->sliderMoved );

    if ( orientation == OD::SliceType::Inline )
    {
	posbox->setInterval( curcs.hsamp_.start_.inl(),
		curcs.hsamp_.stop_.inl() );
	stepbox->setInterval( survcs.hsamp_.step_.inl(),
		curcs.hsamp_.stop_.inl()-curcs.hsamp_.start_.inl(),
		curcs.hsamp_.step_.inl() );
	posbox->setNrDecimals( 0 );
	sliceslider_->setInterval( curcs.hsamp_.start_.inl(),
				   curcs.hsamp_.stop_.inl(),
				   stepbox->getIntValue() );
	stepbox->setNrDecimals( 0 );
    }
    else if ( orientation == OD::SliceType::Crossline )
    {
	posbox->setInterval( curcs.hsamp_.start_.crl(),
		curcs.hsamp_.stop_.crl() );
	stepbox->setInterval( survcs.hsamp_.step_.crl(),
		curcs.hsamp_.stop_.crl()-curcs.hsamp_.start_.crl(),
		curcs.hsamp_.step_.crl() );
	posbox->setNrDecimals( 0 );
	sliceslider_->setInterval( curcs.hsamp_.start_.crl(),
				   curcs.hsamp_.stop_.crl(),
				   stepbox->getIntValue() );
	stepbox->setNrDecimals( 0 );
    }
    else
    {
	const float zfac = mIsUdf(zfactor_) ? 1.f : zfactor_;
	const int nrdec =
		Math::NrSignificantDecimals( curcs.zsamp_.step_*zfac );
	posbox->setInterval( curcs.zsamp_.start_*zfac,
			      curcs.zsamp_.stop_*zfac );
	stepbox->setInterval( survcs.zsamp_.step_*zfac,
			      (curcs.zsamp_.stop_-curcs.zsamp_.start_)*zfac,
			      curcs.zsamp_.step_*zfac );
	posbox->setNrDecimals( nrdec );
	sliceslider_->setInterval( curcs.zsamp_.start_*zfac,
				   curcs.zsamp_.stop_*zfac,
				   stepbox->getFValue() );
	stepbox->setNrDecimals( nrdec );
    }
}


void uiSlicePos::setPosBoxVal( uiSlicePos::SliceDir orientation,
			       const TrcKeyZSampling& tkzs )
{
    uiSpinBox* posbox = sliceposbox_;
    NotifyStopper posstop( posbox->valueChanging );

    if ( orientation == OD::SliceType::Inline )
	posbox->setValue( tkzs.hsamp_.start_.inl() );
    else if ( orientation == OD::SliceType::Crossline )
	posbox->setValue( tkzs.hsamp_.start_.crl() );
    else
    {
	const float zfac = mIsUdf(zfactor_) ? 1.f : zfactor_;
	posbox->setValue( tkzs.zsamp_.start_*zfac );
    }

    NotifyStopper ns( sliceslider_->sliderMoved );
    sliceslider_->setValue( posbox->getIntValue() );
}


void uiSlicePos::prevCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;

    OD::SliceType type = getOrientation();
    if ( type == OD::SliceType::Inline || type == OD::SliceType::Crossline )
	posbox->setValue( posbox->getIntValue()-stepbox->getIntValue() );
    else
	posbox->setValue( posbox->getFValue()-stepbox->getFValue() );

    NotifyStopper ns( sliceslider_->sliderMoved );
    sliceslider_->setValue( posbox->getIntValue() );
}


void uiSlicePos::nextCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;

    OD::SliceType type = getOrientation();
    if ( type == OD::SliceType::Inline || type == OD::SliceType::Crossline )
	posbox->setValue( posbox->getIntValue()+stepbox->getIntValue() );
    else
	posbox->setValue( posbox->getFValue()+stepbox->getFValue() );

    NotifyStopper ns( sliceslider_->sliderMoved );
    sliceslider_->setValue( posbox->getIntValue() );
}
