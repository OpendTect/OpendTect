/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitime2depthzaxistrans.h"

#include "uizrangeinput.h"



uiTime2DepthZTransformBase::uiTime2DepthZTransformBase( uiParent* p, bool t2d )
    : uiZAxisTransform(p)
    , t2d_(t2d)
    , rangefld_(nullptr)
{
    mAttachCB( postFinalize(), uiTime2DepthZTransformBase::finalizeDoneCB );
}


uiTime2DepthZTransformBase::~uiTime2DepthZTransformBase()
{
    detachAllNotifiers();
}


StringView uiTime2DepthZTransformBase::toDomain() const
{
    return t2d_ ? ZDomain::sKeyDepth() : ZDomain::sKeyTime();
}


StringView uiTime2DepthZTransformBase::fromDomain() const
{
    return t2d_ ? ZDomain::sKeyTime() : ZDomain::sKeyDepth();
}


void uiTime2DepthZTransformBase::finalizeDoneCB(CallBacker*)
{
    if ( rangefld_ )
	mAttachCB( rangefld_->valuechanging,
		   uiTime2DepthZTransformBase::rangeChangedCB );
}


void uiTime2DepthZTransformBase::enableTargetSampling()
{
    if ( rangefld_ || finalized() )
	return;

    rangefld_ = new uiZRangeInput( this, t2d_, true );
    rangefld_->attach( alignedBelow, hAlignObj() );
}


bool
uiTime2DepthZTransformBase::getTargetSampling( StepInterval<float>& res ) const
{
    if ( !rangefld_ )
	return false;

    res = rangefld_->getFZRange();
    return true;
}
