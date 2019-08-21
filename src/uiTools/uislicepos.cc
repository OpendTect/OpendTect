/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		April 2009
________________________________________________________________________

-*/

#include "uislicepos.h"

#include "uilabel.h"
#include "uishortcutsmgr.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "trckeyzsampling.h"
#include "dbman.h"
#include "survinfo.h"


uiSlicePos::uiSlicePos( uiParent* p )
    : positionChg(this)
    , curcs_(!DBM().isBad())
    , zfactor_(mUdf(int))
{
    toolbar_ = new uiToolBar( p, tr("Slice Position") );

    boxlabels_.add( uiStrings::sInline() );
    boxlabels_.add( uiStrings::sCrossline() );
    boxlabels_.add( uiStrings::sZ() );

    label_ = new uiLabel( toolbar_, boxlabels_[1] );
    sliceposbox_ = new uiSpinBox( toolbar_, 0, "Slice position" );
    sliceposbox_->valueChanging.notify( mCB(this,uiSlicePos,slicePosChgCB) );

    uiLabel* steplabel = new uiLabel( toolbar_, uiStrings::sStep() );

    slicestepbox_ = new uiSpinBox( toolbar_, 0, "Slice step" );
    slicestepbox_->valueChanging.notify( mCB(this,uiSlicePos,sliceStepChgCB) );

    prevbut_ = new uiToolButton( toolbar_, "prevpos", tr("Previous position"),
				mCB(this,uiSlicePos,prevCB) );
    nextbut_ = new uiToolButton( toolbar_, "nextpos", tr("Next position"),
				mCB(this,uiSlicePos,nextCB) );

    toolbar_->addObject( label_, 2 );
    toolbar_->addObject( sliceposbox_, 2 );
    toolbar_->addObject( steplabel, 2 );
    toolbar_->addObject( slicestepbox_, 2 );
    toolbar_->addObject( prevbut_, 1 );
    toolbar_->addObject( nextbut_, 1 );

    DBM().surveyChanged.notify( mCB(this,uiSlicePos,initSteps) );

    SCMgr().shortcutsChanged.notify( mCB(this,uiSlicePos,shortcutsChg) );
    initSteps();
    shortcutsChg( 0 );
}


uiSlicePos::~uiSlicePos()
{
    DBM().surveyChanged.remove( mCB(this,uiSlicePos,initSteps) );
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
    if ( DBM().isBad() )
	return;

    zfactor_ = mCast(int,SI().zDomain().userFactor());
    laststeps_[0] = SI().inlStep();
    laststeps_[1] = SI().crlStep();
    laststeps_[2] = mNINT32( SI().zStep()*zfactor_ );
}


int uiSlicePos::getStep( SliceDir dir ) const
{
    return laststeps_[ (int)dir ];
}


void uiSlicePos::setStep( SliceDir dir, int step )
{
    laststeps_[ (int)dir ] = step;
}


void uiSlicePos::setSteps( int inl, int crl, int z )
{
    laststeps_[0] = inl>0 ? inl : SI().inlStep();
    laststeps_[1] = crl>0 ? crl : SI().crlStep();
    laststeps_[2] = z>0 ? z : mNINT32( SI().zStep()*zfactor_ );
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


void uiSlicePos::slicePosChgCB( CallBacker* )
{
    handleSlicePosChg();
}


void uiSlicePos::stdHandleSlicePosChg( uiSlicePos::SliceDir orientation,
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


void uiSlicePos::sliceStepChgCB( CallBacker* )
{
    handleSliceStepChg();
}


void uiSlicePos::stdHandleSliceStepChg( uiSlicePos::SliceDir orientation )
{
    laststeps_[(int)orientation] = slicestepbox_->getIntValue();

    sliceposbox_->setStep( laststeps_[(int)orientation] );
}


void uiSlicePos::setBoxRg( uiSlicePos::SliceDir orientation,
				const TrcKeyZSampling& curcs,
				const TrcKeyZSampling& survcs )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    NotifyStopper stepstop( stepbox->valueChanging );

    if ( orientation == OD::InlineSlice )
    {
	posbox->setInterval( curcs.hsamp_.start_.inl(),
	curcs.hsamp_.stop_.inl() );
	stepbox->setInterval( survcs.hsamp_.step_.inl(),
		      curcs.hsamp_.stop_.inl()-curcs.hsamp_.start_.inl(),
		      curcs.hsamp_.step_.inl() );
    }
    else if ( orientation == OD::CrosslineSlice )
    {
	posbox->setInterval( curcs.hsamp_.start_.crl(),
		curcs.hsamp_.stop_.crl() );
	stepbox->setInterval( survcs.hsamp_.step_.crl(),
		curcs.hsamp_.stop_.crl()-curcs.hsamp_.start_.crl(),
		curcs.hsamp_.step_.crl() );
    }
    else
    {
	const int zfac = zfactor_;
	posbox->setInterval( curcs.zsamp_.start*zfac, curcs.zsamp_.stop*zfac);
	stepbox->setInterval( survcs.zsamp_.step*zfac,
			      (curcs.zsamp_.stop-curcs.zsamp_.start)*zfac,
			      curcs.zsamp_.step*zfac );
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
