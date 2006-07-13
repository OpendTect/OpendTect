/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2006
 RCS:		$Id: uivisslicepos3d.cc,v 1.1 2006-07-13 20:18:51 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uislicepos.h"

#include "uispinbox.h"
#include "uitoolbar.h"
#include "visplanedatadisplay.h"

#include "cubesampling.h"
#include "survinfo.h"


#define Display visSurvey::PlaneDataDisplay

uiSlicePos::uiSlicePos( uiParent* p )
    : toolbar_(new uiToolBar(p,"Slice position controls"))
    , curpdd_(0)
{
    toolbar_->setCloseMode( 2 );

    sliceposbox_ = new uiSpinBox( toolbar_, 0, "Slice position" );
    sliceposbox_->setSensitive( curpdd_ );
    sliceposbox_->valueChanged.notify( mCB(this,uiSlicePos,slicePosChg) );

    slicestepbox_ = new uiSpinBox( toolbar_, 0, "Slice step" );
    slicestepbox_->setSensitive( curpdd_ );
    slicestepbox_->valueChanged.notify( mCB(this,uiSlicePos,sliceStepChg) );

    laststeps_[0] = SI().inlStep();
    laststeps_[1] = SI().crlStep();
    laststeps_[2] = mNINT( SI().zStep()*SI().zFactor() );
}


uiSlicePos::~uiSlicePos()
{
    delete toolbar_;
}


void uiSlicePos::setDisplay( Display* pdd )
{
    if ( curpdd_ )
    {
	curpdd_->getManipulationNotifier()->remove(
					mCB(this,uiSlicePos,updatePos) );
	curpdd_->unRef();
    }
    curpdd_ = pdd;
    if ( curpdd_ )
    {
	curpdd_->ref();
	curpdd_->getManipulationNotifier()->notify(
					mCB(this,uiSlicePos,updatePos) );
    }

    sliceposbox_->setSensitive( curpdd_ );
    slicestepbox_->setSensitive( curpdd_ );

    if ( !curpdd_ ) return;

    setBoxRanges();
    setPosBoxValue();
    setStepBoxValue();
}


void uiSlicePos::updatePos( CallBacker* )
{
    setPosBoxValue();
}


void uiSlicePos::setPosBoxValue()
{
    if ( !curpdd_ ) return;

    NotifyStopper posstop( sliceposbox_->valueChanged );

    const CubeSampling cs = curpdd_->getCubeSampling();
    const Display::Orientation orientation = curpdd_->getOrientation();
    if ( orientation == Display::Inline )
	sliceposbox_->setValue( cs.hrg.start.inl );
    else if ( orientation == Display::Crossline )
	sliceposbox_->setValue( cs.hrg.start.crl );
    else
	sliceposbox_->setValue( cs.zrg.start*SI().zFactor() );
}


void uiSlicePos::setStepBoxValue()
{
    const Display::Orientation orientation = curpdd_->getOrientation();
    slicestepbox_->setValue( laststeps_[(int)orientation] );
}


void uiSlicePos::setBoxRanges()
{
    if ( !curpdd_ ) return;

    NotifyStopper posstop( sliceposbox_->valueChanged );
    NotifyStopper stepstop( slicestepbox_->valueChanged );

    const CubeSampling& survey = SI().sampling( true );
    const Display::Orientation orientation = curpdd_->getOrientation();
    if ( orientation == Display::Inline )
    {
	sliceposbox_->setInterval( survey.hrg.start.inl, survey.hrg.stop.inl );
	slicestepbox_->setInterval( survey.hrg.step.inl,
				    survey.hrg.stop.inl-survey.hrg.start.inl );
    }
    else if ( orientation == Display::Crossline )
    {
	sliceposbox_->setInterval( survey.hrg.start.crl, survey.hrg.stop.crl,
				   survey.hrg.step.crl );
	slicestepbox_->setInterval( survey.hrg.step.crl,
				    survey.hrg.stop.crl-survey.hrg.start.crl );
    }
    else
    {
	const float zfac = SI().zFactor();
	sliceposbox_->setInterval( survey.zrg.start*zfac, survey.zrg.stop*zfac);
	slicestepbox_->setInterval( survey.zrg.step*zfac,
				    (survey.zrg.stop-survey.zrg.start)*zfac );
    }
}


void uiSlicePos::slicePosChg( CallBacker* )
{
    if ( !curpdd_ ) return;

    CubeSampling cs = curpdd_->getCubeSampling();
    const Display::Orientation orientation = curpdd_->getOrientation();
    if ( orientation == Display::Inline )
	cs.hrg.start.inl = cs.hrg.stop.inl = sliceposbox_->getValue();
    else if ( orientation == Display::Crossline )
	cs.hrg.start.crl = cs.hrg.stop.crl = sliceposbox_->getValue();
    else
	cs.zrg.start = cs.zrg.stop = sliceposbox_->getValue()/SI().zFactor();

    curpdd_->setCubeSampling( cs );
}


void uiSlicePos::sliceStepChg( CallBacker* )
{
    if ( !curpdd_ ) return;

    const Display::Orientation orientation = curpdd_->getOrientation();
    laststeps_[(int)orientation] = slicestepbox_->getValue();

    sliceposbox_->setStep( laststeps_[(int)orientation] );
}
