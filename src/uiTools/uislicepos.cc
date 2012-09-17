/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uislicepos.cc,v 1.18 2012/07/10 13:06:08 cvskris Exp $";

#include "uislicepos.h"

#include "uilabel.h"
#include "uishortcutsmgr.h"
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

    boxlabels_.add( "Inl" );
    boxlabels_.add( "Crl" );
    boxlabels_.add( "Z" );

    label_ = new uiLabel( toolbar_, boxlabels_.get(1) );
    sliceposbox_ = new uiSpinBox( toolbar_, 0, "Slice position" );
    sliceposbox_->valueChanging.notify( mCB(this,uiSlicePos,slicePosChg) );

    uiLabel* steplabel = new uiLabel( toolbar_, "Step" );

    slicestepbox_ = new uiSpinBox( toolbar_, 0, "Slice step" );
    slicestepbox_->valueChanged.notify( mCB(this,uiSlicePos,sliceStepChg) );
    slicestepbox_->valueChanging.notify( mCB(this,uiSlicePos,sliceStepChg) );

    prevbut_ = new uiToolButton( toolbar_, "prevpos.png", "Previous position",
				mCB(this,uiSlicePos,prevCB) );
    nextbut_ = new uiToolButton( toolbar_, "nextpos.png", "Next position",
				mCB(this,uiSlicePos,nextCB) );

    toolbar_->addObject( label_ );
    toolbar_->addObject( sliceposbox_ );
    toolbar_->addObject( steplabel );
    toolbar_->addObject( slicestepbox_ );
    toolbar_->addObject( prevbut_ );
    toolbar_->addObject( nextbut_ );

    IOM().surveyChanged.notify( mCB(this,uiSlicePos,initSteps) );
    SCMgr().shortcutsChanged.notify( mCB(this,uiSlicePos,shortcutsChg) );
    initSteps();
    shortcutsChg( 0 );
}


uiSlicePos::~uiSlicePos()
{
    IOM().surveyChanged.remove( mCB(this,uiSlicePos,initSteps) );
    SCMgr().shortcutsChanged.remove( mCB(this,uiSlicePos,shortcutsChg) );
    delete toolbar_;
}


void uiSlicePos::shortcutsChg( CallBacker* )
{
    const uiShortcutsList& scl = SCMgr().getList( "ODScene" );
    const uiKeyDesc* keydesc = scl.keyDescOf( "Move slice forward" );
    if ( keydesc )
    {
	BufferString keyseq;
	if ( keydesc->state() == OD::ControlButton )
	    keyseq += "Ctrl+";
	else if ( keydesc->state() == OD::ShiftButton )
	    keyseq += "Shift+";

	keyseq += keydesc->keyStr();
	nextbut_->setShortcut( keyseq.buf() );
    }

    keydesc = scl.keyDescOf( "Move slice backward" );
    if ( keydesc )
    {
	BufferString keyseq;
	if ( keydesc->state() == OD::ControlButton )
	    keyseq += "Ctrl+";
	else if ( keydesc->state() == OD::ShiftButton )
	    keyseq += "Shift+";

	keyseq += keydesc->keyStr();
	prevbut_->setShortcut( keyseq.buf() );
    }
}


void uiSlicePos::initSteps( CallBacker* )
{
    laststeps_[0] = SI().inlStep();
    laststeps_[1] = SI().crlStep();
    laststeps_[2] = mNINT32( SI().zStep()*zfactor_ );
}


void uiSlicePos::setLabels( const char* inl, const char* crl, const char* z )
{
    boxlabels_.get(0) = inl;
    boxlabels_.get(1) = crl;
    boxlabels_.get(2) = z;
}


void uiSlicePos::setBoxLabel( Orientation orientation )
{
    if ( orientation == uiSlicePos::Inline )
	label_->setText( boxlabels_.get(0) );
    else if ( orientation == uiSlicePos::Crossline )
	label_->setText( boxlabels_.get(1) );
    else
	label_->setText( boxlabels_.get(2) );
}


void uiSlicePos::updatePos( CallBacker* )
{
    setPosBoxValue();
}


void uiSlicePos::slicePosChanged( Orientation orientation,
				  const CubeSampling& oldcs )
{
    uiSpinBox* posbox = sliceposbox_;
    curcs_ = oldcs;
    if ( orientation == uiSlicePos::Inline )
	curcs_.hrg.start.inl = curcs_.hrg.stop.inl = posbox->getValue();
    else if ( orientation == uiSlicePos::Crossline )
	curcs_.hrg.start.crl = curcs_.hrg.stop.crl = posbox->getValue();
    else
	curcs_.zrg.start = curcs_.zrg.stop = (float)posbox->getValue()/zfactor_;

    if ( oldcs == curcs_ )
	return;

    positionChg.trigger();
}


void uiSlicePos::sliceStepChanged( Orientation orientation )
{
    laststeps_[(int)orientation] = slicestepbox_->getValue();

    sliceposbox_->setStep( laststeps_[(int)orientation] );
}


void uiSlicePos::setBoxRg( Orientation orientation, const CubeSampling& survcs )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
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
	const int zfac = zfactor_;
	posbox->setInterval( survcs.zrg.start*zfac, survcs.zrg.stop*zfac);
	stepbox->setInterval( survcs.zrg.step*zfac,
			      (survcs.zrg.stop-survcs.zrg.start)*zfac,
			      survcs.zrg.step*zfac );
    }
}


void uiSlicePos::setPosBoxVal( Orientation orientation, const CubeSampling& cs )
{
    uiSpinBox* posbox = sliceposbox_;
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
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getValue()-stepbox->getValue() );
}


void uiSlicePos::nextCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getValue()+stepbox->getValue() );
}
