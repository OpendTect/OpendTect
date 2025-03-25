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
#include "uivispartserv.h"
#include "vissurvscene.h"
#include "visvolorthoslice.h"


uiSlicePos3DDisp::uiSlicePos3DDisp( uiParent* p, uiVisPartServer* server )
    : uiSlicePos(p)
    , vispartserv_(server)
{
    sliceposbox_->setSensitive( false );
    slicestepbox_->setSensitive( false );
    prevbut_->setSensitive( false );
    nextbut_->setSensitive( false );
}


uiSlicePos3DDisp::~uiSlicePos3DDisp()
{
    detachAllNotifiers();
}


bool uiSlicePos3DDisp::isOK() const
{
    return curpdd_ || curvol_;
}


void uiSlicePos3DDisp::setDisplay( const VisID& dispid )
{
    CallBack movecb( mCB(this,uiSlicePos3DDisp,updatePos) );
    CallBack manipcb( mCB(this,uiSlicePos3DDisp,updatePos) );
    RefMan<visSurvey::PlaneDataDisplay> curpdd = curpdd_.get();
    RefMan<visSurvey::VolumeDisplay> curvol = curvol_.get();
    visSurvey::SurveyObject* prevso = curpdd
		    ? (visSurvey::SurveyObject*)curpdd.ptr()
		    : (visSurvey::SurveyObject*)curvol.ptr();
    if ( prevso )
    {
	mDetachCB( *prevso->getMovementNotifier(), uiSlicePos3DDisp::updatePos);
	mDetachCB( *prevso->getManipulationNotifier(),
		   uiSlicePos3DDisp::updatePos );
    }

    mDynamicCastGet(visSurvey::SurveyObject*,so,
		    vispartserv_->getObject(dispid));
    curpdd = dCast(visSurvey::PlaneDataDisplay*,so);
    curvol = dCast(visSurvey::VolumeDisplay*,so);
    const bool isvalidso = ( curpdd && curpdd->isSelected() ) ||
			   ( curvol && curvol->getSelectedSlice() );

    sliceposbox_->setSensitive( isvalidso );
    slicestepbox_->setSensitive( isvalidso );
    prevbut_->setSensitive( isvalidso );
    nextbut_->setSensitive( isvalidso );

    curpdd_ = isvalidso ? curpdd.ptr() : nullptr;
    curvol_ = isvalidso ? curvol.ptr() : nullptr;
    if ( !isvalidso )
	return;

    mAttachCB( *so->getMovementNotifier(), uiSlicePos3DDisp::updatePos );
    mAttachCB( *so->getManipulationNotifier(), uiSlicePos3DDisp::updatePos );

    zfactor_ = 1;
    if ( so->getScene() )
    {
	const ZDomain::Info& zdinf = so->getScene()->zDomainInfo();
	const ZDomain::Info* dispzdinf = nullptr;
	if ( zdinf.isDepth() )
	    dispzdinf = &ZDomain::DefaultDepth();

	zfactor_ = FlatView::Viewer::userFactor( zdinf, dispzdinf );
    }

    setBoxLabel( getOrientation() );
    setBoxRanges();
    setPosBoxValue();
    setStepBoxValue();
}


VisID uiSlicePos3DDisp::getDisplayID() const
{
    if ( !isOK() )
	return VisID::udf();

    ConstRefMan<visSurvey::PlaneDataDisplay> curpdd = curpdd_.get();
    ConstRefMan<visSurvey::VolumeDisplay> curvol = curvol_.get();
    return curpdd ? curpdd->id() : curvol ? curvol->id() : VisID::udf();
}


void uiSlicePos3DDisp::setBoxRanges()
{
    RefMan<visSurvey::PlaneDataDisplay> curpdd = curpdd_.get();
    RefMan<visSurvey::VolumeDisplay> curvol = curvol_.get();
    if ( !curpdd && !curvol )
	return;

    TrcKeyZSampling curcs;
    if ( curpdd && curpdd->getScene() )
	curcs = curpdd->getScene()->getTrcKeyZSampling();
    else if ( curvol )
	curcs = curvol->getTrcKeyZSampling( false, 0 );
    else
	curcs = SI().sampling( true );

    const TrcKeyZSampling survcs( curcs );
    const auto orient = getOrientation();
    if ( orient == OD::SliceType::Z )
    {
	SamplingData<float> sd( sliceposbox_->getFValue() / zfactor_,
				slicestepbox_->getFValue() / zfactor_ );
	if ( !mIsZero(sd.step_,1e-6f) )
	{
	    auto& zsamp = curcs.zsamp_;
	    const auto startidx = sd.indexOnOrAfter( zsamp.start_ );
	    sd.start_ = zsamp.start_ = sd.atIndex( startidx );
	    zsamp.stop_ = sd.atIndex( sd.nrSteps(zsamp.stop_) );
	}
    }
    else
    {
	SamplingData<int> sd( sliceposbox_->getIntValue(),
			      slicestepbox_->getIntValue() );
	if ( sd.step_ != 0 )
	{
	    auto& hsamp = curcs.hsamp_;
	    if ( orient == OD::SliceType::Inline )
	    {
		const auto startidx = sd.indexOnOrAfter( hsamp.start_.inl() );
		sd.start_ = hsamp.start_.inl() = sd.atIndex( startidx );
		hsamp.stop_.inl() = sd.atIndex( sd.nrSteps(hsamp.stop_.inl()) );
	    }
	    else
	    {
		const auto idx = sd.indexOnOrAfter( hsamp.start_.crl() );
		sd.start_ = hsamp.start_.crl() = sd.atIndex(idx);
		hsamp.stop_.crl() = sd.atIndex( sd.nrSteps(hsamp.stop_.crl()) );
	    }
	}
    }

    setBoxRg( orient, curcs, survcs );
}


void uiSlicePos3DDisp::setPosBoxValue()
{
    if ( !isOK() )
	return;

    setPosBoxVal( getOrientation(), getSampling() );
}


void uiSlicePos3DDisp::setStepBoxValue()
{
    if ( !isOK() )
	return;

    const uiSlicePos::SliceDir orientation = getOrientation();
    slicestepbox_->setValue( laststeps_[(int)orientation] );
    sliceStepChg( nullptr );
}


void uiSlicePos3DDisp::slicePosChg( CallBacker* )
{
    if ( !isOK() )
	return;

    slicePosChanged( getOrientation(), getSampling() );
}


void uiSlicePos3DDisp::sliceStepChg( CallBacker* )
{
    if ( !isOK() )
	return;

    setBoxRanges();
    sliceStepChanged( getOrientation() );
}


OD::SliceType uiSlicePos3DDisp::getOrientation() const
{
    ConstRefMan<visSurvey::PlaneDataDisplay> curpdd = curpdd_.get();
    ConstRefMan<visSurvey::VolumeDisplay> curvol = curvol_.get();
    if ( curpdd )
	return curpdd->getOrientation();
    else if ( curvol && curvol->getSelectedSlice() )
    {
	const int dim = curvol->getSelectedSlice()->getDim();
	if ( dim == visSurvey::VolumeDisplay::cInLine() )
	    return OD::SliceType::Inline;
	else if ( dim == visSurvey::VolumeDisplay::cCrossLine() )
	    return OD::SliceType::Crossline;
	else if ( dim == visSurvey::VolumeDisplay::cTimeSlice() )
	    return OD::SliceType::Z;
    }

    return OD::SliceType::Inline;
}


TrcKeyZSampling uiSlicePos3DDisp::getSampling() const
{
    ConstRefMan<visSurvey::PlaneDataDisplay> curpdd = curpdd_.get();
    ConstRefMan<visSurvey::VolumeDisplay> curvol = curvol_.get();
    return curpdd ? curpdd->getTrcKeyZSampling( true, true )
		  : curvol->sliceSampling( curvol->getSelectedSlice() );
}
