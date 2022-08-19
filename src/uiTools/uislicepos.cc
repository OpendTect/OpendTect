/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    laststeps_[2] = z>0 ? z : SI().zStep()*zfactor_;
}


float uiSlicePos::getZStep() const
{ return laststeps_[2]; }

void uiSlicePos::setZStep( float step )
{ laststeps_[2] = step>0 ? step : SI().zStep()*zfactor_; }


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
    laststeps_[(int)orientation] = slicestepbox_->getFValue();
    sliceposbox_->setStep( laststeps_[(int)orientation] );
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

    if ( orientation == OD::InlineSlice )
    {
	posbox->setInterval( curcs.hsamp_.start_.inl(),
		curcs.hsamp_.stop_.inl() );
	stepbox->setInterval( survcs.hsamp_.step_.inl(),
		curcs.hsamp_.stop_.inl()-curcs.hsamp_.start_.inl(),
		curcs.hsamp_.step_.inl() );
	posbox->setNrDecimals( 0 );
	stepbox->setNrDecimals( 0 );
    }
    else if ( orientation == OD::CrosslineSlice )
    {
	posbox->setInterval( curcs.hsamp_.start_.crl(),
		curcs.hsamp_.stop_.crl() );
	stepbox->setInterval( survcs.hsamp_.step_.crl(),
		curcs.hsamp_.stop_.crl()-curcs.hsamp_.start_.crl(),
		curcs.hsamp_.step_.crl() );
	posbox->setNrDecimals( 0 );
	stepbox->setNrDecimals( 0 );
    }
    else
    {
	const int zfac = zfactor_;
	const int nrdec = Math::NrSignificantDecimals( curcs.zsamp_.step*zfac );
	posbox->setInterval( curcs.zsamp_.start*zfac, curcs.zsamp_.stop*zfac);
	stepbox->setInterval( survcs.zsamp_.step*zfac,
		(curcs.zsamp_.stop-curcs.zsamp_.start)*zfac,
		curcs.zsamp_.step*zfac );
	posbox->setNrDecimals( nrdec );
	stepbox->setNrDecimals( nrdec );
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

    OD::SliceType type = getOrientation();
    if ( type == OD::InlineSlice || type == OD::CrosslineSlice )
	posbox->setValue( posbox->getIntValue()-stepbox->getIntValue() );
    else
	posbox->setValue( posbox->getFValue()-stepbox->getFValue() );
}


void uiSlicePos::nextCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;

    OD::SliceType type = getOrientation();
    if ( type == OD::InlineSlice || type == OD::CrosslineSlice )
	posbox->setValue( posbox->getIntValue()+stepbox->getIntValue() );
    else
	posbox->setValue( posbox->getFValue()+stepbox->getFValue() );
}
