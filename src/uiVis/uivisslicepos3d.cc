/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisslicepos3d.cc,v 1.21 2011-10-10 08:34:56 cvsjaap Exp $";

#include "uivisslicepos3d.h"

#include "survinfo.h"
#include "uitoolbutton.h"
#include "uispinbox.h"
#include "visplanedatadisplay.h"
#include "vissurvscene.h"

#define Display visSurvey::PlaneDataDisplay


uiSlicePos3DDisp::uiSlicePos3DDisp( uiParent* p )
    : uiSlicePos( p )
    , curpdd_(0)
{
    sliceposbox_->setSensitive( curpdd_ );
    slicestepbox_->setSensitive( curpdd_ );
    prevbut_->setSensitive( curpdd_ );
    nextbut_->setSensitive( curpdd_ );
}


void uiSlicePos3DDisp::setDisplay( Display* pdd )
{
    if ( curpdd_ )
    {
	curpdd_->getMovementNotifier()->remove(
					mCB(this,uiSlicePos3DDisp,updatePos) );
	curpdd_->getManipulationNotifier()->remove(
					mCB(this,uiSlicePos3DDisp,updatePos) );
	curpdd_->unRef();
    }
    curpdd_ = pdd;
    if ( curpdd_ )
    {
	curpdd_->ref();
	curpdd_->getMovementNotifier()->notify(
					mCB(this,uiSlicePos3DDisp,updatePos) );
	curpdd_->getManipulationNotifier()->notify(
					mCB(this,uiSlicePos3DDisp,updatePos) );
    }

    sliceposbox_->setSensitive( curpdd_ );
    slicestepbox_->setSensitive( curpdd_ );
    prevbut_->setSensitive( curpdd_ );
    nextbut_->setSensitive( curpdd_ );

    if ( !curpdd_ ) return;

    zfactor_ = curpdd_->getScene()
	? curpdd_->getScene()->zDomainUserFactor() : 1;
    setBoxLabel( (uiSlicePos::Orientation) curpdd_->getOrientation() );
    setBoxRanges();
    setPosBoxValue();
    setStepBoxValue();
}


int uiSlicePos3DDisp::getDisplayID() const
{
    return curpdd_ ? curpdd_->id() : -1;
}


void uiSlicePos3DDisp::setBoxRanges()
{
    if ( !curpdd_ ) return;

    const CubeSampling& survey = curpdd_->getScene() ?
	curpdd_->getScene()->getCubeSampling() : SI().sampling( true );
    const Display::Orientation orientation = curpdd_->getOrientation();
    setBoxRg( (uiSlicePos::Orientation) orientation, survey );
}


void uiSlicePos3DDisp::setPosBoxValue()
{
    if ( !curpdd_ ) return;

    const CubeSampling cs = curpdd_->getCubeSampling( true, true );
    const Display::Orientation orientation = curpdd_->getOrientation();
    setPosBoxVal( (uiSlicePos::Orientation) orientation, cs );
}


void uiSlicePos3DDisp::setStepBoxValue()
{
    if ( !curpdd_ ) return;

    const Display::Orientation orientation = curpdd_->getOrientation();
    slicestepbox_->setValue( laststeps_[(int)orientation] );
    sliceStepChg( 0 );
}


void uiSlicePos3DDisp::slicePosChg( CallBacker* )
{
    if ( !curpdd_ ) return;

    CubeSampling oldcs = curpdd_->getCubeSampling( true, true );
    const Display::Orientation orientation = curpdd_->getOrientation();
    slicePosChanged( (uiSlicePos::Orientation) orientation, oldcs );
}


void uiSlicePos3DDisp::sliceStepChg( CallBacker* )
{
    if ( !curpdd_ ) return;

    const Display::Orientation orientation = curpdd_->getOrientation();
    sliceStepChanged( (uiSlicePos::Orientation) orientation );
}
