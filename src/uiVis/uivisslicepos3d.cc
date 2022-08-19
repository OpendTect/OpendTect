/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivisslicepos3d.h"

#include "survinfo.h"
#include "uitoolbutton.h"
#include "uispinbox.h"
#include "visvolorthoslice.h"
#include "vissurvscene.h"
#include "uivispartserv.h"

#define Plane  visSurvey::PlaneDataDisplay
#define Volume visSurvey::VolumeDisplay
#define Object visSurvey::SurveyObject


uiSlicePos3DDisp::uiSlicePos3DDisp( uiParent* p, uiVisPartServer* server )
    : uiSlicePos(p)
    , vispartserv_(server)
{
    const bool isobj = curpdd_ || curvol_;
    sliceposbox_->setSensitive( isobj );
    slicestepbox_->setSensitive( isobj );
    prevbut_->setSensitive( isobj );
    nextbut_->setSensitive( isobj );
}


void uiSlicePos3DDisp::setDisplay( VisID dispid )
{
    CallBack movecb( mCB(this,uiSlicePos3DDisp,updatePos) );
    CallBack manipcb( mCB(this,uiSlicePos3DDisp,updatePos) );
    visSurvey::SurveyObject* prevso = curpdd_ ?
	(visSurvey::SurveyObject*)curpdd_ : (visSurvey::SurveyObject*)curvol_;
    if ( prevso )
    {
	prevso->getMovementNotifier()->remove( movecb );
	prevso->getManipulationNotifier()->remove( manipcb );
    }

    mDynamicCastGet(Object*,so,vispartserv_->getObject(dispid));
    mDynamicCastGet(Plane*,pdd,so);
    mDynamicCastGet(Volume*,vol,so);
    const bool isvalidso = ( pdd && pdd->isSelected() )
			    || ( vol && vol->getSelectedSlice() );

    sliceposbox_->setSensitive( isvalidso );
    slicestepbox_->setSensitive( isvalidso );
    prevbut_->setSensitive( isvalidso );
    nextbut_->setSensitive( isvalidso );

    curpdd_ = isvalidso ? pdd : nullptr;
    curvol_ = isvalidso ? vol : nullptr;
    if ( !isvalidso )
	return;

    so->getMovementNotifier()->notify( movecb );
    so->getManipulationNotifier()->notify( manipcb );

    zfactor_ = so->getScene() ? so->getScene()->zDomainUserFactor() : 1;
    setBoxLabel( getOrientation() );
    setBoxRanges();
    setPosBoxValue();
    setStepBoxValue();
}


VisID uiSlicePos3DDisp::getDisplayID() const
{
    return curpdd_ ? curpdd_->id() : curvol_ ? curvol_->id() : VisID::udf();
}


void uiSlicePos3DDisp::setBoxRanges()
{
    if ( !curpdd_ && !curvol_ ) return;

    TrcKeyZSampling curcs( false );
    if ( curpdd_ && curpdd_->getScene() )
	curcs = curpdd_->getScene()->getTrcKeyZSampling();
    else if ( curvol_ )
	curcs = curvol_->getTrcKeyZSampling( false );
    else
	curcs = SI().sampling( true );
    const TrcKeyZSampling survcs( curcs );

    const auto orient = getOrientation();
    if ( orient == OD::ZSlice )
    {
	SamplingData<float> sd( sliceposbox_->getFValue() / zfactor_,
				slicestepbox_->getFValue() / zfactor_ );
	if ( !mIsZero(sd.step,1e-6f) )
	{
	    auto& zsamp = curcs.zsamp_;
	    const auto startidx = sd.indexOnOrAfter( zsamp.start );
	    sd.start = zsamp.start = sd.atIndex( startidx );
	    zsamp.stop = sd.atIndex( sd.nrSteps(zsamp.stop) );
	}
    }
    else
    {
	SamplingData<int> sd( sliceposbox_->getIntValue(),
			      slicestepbox_->getIntValue() );
	if ( sd.step != 0 )
	{
	    auto& hsamp = curcs.hsamp_;
	    if ( orient == OD::InlineSlice )
	    {
		const auto startidx = sd.indexOnOrAfter( hsamp.start_.inl() );
		sd.start = hsamp.start_.inl() = sd.atIndex( startidx );
		hsamp.stop_.inl() = sd.atIndex( sd.nrSteps(hsamp.stop_.inl()) );
	    }
	    else
	    {
		const auto idx = sd.indexOnOrAfter( hsamp.start_.crl() );
		sd.start = hsamp.start_.crl() = sd.atIndex(idx);
		hsamp.stop_.crl() = sd.atIndex( sd.nrSteps(hsamp.stop_.crl()) );
	    }
	}
    }

    setBoxRg( orient, curcs, survcs );
}


void uiSlicePos3DDisp::setPosBoxValue()
{
    if ( !curpdd_ && !curvol_ ) return;

    setPosBoxVal( getOrientation(), getSampling() );
}


void uiSlicePos3DDisp::setStepBoxValue()
{
    if ( !curpdd_ && !curvol_ ) return;

    const uiSlicePos::SliceDir orientation = getOrientation();
    slicestepbox_->setValue( laststeps_[(int)orientation] );
    sliceStepChg( nullptr );
}


void uiSlicePos3DDisp::slicePosChg( CallBacker* )
{
    if ( !curpdd_ && !curvol_ ) return;

    slicePosChanged( getOrientation(), getSampling() );
}


void uiSlicePos3DDisp::sliceStepChg( CallBacker* )
{
    if ( !curpdd_ && !curvol_ ) return;

    setBoxRanges();
    sliceStepChanged( getOrientation() );
}


OD::SliceType uiSlicePos3DDisp::getOrientation() const
{
    if ( curpdd_ )
	return curpdd_->getOrientation();

    else if ( curvol_ && curvol_->getSelectedSlice() )
    {
	const int dim = curvol_->getSelectedSlice()->getDim();
	if ( dim == Volume::cInLine() )
	    return OD::InlineSlice;
	else if ( dim == Volume::cCrossLine() )
	    return OD::CrosslineSlice;
	else if ( dim == Volume::cTimeSlice() )
	    return OD::ZSlice;
    }
    return OD::InlineSlice;
}


TrcKeyZSampling uiSlicePos3DDisp::getSampling() const
{
    return curpdd_ ? curpdd_->getTrcKeyZSampling( true, true )
		   : curvol_->sliceSampling( curvol_->getSelectedSlice() );
}
