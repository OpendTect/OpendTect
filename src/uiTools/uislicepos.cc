/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uislicepos.h"

#include "uilabel.h"
#include "uishortcutsmgr.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "trckeyzsampling.h"
#include "ioman.h"
#include "survinfo.h"


uiSlicePos::uiSlicePos( uiParent* p )
    : positionChg(this)
    , zfactor_(mUdf(int))
{
    toolbar_ = new uiToolBar( p, uiStrings::phrJoinStrings(uiStrings::sSlice(),
			      uiStrings::sPosition()) );

    boxlabels_.add( uiStrings::sInline() );
    boxlabels_.add( uiStrings::sCrossline() );
    boxlabels_.add( uiStrings::sZ() );

    label_ = new uiLabel( toolbar_, boxlabels_[1] );
    sliceposbox_ = new uiSpinBox( toolbar_, 0, "Slice position" );
    sliceposbox_->valueChanging.notify( mCB(this,uiSlicePos,slicePosChg) );

    uiLabel* steplabel = new uiLabel( toolbar_, uiStrings::sStep() );

    slicestepbox_ = new uiSpinBox( toolbar_, 0, "Slice step" );
    slicestepbox_->valueChanged.notify( mCB(this,uiSlicePos,sliceStepChg) );
    slicestepbox_->valueChanging.notify( mCB(this,uiSlicePos,sliceStepChg) );

    prevbut_ = new uiToolButton( toolbar_, "prevpos", tr("Previous position"),
				mCB(this,uiSlicePos,prevCB) );
    nextbut_ = new uiToolButton( toolbar_, "nextpos", tr("Next position"),
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
}


void uiSlicePos::shortcutsChg( CallBacker* )
{
    const uiShortcutsList& scl = SCMgr().getList( "ODScene" );
    const uiKeyDesc* keydesc = scl.keyDescOf( "Move slice forward" );
    if ( keydesc )
	nextbut_->setShortcut( keydesc->getKeySequenceStr() );

    keydesc = scl.keyDescOf( "Move slice backward" );
    if ( keydesc )
	prevbut_->setShortcut( keydesc->getKeySequenceStr() );
}


void uiSlicePos::initSteps( CallBacker* )
{
    zfactor_ = mCast( int, SI().zDomain().userFactor() );
    laststeps_[0] = SI().inlStep();
    laststeps_[1] = SI().crlStep();
    laststeps_[2] = mNINT32( SI().zStep()*zfactor_ );
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
    if ( orientation == OD::InlineSlice )
	label_->setText( boxlabels_[0] );
    else if ( orientation == OD::CrosslineSlice )
	label_->setText( boxlabels_[1] );
    else
	label_->setText( boxlabels_[2] );
}


void uiSlicePos::updatePos( CallBacker* )
{
    setPosBoxValue();
}


void uiSlicePos::slicePosChanged( uiSlicePos::SliceDir orientation,
				  const TrcKeyZSampling& oldcs )
{
    uiSpinBox* posbox = sliceposbox_;
    curcs_ = oldcs;
    if ( orientation == OD::InlineSlice )
    {
	curcs_.hsamp_.start_.inl() =
		curcs_.hsamp_.stop_.inl() = posbox->getIntValue();
    }
    else if ( orientation == OD::CrosslineSlice )
    {
	curcs_.hsamp_.start_.crl() =
		curcs_.hsamp_.stop_.crl() = posbox->getIntValue();
    }
    else
	curcs_.zsamp_.start = curcs_.zsamp_.stop 
			    = posbox->getFValue()/zfactor_;

    if ( oldcs == curcs_ )
	return;

    positionChg.trigger();
}


void uiSlicePos::sliceStepChanged( uiSlicePos::SliceDir orientation )
{
    laststeps_[(int)orientation] = slicestepbox_->getIntValue();

    sliceposbox_->setStep( laststeps_[(int)orientation] );
}


void uiSlicePos::setBoxRg( uiSlicePos::SliceDir orientation,
				const TrcKeyZSampling& survcs )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    NotifyStopper posstop( posbox->valueChanging );
    NotifyStopper stepstop1( stepbox->valueChanged );
    NotifyStopper stepstop2( stepbox->valueChanging );

    if ( orientation == OD::InlineSlice )
    {
	posbox->setInterval( survcs.hsamp_.start_.inl(),
	survcs.hsamp_.stop_.inl() );
	stepbox->setInterval( survcs.hsamp_.step_.inl(),
		      survcs.hsamp_.stop_.inl()-survcs.hsamp_.start_.inl(),
		      survcs.hsamp_.step_.inl() );
    }
    else if ( orientation == OD::CrosslineSlice )
    {
	posbox->setInterval( survcs.hsamp_.start_.crl(),
		survcs.hsamp_.stop_.crl() );
	stepbox->setInterval( survcs.hsamp_.step_.crl(),
		survcs.hsamp_.stop_.crl()-survcs.hsamp_.start_.crl(),
		survcs.hsamp_.step_.crl() );
    }
    else
    {
	const int zfac = zfactor_;
	posbox->setInterval( survcs.zsamp_.start*zfac, survcs.zsamp_.stop*zfac);
	stepbox->setInterval( survcs.zsamp_.step*zfac,
			      (survcs.zsamp_.stop-survcs.zsamp_.start)*zfac,
			      survcs.zsamp_.step*zfac );
    }
}


void uiSlicePos::setPosBoxVal( uiSlicePos::SliceDir orientation,
				const TrcKeyZSampling& cs )
{
    uiSpinBox* posbox = sliceposbox_;
    NotifyStopper posstop( posbox->valueChanging );

    if ( orientation == OD::InlineSlice )
	posbox->setValue( cs.hsamp_.start_.inl() );
    else if ( orientation == OD::CrosslineSlice )
	posbox->setValue( cs.hsamp_.start_.crl() );
    else
	posbox->setValue( cs.zsamp_.start*zfactor_ );
}


void uiSlicePos::prevCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getIntValue()-stepbox->getIntValue() );
}


void uiSlicePos::nextCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getIntValue()+stepbox->getIntValue() );
}
