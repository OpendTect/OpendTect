/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uislicepos.cc,v 1.9 2010-11-16 09:49:11 cvsbert Exp $";

#include "uislicepos.h"

#include "uilabel.h"
#include "uispinbox.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "cubesampling.h"
#include "ioman.h"
#include "pixmap.h"
#include "survinfo.h"

uiSlicePos::uiSlicePos( uiParent* p )
    : positionChg(this)
    , zfactor_(1)
{
    toolbar_ = new uiToolBar( p, "Slice position" );

    sliceposbox_ = new uiLabeledSpinBox( toolbar_, "Crl", 0,
	    				 "Slice position" );
    sliceposbox_->box()->valueChanging.notify(
				mCB(this,uiSlicePos,slicePosChg) );

    slicestepbox_ = new uiLabeledSpinBox( toolbar_, "Step", 0, "Slice step" );
    slicestepbox_->box()->valueChanged.notify(
				mCB(this,uiSlicePos,sliceStepChg) );
    slicestepbox_->box()->valueChanging.notify(
				mCB(this,uiSlicePos,sliceStepChg) );
    slicestepbox_->box()->disabFocus();

    prevbut_ = new uiToolButton( toolbar_, "prevpos.png", "Previous position",
				mCB(this,uiSlicePos,prevCB) );
    nextbut_ = new uiToolButton( toolbar_, "nextpos.png", "Next position",
				mCB(this,uiSlicePos,nextCB) );

    toolbar_->addObject( sliceposbox_->label() );
    toolbar_->addObject( sliceposbox_->box() );
    toolbar_->addObject( slicestepbox_->label() );
    toolbar_->addObject( slicestepbox_->box() );
    toolbar_->addObject( prevbut_ );
    toolbar_->addObject( nextbut_ );

    IOM().surveyChanged.notify( mCB(this,uiSlicePos,initSteps) );
    initSteps();
}


uiSlicePos::~uiSlicePos()
{
    IOM().surveyChanged.remove( mCB(this,uiSlicePos,initSteps) );
    delete toolbar_;
}


void uiSlicePos::initSteps( CallBacker* )
{
    laststeps_[0] = SI().inlStep();
    laststeps_[1] = SI().crlStep();
    laststeps_[2] = mNINT( SI().zStep()*zfactor_ );
}


void uiSlicePos::setBoxLabel( Orientation orientation )
{
    if ( orientation == uiSlicePos::Inline )
	sliceposbox_->label()->setText( "Inl" );
    else if ( orientation == uiSlicePos::Crossline )
	sliceposbox_->label()->setText( "Crl" );
    else
	sliceposbox_->label()->setText( "Z" );
}


void uiSlicePos::updatePos( CallBacker* )
{
    setPosBoxValue();
}


void uiSlicePos::slicePosChanged( Orientation orientation,
				  const CubeSampling& oldcs )
{
    uiSpinBox* posbox = sliceposbox_->box();
    curcs_ = oldcs;
    if ( orientation == uiSlicePos::Inline )
	curcs_.hrg.start.inl = curcs_.hrg.stop.inl = posbox->getValue();
    else if ( orientation == uiSlicePos::Crossline )
	curcs_.hrg.start.crl = curcs_.hrg.stop.crl = posbox->getValue();
    else
	curcs_.zrg.start = curcs_.zrg.stop = posbox->getValue()/zfactor_;

    if ( oldcs == curcs_ )
	return;

    positionChg.trigger();
}


void uiSlicePos::sliceStepChanged( Orientation orientation )
{
    laststeps_[(int)orientation] = slicestepbox_->box()->getValue();

    sliceposbox_->box()->setStep( laststeps_[(int)orientation] );
}


void uiSlicePos::setBoxRg( Orientation orientation, const CubeSampling& survcs )
{
    uiSpinBox* posbox = sliceposbox_->box();
    uiSpinBox* stepbox = slicestepbox_->box();
    NotifyStopper posstop( posbox->valueChanging );
    NotifyStopper stepstop( stepbox->valueChanged );

    if ( orientation == uiSlicePos::Inline )
    {
	posbox->setInterval( survcs.hrg.start.inl, survcs.hrg.stop.inl );
	stepbox->setInterval( survcs.hrg.step.inl,
			      survcs.hrg.stop.inl-survcs.hrg.start.inl,
			      survcs.hrg.step.inl );
    }
    else if ( orientation == uiSlicePos::Crossline )
    {
	posbox->setInterval( survcs.hrg.start.crl, survcs.hrg.stop.crl );
	stepbox->setInterval( survcs.hrg.step.crl,
			      survcs.hrg.stop.crl-survcs.hrg.start.crl,
			      survcs.hrg.step.crl );
    }
    else
    {
	const float zfac = zfactor_;
	posbox->setInterval( survcs.zrg.start*zfac, survcs.zrg.stop*zfac);
	stepbox->setInterval( survcs.zrg.step*zfac,
			      (survcs.zrg.stop-survcs.zrg.start)*zfac,
			      survcs.zrg.step*zfac );
    }
}


void uiSlicePos::setPosBoxVal( Orientation orientation, const CubeSampling& cs )
{
    uiSpinBox* posbox = sliceposbox_->box();
    NotifyStopper posstop( posbox->valueChanging );

    if ( orientation == uiSlicePos::Inline )
	posbox->setValue( cs.hrg.start.inl );
    else if ( orientation == uiSlicePos::Crossline )
	posbox->setValue( cs.hrg.start.crl );
    else
	posbox->setValue( cs.zrg.start*zfactor_ );
}


void uiSlicePos::prevCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_->box();
    uiSpinBox* stepbox = slicestepbox_->box();
    posbox->setValue( posbox->getValue()-stepbox->getValue() );
}


void uiSlicePos::nextCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_->box();
    uiSpinBox* stepbox = slicestepbox_->box();
    posbox->setValue( posbox->getValue()+stepbox->getValue() );
}
